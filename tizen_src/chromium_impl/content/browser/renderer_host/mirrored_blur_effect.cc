/*
 * Copyright (C) 2018 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "content/browser/renderer_host/mirrored_blur_effect.h"

namespace content {

const int widthFBO = 320;
const int heightFBO = 320;
const int widthMaxFBO = 360;
const int heightMaxFBO = 360;

MirroredBlurEffect::MirroredBlurEffect(Evas_GL_API* evas_gl_api)
    : evas_gl_api_(evas_gl_api),
      m_initShaderAndFBO(0),
      fbo_texture_(0),
      fbo_depth_(0),
      fbo_(0),
      program_(0),
      blur_program(0),
      mvp_matrix_id(0),
      texture_id(0),
      bias_(0),
      blur_mvp_matrix_id(0),
      blur_texture_id(0),
      position_attrib_(0),
      texture_attrib_(0),
      blur_position_attrib_(0),
      blur_texture_attrib_(0),
      index_buffer_obj_(0) {}

GLuint MirroredBlurEffect::LoadAndCompileShaders(const char* vertexShader,
                                                 const char* fragmentShader) {
  GLuint vertShader = evas_gl_api_->glCreateShader(GL_VERTEX_SHADER);
  GLuint fragShader = evas_gl_api_->glCreateShader(GL_FRAGMENT_SHADER);
  const char* p = vertexShader;
  GLint bShaderCompiled = GL_FALSE;
  evas_gl_api_->glShaderSource(vertShader, 1, &p, NULL);
  evas_gl_api_->glCompileShader(vertShader);
  evas_gl_api_->glGetShaderiv(vertShader, GL_COMPILE_STATUS, &bShaderCompiled);
  if (bShaderCompiled == GL_FALSE) {
    LOG(ERROR) << "MirroredBlurEffect: SHADER compiled fail";
  }
  // Fragment shader compilation.
  p = fragmentShader;
  evas_gl_api_->glShaderSource(fragShader, 1, &p, NULL);
  evas_gl_api_->glCompileShader(fragShader);
  bShaderCompiled = GL_FALSE;
  evas_gl_api_->glGetShaderiv(fragShader, GL_COMPILE_STATUS, &bShaderCompiled);
  if (bShaderCompiled == GL_FALSE) {
    LOG(ERROR) << "MirroredBlurEffect: SHADER compiled fail";
  }
  // Create Program
  GLuint program = evas_gl_api_->glCreateProgram();
  evas_gl_api_->glAttachShader(program, vertShader);
  evas_gl_api_->glAttachShader(program, fragShader);
  // Link program.
  evas_gl_api_->glLinkProgram(program);
  GLint isLinked = 0;
  evas_gl_api_->glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
  if (isLinked == GL_FALSE) {
    LOG(ERROR) << "MirroredBlurEffect: SHADER link fail";
    evas_gl_api_->glDeleteShader(vertShader);
  }
  evas_gl_api_->glDeleteShader(fragShader);
  return program;
}

void MirroredBlurEffect::InitializeShaders() {
  // Shader for rendering basic texture
  const char vertexShader[] =
      "uniform mat4 mvpMatrix;\n"
      "attribute vec4 aVertexPosition;\n"
      "attribute vec4 aTextureCoord;\n"
      "varying vec2 vCoordinates;\n"
      "void main()\n"
      "{\n"
      "    gl_Position = mvpMatrix * aVertexPosition;\n"
      "    vCoordinates = aTextureCoord.xy;\n"
      "}";

  const char fragmentShader[] =
      "precision mediump float;\n"
      "uniform sampler2D uTexture;\n"
      "varying vec2 vCoordinates;\n"
      "void main()\n"
      "{\n"
      "    vec3 color = texture2D(uTexture, vCoordinates).rgb;"
      "    gl_FragColor = vec4(color, 1.0);\n"
      "}";

  // Shader for rendering blur texture
  const char vertexShaderBlur[] =
      "uniform mat4 mvpMatrix;\n"
      "attribute vec4 aVertexPosition;\n"
      "attribute vec4 aTextureCoord;\n"
      "varying vec2 vCoordinates;\n"
      "void main()\n"
      "{\n"
      "    gl_Position = mvpMatrix * aVertexPosition;\n"
      "    vCoordinates = aTextureCoord.xy;\n"
      "}";

  const char fragmentShaderBlur[] =
      "precision mediump float;\n"
      "uniform sampler2D uTexture;\n"
      "varying vec2 vCoordinates;\n"
      "uniform float bias;\n"
      "void main()\n"
      "{\n"
      "    vec3 color = texture2D(uTexture, vCoordinates, bias).rgb;"
      "    gl_FragColor = vec4(color, 1.0);\n"
      "}";

  program_ = LoadAndCompileShaders(vertexShader, fragmentShader);
  blur_program = LoadAndCompileShaders(vertexShaderBlur, fragmentShaderBlur);

  const GLushort index_attributes[] = {0, 1, 2, 2, 3, 0};
  evas_gl_api_->glGenBuffers(1, &index_buffer_obj_);
  evas_gl_api_->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_obj_);

  evas_gl_api_->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_attributes),
                             index_attributes, GL_STATIC_DRAW);
  position_attrib_ =
      evas_gl_api_->glGetAttribLocation(program_, "aVertexPosition");
  texture_attrib_ =
      evas_gl_api_->glGetAttribLocation(program_, "aTextureCoord");
  mvp_matrix_id = evas_gl_api_->glGetUniformLocation(program_, "mvpMatrix");
  texture_id = evas_gl_api_->glGetUniformLocation(program_, "uTexture");

  blur_mvp_matrix_id =
      evas_gl_api_->glGetUniformLocation(blur_program, "mvpMatrix");
  blur_texture_id =
      evas_gl_api_->glGetUniformLocation(blur_program, "uTexture");
  bias_ = evas_gl_api_->glGetUniformLocation(blur_program, "bias");
  blur_position_attrib_ =
      evas_gl_api_->glGetAttribLocation(blur_program, "aVertexPosition");
  blur_texture_attrib_ =
      evas_gl_api_->glGetAttribLocation(blur_program, "aTextureCoord");
}

void MirroredBlurEffect::DrawToSurface(int w, int h) {
  GLuint vertex_buffer_obj;
  float mvp[16] = {
      1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
  };

  mvp[0] = 320.0f / w;
  mvp[5] = 320.0f / h;
  const GLfloat vertex_attributes[] = {
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
      1.0f,  1.0f,  0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f};

  evas_gl_api_->glGenBuffers(1, &vertex_buffer_obj);
  evas_gl_api_->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_obj);
  evas_gl_api_->glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_attributes),
                             vertex_attributes, GL_STATIC_DRAW);

  evas_gl_api_->glViewport(0, 0, w, h);
  evas_gl_api_->glUseProgram(program_);

  evas_gl_api_->glUniformMatrix4fv(mvp_matrix_id, 1, GL_FALSE, mvp);

  evas_gl_api_->glActiveTexture(GL_TEXTURE0);
  evas_gl_api_->glBindTexture(GL_TEXTURE_2D, fbo_texture_);
  evas_gl_api_->glUniform1i(texture_id, 0);

  evas_gl_api_->glEnableVertexAttribArray(position_attrib_);
  evas_gl_api_->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_obj);
  evas_gl_api_->glVertexAttribPointer(position_attrib_, 3, GL_FLOAT, GL_FALSE,
                                      5 * sizeof(GLfloat), NULL);

  evas_gl_api_->glEnableVertexAttribArray(texture_attrib_);
  evas_gl_api_->glVertexAttribPointer(texture_attrib_, 2, GL_FLOAT, GL_FALSE,
                                      5 * sizeof(GL_FLOAT),
                                      (void*)(3 * sizeof(GLfloat)));
  evas_gl_api_->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_obj_);
  evas_gl_api_->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);
  evas_gl_api_->glDisableVertexAttribArray(position_attrib_);
  evas_gl_api_->glDisableVertexAttribArray(texture_attrib_);
  evas_gl_api_->glDeleteBuffers(1, &vertex_buffer_obj);
  evas_gl_api_->glFlush();
}

void MirroredBlurEffect::DrawBlur(int w, int h) {
  GLuint blur_vertex_buffer_obj;
  const GLfloat vertex_attributes[] = {
      -1.0f, 1.0f,  0.0f, -1.0f, 2.0f,  -1.0f, -1.0f, 0.0f, -1.0, -1.0,
      1.0f,  -1.0f, 0.0f, 2.0f,  -1.0f, 1.0f,  1.0f,  0.0f, 2.0f, 2.0f};
  evas_gl_api_->glGenBuffers(1, &blur_vertex_buffer_obj);
  evas_gl_api_->glBindBuffer(GL_ARRAY_BUFFER, blur_vertex_buffer_obj);
  evas_gl_api_->glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_attributes),
                             vertex_attributes, GL_STATIC_DRAW);
  evas_gl_api_->glViewport(0, 0, w, h);
  evas_gl_api_->glUseProgram(blur_program);

  float view[16];
  float mview[16];
  InitMatrix(view);
  // FIXME: Need to check why changing scaling factor fixes app blur issue on
  // 4.0. On tizen 2.3.2 its 2.9 and on tizen 3.0 its 3.0.
  view[0] = (320.0f / w) * 3.1;
  view[5] = (float)w / h * view[0];
  InitMatrix(mview);
  MultiplyMatrix(mview, view, mview);

  evas_gl_api_->glActiveTexture(GL_TEXTURE0);
  evas_gl_api_->glBindTexture(GL_TEXTURE_2D, fbo_texture_);
  evas_gl_api_->glUniformMatrix4fv(blur_mvp_matrix_id, 1, GL_FALSE, mview);
  evas_gl_api_->glUniform1i(blur_texture_id, 0);
  evas_gl_api_->glUniform1f(bias_, 4.0f);

  evas_gl_api_->glEnableVertexAttribArray(blur_position_attrib_);
  evas_gl_api_->glBindBuffer(GL_ARRAY_BUFFER, blur_vertex_buffer_obj);
  evas_gl_api_->glVertexAttribPointer(blur_position_attrib_, 3, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat), NULL);

  evas_gl_api_->glEnableVertexAttribArray(blur_texture_attrib_);
  evas_gl_api_->glVertexAttribPointer(blur_texture_attrib_, 2, GL_FLOAT,
                                      GL_FALSE, 5 * sizeof(GLfloat),
                                      (void*)(3 * sizeof(GLfloat)));

  evas_gl_api_->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_obj_);
  evas_gl_api_->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);
  evas_gl_api_->glDisableVertexAttribArray(blur_position_attrib_);
  evas_gl_api_->glDisableVertexAttribArray(blur_texture_attrib_);
  evas_gl_api_->glDeleteBuffers(1, &blur_vertex_buffer_obj);
}

void MirroredBlurEffect::InitMatrix(float matrix[16]) {
  matrix[0] = 1.0f;
  matrix[1] = 0.0f;
  matrix[2] = 0.0f;
  matrix[3] = 0.0f;

  matrix[4] = 0.0f;
  matrix[5] = 1.0f;
  matrix[6] = 0.0f;
  matrix[7] = 0.0f;

  matrix[8] = 0.0f;
  matrix[9] = 0.0f;
  matrix[10] = 1.0f;
  matrix[11] = 0.0f;

  matrix[12] = 0.0f;
  matrix[13] = 0.0f;
  matrix[14] = 0.0f;
  matrix[15] = 1.0f;
}

void MirroredBlurEffect::MultiplyMatrix(float matrix[16],
                                        const float matrix0[16],
                                        const float matrix1[16]) {
  int i;
  int row;
  int column;
  float temp[16];

  for (column = 0; column < 4; column++) {
    for (row = 0; row < 4; row++) {
      temp[column * 4 + row] = 0.0f;
      for (i = 0; i < 4; i++)
        temp[column * 4 + row] +=
            matrix0[i * 4 + row] * matrix1[column * 4 + i];
    }
  }

  for (i = 0; i < 16; i++)
    matrix[i] = temp[i];
}

void MirroredBlurEffect::InitializeFBO(int width, int height) {
  evas_gl_api_->glGenTextures(1, &fbo_texture_);
  evas_gl_api_->glBindTexture(GL_TEXTURE_2D, fbo_texture_);
  evas_gl_api_->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, 0);
  evas_gl_api_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR_MIPMAP_NEAREST);
  evas_gl_api_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);
  evas_gl_api_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                GL_MIRRORED_REPEAT);
  evas_gl_api_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                GL_MIRRORED_REPEAT);
  evas_gl_api_->glGenRenderbuffers(1, &fbo_depth_);
  evas_gl_api_->glBindRenderbuffer(GL_RENDERBUFFER, fbo_depth_);
  evas_gl_api_->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                                      width, height);
  evas_gl_api_->glBindRenderbuffer(GL_RENDERBUFFER, 0);
  evas_gl_api_->glGenFramebuffers(1, &fbo_);
  evas_gl_api_->glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  evas_gl_api_->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_2D, fbo_texture_, 0);
  evas_gl_api_->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                          GL_RENDERBUFFER, fbo_depth_);
  GLuint status = evas_gl_api_->glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    LOG(ERROR) << "MirroredBlurEffect: Incomplete FrameBuffer";
  }
  evas_gl_api_->glBindRenderbuffer(GL_RENDERBUFFER, 0);
  evas_gl_api_->glBindTexture(GL_TEXTURE_2D, 0);
  evas_gl_api_->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MirroredBlurEffect::InitializeForEffect() {
  if (m_initShaderAndFBO == 0) {
    InitializeShaders();
    InitializeFBO(widthFBO, heightFBO);
    m_initShaderAndFBO = 1;
  }
}

void MirroredBlurEffect::PaintToBackupTextureEffect() {
  if (m_initShaderAndFBO) {
    evas_gl_api_->glBindTexture(GL_TEXTURE_2D, fbo_texture_);
    evas_gl_api_->glGenerateMipmap(GL_TEXTURE_2D);
    DrawBlur(widthMaxFBO, heightMaxFBO);
    evas_gl_api_->glBindTexture(GL_TEXTURE_2D, fbo_texture_);
    DrawToSurface(widthMaxFBO, heightMaxFBO);
  }
}

void MirroredBlurEffect::ClearBlurEffectResources() {
  if (m_initShaderAndFBO) {
    evas_gl_api_->glDeleteTextures(1, &fbo_texture_);
    evas_gl_api_->glDeleteRenderbuffers(1, &fbo_depth_);
    evas_gl_api_->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    evas_gl_api_->glDeleteFramebuffers(1, &fbo_);
    m_initShaderAndFBO = 0;
  }
}
}  // namespace content
