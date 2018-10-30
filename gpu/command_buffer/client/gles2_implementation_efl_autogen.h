// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated from
// gpu/command_buffer/build_gles2_cmd_buffer.py
// It's formatted by clang-format using chromium coding style:
//    clang-format -i -style=chromium filename
// DO NOT EDIT!

// This file is included by gles2_implementation_efl.cc.
#ifndef GPU_COMMAND_BUFFER_CLIENT_GLES2_IMPLEMENTATION_EFL_AUTOGEN_H_
#define GPU_COMMAND_BUFFER_CLIENT_GLES2_IMPLEMENTATION_EFL_AUTOGEN_H_

void GLES2ImplementationEfl::ActiveTexture(GLenum texture) {
  evas_gl_api_->glActiveTexture(texture);
}

void GLES2ImplementationEfl::AttachShader(GLuint program, GLuint shader) {
  evas_gl_api_->glAttachShader(program, shader);
}

void GLES2ImplementationEfl::BindAttribLocation(GLuint program,
                                                GLuint index,
                                                const char* name) {
  evas_gl_api_->glBindAttribLocation(program, index, name);
}

void GLES2ImplementationEfl::BindBuffer(GLenum target, GLuint buffer) {
  evas_gl_api_->glBindBuffer(target, buffer);
}

void GLES2ImplementationEfl::BindBufferBase(GLenum /* target */,
                                            GLuint /* index */,
                                            GLuint /* buffer */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::BindBufferRange(GLenum /* target */,
                                             GLuint /* index */,
                                             GLuint /* buffer */,
                                             GLintptr /* offset */,
                                             GLsizeiptr /* size */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::BindFramebuffer(GLenum target,
                                             GLuint framebuffer) {
  evas_gl_api_->glBindFramebuffer(target, framebuffer);
}

void GLES2ImplementationEfl::BindRenderbuffer(GLenum target,
                                              GLuint renderbuffer) {
  evas_gl_api_->glBindRenderbuffer(target, renderbuffer);
}

void GLES2ImplementationEfl::BindSampler(GLuint /* unit */,
                                         GLuint /* sampler */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::BindTexture(GLenum target, GLuint texture) {
  DoBindTexture(target, texture);
}

void GLES2ImplementationEfl::BindTransformFeedback(
    GLenum /* target */,
    GLuint /* transformfeedback */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::BlendColor(GLclampf red,
                                        GLclampf green,
                                        GLclampf blue,
                                        GLclampf alpha) {
  evas_gl_api_->glBlendColor(red, green, blue, alpha);
}

void GLES2ImplementationEfl::BlendEquation(GLenum mode) {
  evas_gl_api_->glBlendEquation(mode);
}

void GLES2ImplementationEfl::BlendEquationSeparate(GLenum modeRGB,
                                                   GLenum modeAlpha) {
  evas_gl_api_->glBlendEquationSeparate(modeRGB, modeAlpha);
}

void GLES2ImplementationEfl::BlendFunc(GLenum sfactor, GLenum dfactor) {
  evas_gl_api_->glBlendFunc(sfactor, dfactor);
}

void GLES2ImplementationEfl::BlendFuncSeparate(GLenum srcRGB,
                                               GLenum dstRGB,
                                               GLenum srcAlpha,
                                               GLenum dstAlpha) {
  evas_gl_api_->glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void GLES2ImplementationEfl::BufferData(GLenum target,
                                        GLsizeiptr size,
                                        const void* data,
                                        GLenum usage) {
  evas_gl_api_->glBufferData(target, size, data, usage);
}

void GLES2ImplementationEfl::BufferSubData(GLenum target,
                                           GLintptr offset,
                                           GLsizeiptr size,
                                           const void* data) {
  evas_gl_api_->glBufferSubData(target, offset, size, data);
}

GLenum GLES2ImplementationEfl::CheckFramebufferStatus(GLenum target) {
  return evas_gl_api_->glCheckFramebufferStatus(target);
}

void GLES2ImplementationEfl::Clear(GLbitfield mask) {
  evas_gl_api_->glClear(mask);
}

void GLES2ImplementationEfl::ClearBufferfi(GLenum /* buffer */,
                                           GLint /* drawbuffers */,
                                           GLfloat /* depth */,
                                           GLint /* stencil */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ClearBufferfv(GLenum /* buffer */,
                                           GLint /* drawbuffers */,
                                           const GLfloat* /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ClearBufferiv(GLenum /* buffer */,
                                           GLint /* drawbuffers */,
                                           const GLint* /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ClearBufferuiv(GLenum /* buffer */,
                                            GLint /* drawbuffers */,
                                            const GLuint* /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ClearColor(GLclampf red,
                                        GLclampf green,
                                        GLclampf blue,
                                        GLclampf alpha) {
  evas_gl_api_->glClearColor(red, green, blue, alpha);
}

void GLES2ImplementationEfl::ClearDepthf(GLclampf depth) {
  evas_gl_api_->glClearDepthf(depth);
}

void GLES2ImplementationEfl::ClearStencil(GLint s) {
  evas_gl_api_->glClearStencil(s);
}

GLenum GLES2ImplementationEfl::ClientWaitSync(GLsync /* sync */,
                                              GLbitfield /* flags */,
                                              GLuint64 /* timeout */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::ColorMask(GLboolean red,
                                       GLboolean green,
                                       GLboolean blue,
                                       GLboolean alpha) {
  evas_gl_api_->glColorMask(red, green, blue, alpha);
}

void GLES2ImplementationEfl::CompileShader(GLuint shader) {
  evas_gl_api_->glCompileShader(shader);
}

void GLES2ImplementationEfl::CompressedTexImage2D(GLenum target,
                                                  GLint level,
                                                  GLenum internalformat,
                                                  GLsizei width,
                                                  GLsizei height,
                                                  GLint border,
                                                  GLsizei imageSize,
                                                  const void* data) {
  evas_gl_api_->glCompressedTexImage2D(target, level, internalformat, width,
                                       height, border, imageSize, data);
}

void GLES2ImplementationEfl::CompressedTexSubImage2D(GLenum target,
                                                     GLint level,
                                                     GLint xoffset,
                                                     GLint yoffset,
                                                     GLsizei width,
                                                     GLsizei height,
                                                     GLenum format,
                                                     GLsizei imageSize,
                                                     const void* data) {
  evas_gl_api_->glCompressedTexSubImage2D(
      target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

void GLES2ImplementationEfl::CompressedTexImage3D(GLenum /* target */,
                                                  GLint /* level */,
                                                  GLenum /* internalformat */,
                                                  GLsizei /* width */,
                                                  GLsizei /* height */,
                                                  GLsizei /* depth */,
                                                  GLint /* border */,
                                                  GLsizei /* imageSize */,
                                                  const void* /* data */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CompressedTexSubImage3D(GLenum /* target */,
                                                     GLint /* level */,
                                                     GLint /* xoffset */,
                                                     GLint /* yoffset */,
                                                     GLint /* zoffset */,
                                                     GLsizei /* width */,
                                                     GLsizei /* height */,
                                                     GLsizei /* depth */,
                                                     GLenum /* format */,
                                                     GLsizei /* imageSize */,
                                                     const void* /* data */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CopyBufferSubData(GLenum /* readtarget */,
                                               GLenum /* writetarget */,
                                               GLintptr /* readoffset */,
                                               GLintptr /* writeoffset */,
                                               GLsizeiptr /* size */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CopyTexImage2D(GLenum target,
                                            GLint level,
                                            GLenum internalformat,
                                            GLint x,
                                            GLint y,
                                            GLsizei width,
                                            GLsizei height,
                                            GLint border) {
  evas_gl_api_->glCopyTexImage2D(target, level, internalformat, x, y, width,
                                 height, border);
}

void GLES2ImplementationEfl::CopyTexSubImage2D(GLenum target,
                                               GLint level,
                                               GLint xoffset,
                                               GLint yoffset,
                                               GLint x,
                                               GLint y,
                                               GLsizei width,
                                               GLsizei height) {
  evas_gl_api_->glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y,
                                    width, height);
}

void GLES2ImplementationEfl::CopyTexSubImage3D(GLenum /* target */,
                                               GLint /* level */,
                                               GLint /* xoffset */,
                                               GLint /* yoffset */,
                                               GLint /* zoffset */,
                                               GLint /* x */,
                                               GLint /* y */,
                                               GLsizei /* width */,
                                               GLsizei /* height */) {
  NOTIMPLEMENTED();
}

GLuint GLES2ImplementationEfl::CreateProgram() {
  return DoCreateProgram();
}

GLuint GLES2ImplementationEfl::CreateShader(GLenum type) {
  return evas_gl_api_->glCreateShader(type);
}

void GLES2ImplementationEfl::CullFace(GLenum mode) {
  evas_gl_api_->glCullFace(mode);
}

void GLES2ImplementationEfl::DeleteBuffers(GLsizei n, const GLuint* buffers) {
  evas_gl_api_->glDeleteBuffers(n, buffers);
}

void GLES2ImplementationEfl::DeleteFramebuffers(GLsizei n,
                                                const GLuint* framebuffers) {
  evas_gl_api_->glDeleteFramebuffers(n, framebuffers);
}

void GLES2ImplementationEfl::DeleteProgram(GLuint program) {
  DoDeleteProgram(program);
}

void GLES2ImplementationEfl::DeleteRenderbuffers(GLsizei n,
                                                 const GLuint* renderbuffers) {
  evas_gl_api_->glDeleteRenderbuffers(n, renderbuffers);
}

void GLES2ImplementationEfl::DeleteSamplers(GLsizei /* n */,
                                            const GLuint* /* samplers */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::DeleteSync(GLsync /* sync */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::DeleteShader(GLuint shader) {
  evas_gl_api_->glDeleteShader(shader);
}

void GLES2ImplementationEfl::DeleteTextures(GLsizei n, const GLuint* textures) {
  DoDeleteTextures(n, textures);
}

void GLES2ImplementationEfl::DeleteTransformFeedbacks(GLsizei /* n */,
                                                      const GLuint* /* ids */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::DepthFunc(GLenum func) {
  evas_gl_api_->glDepthFunc(func);
}

void GLES2ImplementationEfl::DepthMask(GLboolean flag) {
  evas_gl_api_->glDepthMask(flag);
}

void GLES2ImplementationEfl::DepthRangef(GLclampf zNear, GLclampf zFar) {
  evas_gl_api_->glDepthRangef(zNear, zFar);
}

void GLES2ImplementationEfl::DetachShader(GLuint program, GLuint shader) {
  evas_gl_api_->glDetachShader(program, shader);
}

void GLES2ImplementationEfl::Disable(GLenum cap) {
  evas_gl_api_->glDisable(cap);
}

void GLES2ImplementationEfl::DisableVertexAttribArray(GLuint index) {
  evas_gl_api_->glDisableVertexAttribArray(index);
}

void GLES2ImplementationEfl::DrawArrays(GLenum mode,
                                        GLint first,
                                        GLsizei count) {
  evas_gl_api_->glDrawArrays(mode, first, count);
}

void GLES2ImplementationEfl::DrawElements(GLenum mode,
                                          GLsizei count,
                                          GLenum type,
                                          const void* indices) {
  evas_gl_api_->glDrawElements(mode, count, type, indices);
}

void GLES2ImplementationEfl::DrawRangeElements(GLenum /* mode */,
                                               GLuint /* start */,
                                               GLuint /* end */,
                                               GLsizei /* count */,
                                               GLenum /* type */,
                                               const void* /* indices */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::Enable(GLenum cap) {
  evas_gl_api_->glEnable(cap);
}

void GLES2ImplementationEfl::EnableVertexAttribArray(GLuint index) {
  evas_gl_api_->glEnableVertexAttribArray(index);
}

GLsync GLES2ImplementationEfl::FenceSync(GLenum /* condition */,
                                         GLbitfield /* flags */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::Finish() {
  evas_gl_api_->glFinish();
}

void GLES2ImplementationEfl::Flush() {
  evas_gl_api_->glFlush();
}

void GLES2ImplementationEfl::FramebufferRenderbuffer(GLenum target,
                                                     GLenum attachment,
                                                     GLenum renderbuffertarget,
                                                     GLuint renderbuffer) {
  evas_gl_api_->glFramebufferRenderbuffer(target, attachment,
                                          renderbuffertarget, renderbuffer);
}

void GLES2ImplementationEfl::FramebufferTexture2D(GLenum target,
                                                  GLenum attachment,
                                                  GLenum textarget,
                                                  GLuint texture,
                                                  GLint level) {
  evas_gl_api_->glFramebufferTexture2D(target, attachment, textarget, texture,
                                       level);
}

void GLES2ImplementationEfl::FramebufferTextureLayer(GLenum /* target */,
                                                     GLenum /* attachment */,
                                                     GLuint /* texture */,
                                                     GLint /* level */,
                                                     GLint /* layer */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::FrontFace(GLenum mode) {
  evas_gl_api_->glFrontFace(mode);
}

void GLES2ImplementationEfl::GenBuffers(GLsizei n, GLuint* buffers) {
  evas_gl_api_->glGenBuffers(n, buffers);
}

void GLES2ImplementationEfl::GenerateMipmap(GLenum target) {
  evas_gl_api_->glGenerateMipmap(target);
}

void GLES2ImplementationEfl::GenFramebuffers(GLsizei n, GLuint* framebuffers) {
  evas_gl_api_->glGenFramebuffers(n, framebuffers);
}

void GLES2ImplementationEfl::GenRenderbuffers(GLsizei n,
                                              GLuint* renderbuffers) {
  evas_gl_api_->glGenRenderbuffers(n, renderbuffers);
}

void GLES2ImplementationEfl::GenSamplers(GLsizei /* n */,
                                         GLuint* /* samplers */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GenTextures(GLsizei n, GLuint* textures) {
  DoGenTextures(n, textures);
}

void GLES2ImplementationEfl::GenTransformFeedbacks(GLsizei /* n */,
                                                   GLuint* /* ids */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetActiveAttrib(GLuint program,
                                             GLuint index,
                                             GLsizei bufsize,
                                             GLsizei* length,
                                             GLint* size,
                                             GLenum* type,
                                             char* name) {
  evas_gl_api_->glGetActiveAttrib(program, index, bufsize, length, size, type,
                                  name);
}

void GLES2ImplementationEfl::GetActiveUniform(GLuint program,
                                              GLuint index,
                                              GLsizei bufsize,
                                              GLsizei* length,
                                              GLint* size,
                                              GLenum* type,
                                              char* name) {
  evas_gl_api_->glGetActiveUniform(program, index, bufsize, length, size, type,
                                   name);
}

void GLES2ImplementationEfl::GetActiveUniformBlockiv(GLuint /* program */,
                                                     GLuint /* index */,
                                                     GLenum /* pname */,
                                                     GLint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetActiveUniformBlockName(GLuint /* program */,
                                                       GLuint /* index */,
                                                       GLsizei /* bufsize */,
                                                       GLsizei* /* length */,
                                                       char* /* name */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetActiveUniformsiv(GLuint /* program */,
                                                 GLsizei /* count */,
                                                 const GLuint* /* indices */,
                                                 GLenum /* pname */,
                                                 GLint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetAttachedShaders(GLuint program,
                                                GLsizei maxcount,
                                                GLsizei* count,
                                                GLuint* shaders) {
  evas_gl_api_->glGetAttachedShaders(program, maxcount, count, shaders);
}

GLint GLES2ImplementationEfl::GetAttribLocation(GLuint program,
                                                const char* name) {
  return evas_gl_api_->glGetAttribLocation(program, name);
}

void GLES2ImplementationEfl::GetBooleanv(GLenum pname, GLboolean* params) {
  evas_gl_api_->glGetBooleanv(pname, params);
}

void GLES2ImplementationEfl::GetBufferParameteri64v(GLenum /* target */,
                                                    GLenum /* pname */,
                                                    GLint64* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetBufferParameteriv(GLenum target,
                                                  GLenum pname,
                                                  GLint* params) {
  evas_gl_api_->glGetBufferParameteriv(target, pname, params);
}

GLenum GLES2ImplementationEfl::GetError() {
  return evas_gl_api_->glGetError();
}

void GLES2ImplementationEfl::GetFloatv(GLenum pname, GLfloat* params) {
  evas_gl_api_->glGetFloatv(pname, params);
}

GLint GLES2ImplementationEfl::GetFragDataLocation(GLuint /* program */,
                                                  const char* /* name */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::GetFramebufferAttachmentParameteriv(
    GLenum target,
    GLenum attachment,
    GLenum pname,
    GLint* params) {
  evas_gl_api_->glGetFramebufferAttachmentParameteriv(target, attachment, pname,
                                                      params);
}

void GLES2ImplementationEfl::GetInteger64v(GLenum /* pname */,
                                           GLint64* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetIntegeri_v(GLenum /* pname */,
                                           GLuint /* index */,
                                           GLint* /* data */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetInteger64i_v(GLenum /* pname */,
                                             GLuint /* index */,
                                             GLint64* /* data */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetIntegerv(GLenum pname, GLint* params) {
  evas_gl_api_->glGetIntegerv(pname, params);
}

void GLES2ImplementationEfl::GetInternalformativ(GLenum /* target */,
                                                 GLenum /* format */,
                                                 GLenum /* pname */,
                                                 GLsizei /* bufSize */,
                                                 GLint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetProgramiv(GLuint program,
                                          GLenum pname,
                                          GLint* params) {
  evas_gl_api_->glGetProgramiv(program, pname, params);
}

void GLES2ImplementationEfl::GetProgramInfoLog(GLuint program,
                                               GLsizei bufsize,
                                               GLsizei* length,
                                               char* infolog) {
  evas_gl_api_->glGetProgramInfoLog(program, bufsize, length, infolog);
}

void GLES2ImplementationEfl::GetRenderbufferParameteriv(GLenum target,
                                                        GLenum pname,
                                                        GLint* params) {
  evas_gl_api_->glGetRenderbufferParameteriv(target, pname, params);
}

void GLES2ImplementationEfl::GetSamplerParameterfv(GLuint /* sampler */,
                                                   GLenum /* pname */,
                                                   GLfloat* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetSamplerParameteriv(GLuint /* sampler */,
                                                   GLenum /* pname */,
                                                   GLint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetShaderiv(GLuint shader,
                                         GLenum pname,
                                         GLint* params) {
  evas_gl_api_->glGetShaderiv(shader, pname, params);
}

void GLES2ImplementationEfl::GetShaderInfoLog(GLuint shader,
                                              GLsizei bufsize,
                                              GLsizei* length,
                                              char* infolog) {
  evas_gl_api_->glGetShaderInfoLog(shader, bufsize, length, infolog);
}

void GLES2ImplementationEfl::GetShaderPrecisionFormat(GLenum shadertype,
                                                      GLenum precisiontype,
                                                      GLint* range,
                                                      GLint* precision) {
  evas_gl_api_->glGetShaderPrecisionFormat(shadertype, precisiontype, range,
                                           precision);
}

void GLES2ImplementationEfl::GetShaderSource(GLuint shader,
                                             GLsizei bufsize,
                                             GLsizei* length,
                                             char* source) {
  evas_gl_api_->glGetShaderSource(shader, bufsize, length, source);
}

const GLubyte* GLES2ImplementationEfl::GetString(GLenum name) {
  return evas_gl_api_->glGetString(name);
}

const GLubyte* GLES2ImplementationEfl::GetStringi(GLenum /* name */,
                                                  GLuint /* index */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::GetSynciv(GLsync /* sync */,
                                       GLenum /* pname */,
                                       GLsizei /* bufsize */,
                                       GLsizei* /* length */,
                                       GLint* /* values */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetTexParameterfv(GLenum target,
                                               GLenum pname,
                                               GLfloat* params) {
  evas_gl_api_->glGetTexParameterfv(target, pname, params);
}

void GLES2ImplementationEfl::GetTexParameteriv(GLenum target,
                                               GLenum pname,
                                               GLint* params) {
  evas_gl_api_->glGetTexParameteriv(target, pname, params);
}

void GLES2ImplementationEfl::GetTransformFeedbackVarying(GLuint /* program */,
                                                         GLuint /* index */,
                                                         GLsizei /* bufsize */,
                                                         GLsizei* /* length */,
                                                         GLsizei* /* size */,
                                                         GLenum* /* type */,
                                                         char* /* name */) {
  NOTIMPLEMENTED();
}

GLuint GLES2ImplementationEfl::GetUniformBlockIndex(GLuint /* program */,
                                                    const char* /* name */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::GetUniformfv(GLuint program,
                                          GLint location,
                                          GLfloat* params) {
  evas_gl_api_->glGetUniformfv(program, location, params);
}

void GLES2ImplementationEfl::GetUniformiv(GLuint program,
                                          GLint location,
                                          GLint* params) {
  evas_gl_api_->glGetUniformiv(program, location, params);
}

void GLES2ImplementationEfl::GetUniformuiv(GLuint /* program */,
                                           GLint /* location */,
                                           GLuint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetUniformIndices(GLuint /* program */,
                                               GLsizei /* count */,
                                               const char* const* /* names */,
                                               GLuint* /* indices */) {
  NOTIMPLEMENTED();
}

GLint GLES2ImplementationEfl::GetUniformLocation(GLuint program,
                                                 const char* name) {
  return evas_gl_api_->glGetUniformLocation(program, name);
}

void GLES2ImplementationEfl::GetVertexAttribfv(GLuint index,
                                               GLenum pname,
                                               GLfloat* params) {
  evas_gl_api_->glGetVertexAttribfv(index, pname, params);
}

void GLES2ImplementationEfl::GetVertexAttribiv(GLuint index,
                                               GLenum pname,
                                               GLint* params) {
  evas_gl_api_->glGetVertexAttribiv(index, pname, params);
}

void GLES2ImplementationEfl::GetVertexAttribIiv(GLuint /* index */,
                                                GLenum /* pname */,
                                                GLint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetVertexAttribIuiv(GLuint /* index */,
                                                 GLenum /* pname */,
                                                 GLuint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetVertexAttribPointerv(GLuint index,
                                                     GLenum pname,
                                                     void** pointer) {
  evas_gl_api_->glGetVertexAttribPointerv(index, pname, pointer);
}

void GLES2ImplementationEfl::Hint(GLenum target, GLenum mode) {
  evas_gl_api_->glHint(target, mode);
}

void GLES2ImplementationEfl::InvalidateFramebuffer(
    GLenum /* target */,
    GLsizei /* count */,
    const GLenum* /* attachments */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::InvalidateSubFramebuffer(
    GLenum /* target */,
    GLsizei /* count */,
    const GLenum* /* attachments */,
    GLint /* x */,
    GLint /* y */,
    GLsizei /* width */,
    GLsizei /* height */) {
  NOTIMPLEMENTED();
}

GLboolean GLES2ImplementationEfl::IsBuffer(GLuint buffer) {
  return evas_gl_api_->glIsBuffer(buffer);
}

GLboolean GLES2ImplementationEfl::IsEnabled(GLenum cap) {
  return evas_gl_api_->glIsEnabled(cap);
}

GLboolean GLES2ImplementationEfl::IsFramebuffer(GLuint framebuffer) {
  return evas_gl_api_->glIsFramebuffer(framebuffer);
}

GLboolean GLES2ImplementationEfl::IsProgram(GLuint program) {
  return evas_gl_api_->glIsProgram(program);
}

GLboolean GLES2ImplementationEfl::IsRenderbuffer(GLuint renderbuffer) {
  return evas_gl_api_->glIsRenderbuffer(renderbuffer);
}

GLboolean GLES2ImplementationEfl::IsSampler(GLuint /* sampler */) {
  NOTIMPLEMENTED();
  return 0;
}

GLboolean GLES2ImplementationEfl::IsShader(GLuint shader) {
  return evas_gl_api_->glIsShader(shader);
}

GLboolean GLES2ImplementationEfl::IsSync(GLsync /* sync */) {
  NOTIMPLEMENTED();
  return 0;
}

GLboolean GLES2ImplementationEfl::IsTexture(GLuint texture) {
  return evas_gl_api_->glIsTexture(texture);
}

GLboolean GLES2ImplementationEfl::IsTransformFeedback(
    GLuint /* transformfeedback */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::LineWidth(GLfloat width) {
  evas_gl_api_->glLineWidth(width);
}

void GLES2ImplementationEfl::LinkProgram(GLuint program) {
  evas_gl_api_->glLinkProgram(program);
}

void GLES2ImplementationEfl::PauseTransformFeedback() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::PixelStorei(GLenum pname, GLint param) {
  evas_gl_api_->glPixelStorei(pname, param);
}

void GLES2ImplementationEfl::PolygonOffset(GLfloat factor, GLfloat units) {
  evas_gl_api_->glPolygonOffset(factor, units);
}

void GLES2ImplementationEfl::ReadBuffer(GLenum /* src */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ReadPixels(GLint x,
                                        GLint y,
                                        GLsizei width,
                                        GLsizei height,
                                        GLenum format,
                                        GLenum type,
                                        void* pixels) {
  evas_gl_api_->glReadPixels(x, y, width, height, format, type, pixels);
}

void GLES2ImplementationEfl::ReleaseShaderCompiler() {
  evas_gl_api_->glReleaseShaderCompiler();
}

void GLES2ImplementationEfl::RenderbufferStorage(GLenum target,
                                                 GLenum internalformat,
                                                 GLsizei width,
                                                 GLsizei height) {
  evas_gl_api_->glRenderbufferStorage(target, internalformat, width, height);
}

void GLES2ImplementationEfl::ResumeTransformFeedback() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SampleCoverage(GLclampf value, GLboolean invert) {
  evas_gl_api_->glSampleCoverage(value, invert);
}

void GLES2ImplementationEfl::SamplerParameterf(GLuint /* sampler */,
                                               GLenum /* pname */,
                                               GLfloat /* param */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SamplerParameterfv(GLuint /* sampler */,
                                                GLenum /* pname */,
                                                const GLfloat* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SamplerParameteri(GLuint /* sampler */,
                                               GLenum /* pname */,
                                               GLint /* param */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SamplerParameteriv(GLuint /* sampler */,
                                                GLenum /* pname */,
                                                const GLint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::Scissor(GLint x,
                                     GLint y,
                                     GLsizei width,
                                     GLsizei height) {
  evas_gl_api_->glScissor(x, y, width, height);
}

void GLES2ImplementationEfl::ShaderBinary(GLsizei n,
                                          const GLuint* shaders,
                                          GLenum binaryformat,
                                          const void* binary,
                                          GLsizei length) {
  evas_gl_api_->glShaderBinary(n, shaders, binaryformat, binary, length);
}

void GLES2ImplementationEfl::ShaderSource(GLuint shader,
                                          GLsizei count,
                                          const GLchar* const* str,
                                          const GLint* length) {
  evas_gl_api_->glShaderSource(shader, count, str, length);
}

void GLES2ImplementationEfl::ShallowFinishCHROMIUM() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ShallowFlushCHROMIUM() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::OrderingBarrierCHROMIUM() {}

void GLES2ImplementationEfl::StencilFunc(GLenum func, GLint ref, GLuint mask) {
  evas_gl_api_->glStencilFunc(func, ref, mask);
}

void GLES2ImplementationEfl::StencilFuncSeparate(GLenum face,
                                                 GLenum func,
                                                 GLint ref,
                                                 GLuint mask) {
  evas_gl_api_->glStencilFuncSeparate(face, func, ref, mask);
}

void GLES2ImplementationEfl::StencilMask(GLuint mask) {
  evas_gl_api_->glStencilMask(mask);
}

void GLES2ImplementationEfl::StencilMaskSeparate(GLenum face, GLuint mask) {
  evas_gl_api_->glStencilMaskSeparate(face, mask);
}

void GLES2ImplementationEfl::StencilOp(GLenum fail,
                                       GLenum zfail,
                                       GLenum zpass) {
  evas_gl_api_->glStencilOp(fail, zfail, zpass);
}

void GLES2ImplementationEfl::StencilOpSeparate(GLenum face,
                                               GLenum fail,
                                               GLenum zfail,
                                               GLenum zpass) {
  evas_gl_api_->glStencilOpSeparate(face, fail, zfail, zpass);
}

void GLES2ImplementationEfl::TexImage2D(GLenum target,
                                        GLint level,
                                        GLint internalformat,
                                        GLsizei width,
                                        GLsizei height,
                                        GLint border,
                                        GLenum format,
                                        GLenum type,
                                        const void* pixels) {
  evas_gl_api_->glTexImage2D(target, level, internalformat, width, height,
                             border, format, type, pixels);
}

void GLES2ImplementationEfl::TexImage3D(GLenum /* target */,
                                        GLint /* level */,
                                        GLint /* internalformat */,
                                        GLsizei /* width */,
                                        GLsizei /* height */,
                                        GLsizei /* depth */,
                                        GLint /* border */,
                                        GLenum /* format */,
                                        GLenum /* type */,
                                        const void* /* pixels */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::TexParameterf(GLenum target,
                                           GLenum pname,
                                           GLfloat param) {
  evas_gl_api_->glTexParameterf(target, pname, param);
}

void GLES2ImplementationEfl::TexParameterfv(GLenum target,
                                            GLenum pname,
                                            const GLfloat* params) {
  evas_gl_api_->glTexParameterfv(target, pname, params);
}

void GLES2ImplementationEfl::TexParameteri(GLenum target,
                                           GLenum pname,
                                           GLint param) {
  evas_gl_api_->glTexParameteri(target, pname, param);
}

void GLES2ImplementationEfl::TexParameteriv(GLenum target,
                                            GLenum pname,
                                            const GLint* params) {
  evas_gl_api_->glTexParameteriv(target, pname, params);
}

void GLES2ImplementationEfl::TexStorage3D(GLenum /* target */,
                                          GLsizei /* levels */,
                                          GLenum /* internalFormat */,
                                          GLsizei /* width */,
                                          GLsizei /* height */,
                                          GLsizei /* depth */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::TexSubImage2D(GLenum target,
                                           GLint level,
                                           GLint xoffset,
                                           GLint yoffset,
                                           GLsizei width,
                                           GLsizei height,
                                           GLenum format,
                                           GLenum type,
                                           const void* pixels) {
  evas_gl_api_->glTexSubImage2D(target, level, xoffset, yoffset, width, height,
                                format, type, pixels);
}

void GLES2ImplementationEfl::TexSubImage3D(GLenum /* target */,
                                           GLint /* level */,
                                           GLint /* xoffset */,
                                           GLint /* yoffset */,
                                           GLint /* zoffset */,
                                           GLsizei /* width */,
                                           GLsizei /* height */,
                                           GLsizei /* depth */,
                                           GLenum /* format */,
                                           GLenum /* type */,
                                           const void* /* pixels */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::TransformFeedbackVaryings(
    GLuint /* program */,
    GLsizei /* count */,
    const char* const* /* varyings */,
    GLenum /* buffermode */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::Uniform1f(GLint location, GLfloat x) {
  DoUniform1f(location, x);
}

void GLES2ImplementationEfl::Uniform1fv(GLint location,
                                        GLsizei count,
                                        const GLfloat* v) {
  DoUniform1fv(location, count, v);
}

void GLES2ImplementationEfl::Uniform1i(GLint location, GLint x) {
  DoUniform1i(location, x);
}

void GLES2ImplementationEfl::Uniform1iv(GLint location,
                                        GLsizei count,
                                        const GLint* v) {
  DoUniform1iv(location, count, v);
}

void GLES2ImplementationEfl::Uniform1ui(GLint /* location */, GLuint /* x */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::Uniform1uiv(GLint /* location */,
                                         GLsizei /* count */,
                                         const GLuint* /* v */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::Uniform2f(GLint location, GLfloat x, GLfloat y) {
  DoUniform2f(location, x, y);
}

void GLES2ImplementationEfl::Uniform2fv(GLint location,
                                        GLsizei count,
                                        const GLfloat* v) {
  DoUniform2fv(location, count, v);
}

void GLES2ImplementationEfl::Uniform2i(GLint location, GLint x, GLint y) {
  DoUniform2i(location, x, y);
}

void GLES2ImplementationEfl::Uniform2iv(GLint location,
                                        GLsizei count,
                                        const GLint* v) {
  DoUniform2iv(location, count, v);
}

void GLES2ImplementationEfl::Uniform2ui(GLint /* location */,
                                        GLuint /* x */,
                                        GLuint /* y */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::Uniform2uiv(GLint /* location */,
                                         GLsizei /* count */,
                                         const GLuint* /* v */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::Uniform3f(GLint location,
                                       GLfloat x,
                                       GLfloat y,
                                       GLfloat z) {
  DoUniform3f(location, x, y, z);
}

void GLES2ImplementationEfl::Uniform3fv(GLint location,
                                        GLsizei count,
                                        const GLfloat* v) {
  DoUniform3fv(location, count, v);
}

void GLES2ImplementationEfl::Uniform3i(GLint location,
                                       GLint x,
                                       GLint y,
                                       GLint z) {
  DoUniform3i(location, x, y, z);
}

void GLES2ImplementationEfl::Uniform3iv(GLint location,
                                        GLsizei count,
                                        const GLint* v) {
  DoUniform3iv(location, count, v);
}

void GLES2ImplementationEfl::Uniform3ui(GLint /* location */,
                                        GLuint /* x */,
                                        GLuint /* y */,
                                        GLuint /* z */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::Uniform3uiv(GLint /* location */,
                                         GLsizei /* count */,
                                         const GLuint* /* v */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::Uniform4f(GLint location,
                                       GLfloat x,
                                       GLfloat y,
                                       GLfloat z,
                                       GLfloat w) {
  DoUniform4f(location, x, y, z, w);
}

void GLES2ImplementationEfl::Uniform4fv(GLint location,
                                        GLsizei count,
                                        const GLfloat* v) {
  DoUniform4fv(location, count, v);
}

void GLES2ImplementationEfl::Uniform4i(GLint location,
                                       GLint x,
                                       GLint y,
                                       GLint z,
                                       GLint w) {
  DoUniform4i(location, x, y, z, w);
}

void GLES2ImplementationEfl::Uniform4iv(GLint location,
                                        GLsizei count,
                                        const GLint* v) {
  DoUniform4iv(location, count, v);
}

void GLES2ImplementationEfl::Uniform4ui(GLint /* location */,
                                        GLuint /* x */,
                                        GLuint /* y */,
                                        GLuint /* z */,
                                        GLuint /* w */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::Uniform4uiv(GLint /* location */,
                                         GLsizei /* count */,
                                         const GLuint* /* v */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::UniformBlockBinding(GLuint /* program */,
                                                 GLuint /* index */,
                                                 GLuint /* binding */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::UniformMatrix2fv(GLint location,
                                              GLsizei count,
                                              GLboolean transpose,
                                              const GLfloat* value) {
  DoUniformMatrix2fv(location, count, transpose, value);
}

void GLES2ImplementationEfl::UniformMatrix2x3fv(GLint /* location */,
                                                GLsizei /* count */,
                                                GLboolean /* transpose */,
                                                const GLfloat* /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::UniformMatrix2x4fv(GLint /* location */,
                                                GLsizei /* count */,
                                                GLboolean /* transpose */,
                                                const GLfloat* /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::UniformMatrix3fv(GLint location,
                                              GLsizei count,
                                              GLboolean transpose,
                                              const GLfloat* value) {
  DoUniformMatrix3fv(location, count, transpose, value);
}

void GLES2ImplementationEfl::UniformMatrix3x2fv(GLint /* location */,
                                                GLsizei /* count */,
                                                GLboolean /* transpose */,
                                                const GLfloat* /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::UniformMatrix3x4fv(GLint /* location */,
                                                GLsizei /* count */,
                                                GLboolean /* transpose */,
                                                const GLfloat* /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::UniformMatrix4fv(GLint location,
                                              GLsizei count,
                                              GLboolean transpose,
                                              const GLfloat* value) {
  DoUniformMatrix4fv(location, count, transpose, value);
}

void GLES2ImplementationEfl::UniformMatrix4x2fv(GLint /* location */,
                                                GLsizei /* count */,
                                                GLboolean /* transpose */,
                                                const GLfloat* /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::UniformMatrix4x3fv(GLint /* location */,
                                                GLsizei /* count */,
                                                GLboolean /* transpose */,
                                                const GLfloat* /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::UseProgram(GLuint program) {
  DoUseProgram(program);
}

void GLES2ImplementationEfl::ValidateProgram(GLuint program) {
  evas_gl_api_->glValidateProgram(program);
}

void GLES2ImplementationEfl::VertexAttrib1f(GLuint indx, GLfloat x) {
  evas_gl_api_->glVertexAttrib1f(indx, x);
}

void GLES2ImplementationEfl::VertexAttrib1fv(GLuint indx,
                                             const GLfloat* values) {
  evas_gl_api_->glVertexAttrib1fv(indx, values);
}

void GLES2ImplementationEfl::VertexAttrib2f(GLuint indx, GLfloat x, GLfloat y) {
  evas_gl_api_->glVertexAttrib2f(indx, x, y);
}

void GLES2ImplementationEfl::VertexAttrib2fv(GLuint indx,
                                             const GLfloat* values) {
  evas_gl_api_->glVertexAttrib2fv(indx, values);
}

void GLES2ImplementationEfl::VertexAttrib3f(GLuint indx,
                                            GLfloat x,
                                            GLfloat y,
                                            GLfloat z) {
  evas_gl_api_->glVertexAttrib3f(indx, x, y, z);
}

void GLES2ImplementationEfl::VertexAttrib3fv(GLuint indx,
                                             const GLfloat* values) {
  evas_gl_api_->glVertexAttrib3fv(indx, values);
}

void GLES2ImplementationEfl::VertexAttrib4f(GLuint indx,
                                            GLfloat x,
                                            GLfloat y,
                                            GLfloat z,
                                            GLfloat w) {
  evas_gl_api_->glVertexAttrib4f(indx, x, y, z, w);
}

void GLES2ImplementationEfl::VertexAttrib4fv(GLuint indx,
                                             const GLfloat* values) {
  evas_gl_api_->glVertexAttrib4fv(indx, values);
}

void GLES2ImplementationEfl::VertexAttribI4i(GLuint /* indx */,
                                             GLint /* x */,
                                             GLint /* y */,
                                             GLint /* z */,
                                             GLint /* w */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::VertexAttribI4iv(GLuint /* indx */,
                                              const GLint* /* values */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::VertexAttribI4ui(GLuint /* indx */,
                                              GLuint /* x */,
                                              GLuint /* y */,
                                              GLuint /* z */,
                                              GLuint /* w */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::VertexAttribI4uiv(GLuint /* indx */,
                                               const GLuint* /* values */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::VertexAttribIPointer(GLuint /* indx */,
                                                  GLint /* size */,
                                                  GLenum /* type */,
                                                  GLsizei /* stride */,
                                                  const void* /* ptr */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::VertexAttribPointer(GLuint indx,
                                                 GLint size,
                                                 GLenum type,
                                                 GLboolean normalized,
                                                 GLsizei stride,
                                                 const void* ptr) {
  evas_gl_api_->glVertexAttribPointer(indx, size, type, normalized, stride,
                                      ptr);
}

void GLES2ImplementationEfl::Viewport(GLint x,
                                      GLint y,
                                      GLsizei width,
                                      GLsizei height) {
  evas_gl_api_->glViewport(x, y, width, height);
}

void GLES2ImplementationEfl::WaitSync(GLsync /* sync */,
                                      GLbitfield /* flags */,
                                      GLuint64 /* timeout */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::BlitFramebufferCHROMIUM(GLint /* srcX0 */,
                                                     GLint /* srcY0 */,
                                                     GLint /* srcX1 */,
                                                     GLint /* srcY1 */,
                                                     GLint /* dstX0 */,
                                                     GLint /* dstY0 */,
                                                     GLint /* dstX1 */,
                                                     GLint /* dstY1 */,
                                                     GLbitfield /* mask */,
                                                     GLenum /* filter */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::RenderbufferStorageMultisampleCHROMIUM(
    GLenum /* target */,
    GLsizei /* samples */,
    GLenum /* internalformat */,
    GLsizei /* width */,
    GLsizei /* height */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::RenderbufferStorageMultisampleEXT(
    GLenum /* target */,
    GLsizei /* samples */,
    GLenum /* internalformat */,
    GLsizei /* width */,
    GLsizei /* height */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::FramebufferTexture2DMultisampleEXT(
    GLenum /* target */,
    GLenum /* attachment */,
    GLenum /* textarget */,
    GLuint /* texture */,
    GLint /* level */,
    GLsizei /* samples */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::TexStorage2DEXT(GLenum /* target */,
                                             GLsizei /* levels */,
                                             GLenum /* internalFormat */,
                                             GLsizei /* width */,
                                             GLsizei /* height */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GenQueriesEXT(GLsizei /* n */,
                                           GLuint* /* queries */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::DeleteQueriesEXT(GLsizei /* n */,
                                              const GLuint* /* queries */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::QueryCounterEXT(GLuint /* id */,
                                             GLenum /* target */) {
  NOTIMPLEMENTED();
}

GLboolean GLES2ImplementationEfl::IsQueryEXT(GLuint /* id */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::BeginQueryEXT(GLenum /* target */,
                                           GLuint /* id */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::BeginTransformFeedback(
    GLenum /* primitivemode */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::EndQueryEXT(GLenum /* target */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::EndTransformFeedback() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetQueryivEXT(GLenum /* target */,
                                           GLenum /* pname */,
                                           GLint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetQueryObjectivEXT(GLuint /* id */,
                                                 GLenum /* pname */,
                                                 GLint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetQueryObjectuivEXT(GLuint /* id */,
                                                  GLenum /* pname */,
                                                  GLuint* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetQueryObjecti64vEXT(GLuint /* id */,
                                                   GLenum /* pname */,
                                                   GLint64* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetQueryObjectui64vEXT(GLuint /* id */,
                                                    GLenum /* pname */,
                                                    GLuint64* /* params */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SetDisjointValueSyncCHROMIUM() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::InsertEventMarkerEXT(GLsizei /* length */,
                                                  const GLchar* /* marker */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::PushGroupMarkerEXT(GLsizei /* length */,
                                                const GLchar* /* marker */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::PopGroupMarkerEXT() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GenVertexArraysOES(GLsizei /* n */,
                                                GLuint* /* arrays */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::DeleteVertexArraysOES(GLsizei /* n */,
                                                   const GLuint* /* arrays */) {
  NOTIMPLEMENTED();
}

GLboolean GLES2ImplementationEfl::IsVertexArrayOES(GLuint /* array */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::BindVertexArrayOES(GLuint /* array */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SwapBuffers() {
  NOTIMPLEMENTED();
}

GLuint GLES2ImplementationEfl::GetMaxValueInBufferCHROMIUM(
    GLuint /* buffer_id */,
    GLsizei /* count */,
    GLenum /* type */,
    GLuint /* offset */) {
  NOTIMPLEMENTED();
  return 0;
}

GLboolean GLES2ImplementationEfl::EnableFeatureCHROMIUM(
    const char* /* feature */) {
  NOTIMPLEMENTED();
  return 0;
}

void* GLES2ImplementationEfl::MapBufferCHROMIUM(GLuint /* target */,
                                                GLenum /* access */) {
  NOTIMPLEMENTED();
  return 0;
}

GLboolean GLES2ImplementationEfl::UnmapBufferCHROMIUM(GLuint /* target */) {
  NOTIMPLEMENTED();
  return 0;
}

void* GLES2ImplementationEfl::MapBufferSubDataCHROMIUM(GLuint /* target */,
                                                       GLintptr /* offset */,
                                                       GLsizeiptr /* size */,
                                                       GLenum /* access */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::UnmapBufferSubDataCHROMIUM(const void* /* mem */) {
  NOTIMPLEMENTED();
}

void* GLES2ImplementationEfl::MapBufferRange(GLenum /* target */,
                                             GLintptr /* offset */,
                                             GLsizeiptr /* size */,
                                             GLbitfield /* access */) {
  NOTIMPLEMENTED();
  return 0;
}

GLboolean GLES2ImplementationEfl::UnmapBuffer(GLenum /* target */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::FlushMappedBufferRange(GLenum /* target */,
                                                    GLintptr /* offset */,
                                                    GLsizeiptr /* size */) {
  NOTIMPLEMENTED();
}

void* GLES2ImplementationEfl::MapTexSubImage2DCHROMIUM(GLenum /* target */,
                                                       GLint /* level */,
                                                       GLint /* xoffset */,
                                                       GLint /* yoffset */,
                                                       GLsizei /* width */,
                                                       GLsizei /* height */,
                                                       GLenum /* format */,
                                                       GLenum /* type */,
                                                       GLenum /* access */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::UnmapTexSubImage2DCHROMIUM(const void* /* mem */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ResizeCHROMIUM(GLuint /* width */,
                                            GLuint /* height */,
                                            GLfloat /* scale_factor */,
                                            GLenum /* color_space */,
                                            GLboolean /* alpha */) {
  NOTIMPLEMENTED();
}

const GLchar* GLES2ImplementationEfl::GetRequestableExtensionsCHROMIUM() {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::RequestExtensionCHROMIUM(
    const char* /* extension */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetProgramInfoCHROMIUM(GLuint /* program */,
                                                    GLsizei /* bufsize */,
                                                    GLsizei* /* size */,
                                                    void* /* info */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetUniformBlocksCHROMIUM(GLuint /* program */,
                                                      GLsizei /* bufsize */,
                                                      GLsizei* /* size */,
                                                      void* /* info */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetTransformFeedbackVaryingsCHROMIUM(
    GLuint /* program */,
    GLsizei /* bufsize */,
    GLsizei* /* size */,
    void* /* info */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetUniformsES3CHROMIUM(GLuint /* program */,
                                                    GLsizei /* bufsize */,
                                                    GLsizei* /* size */,
                                                    void* /* info */) {
  NOTIMPLEMENTED();
}

GLuint GLES2ImplementationEfl::CreateImageCHROMIUM(
    ClientBuffer /* buffer */,
    GLsizei /* width */,
    GLsizei /* height */,
    GLenum /* internalformat */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::DestroyImageCHROMIUM(GLuint /* image_id */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::DescheduleUntilFinishedCHROMIUM() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GetTranslatedShaderSourceANGLE(
    GLuint /* shader */,
    GLsizei /* bufsize */,
    GLsizei* /* length */,
    char* /* source */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::PostSubBufferCHROMIUM(GLint /* x */,
                                                   GLint /* y */,
                                                   GLint /* width */,
                                                   GLint /* height */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CopyTextureCHROMIUM(
    GLuint /* source_id */,
    GLint /* source_level */,
    GLenum /* dest_target */,
    GLuint /* dest_id */,
    GLint /* dest_level */,
    GLint /* internalformat */,
    GLenum /* dest_type */,
    GLboolean /* unpack_flip_y */,
    GLboolean /* unpack_premultiply_alpha */,
    GLboolean /* unpack_unmultiply_alpha */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CopySubTextureCHROMIUM(
    GLuint /* source_id */,
    GLint /* source_level */,
    GLenum /* dest_target */,
    GLuint /* dest_id */,
    GLint /* dest_level */,
    GLint /* xoffset */,
    GLint /* yoffset */,
    GLint /* x */,
    GLint /* y */,
    GLsizei /* width */,
    GLsizei /* height */,
    GLboolean /* unpack_flip_y */,
    GLboolean /* unpack_premultiply_alpha */,
    GLboolean /* unpack_unmultiply_alpha */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CompressedCopyTextureCHROMIUM(
    GLuint /* source_id */,
    GLuint /* dest_id */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::DrawArraysInstancedANGLE(GLenum /* mode */,
                                                      GLint /* first */,
                                                      GLsizei /* count */,
                                                      GLsizei /* primcount */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::DrawElementsInstancedANGLE(
    GLenum /* mode */,
    GLsizei /* count */,
    GLenum /* type */,
    const void* /* indices */,
    GLsizei /* primcount */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::VertexAttribDivisorANGLE(GLuint /* index */,
                                                      GLuint /* divisor */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GenMailboxCHROMIUM(GLbyte* /* mailbox */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ProduceTextureCHROMIUM(
    GLenum /* target */,
    const GLbyte* /* mailbox */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ProduceTextureDirectCHROMIUM(
    GLuint /* texture */,
    GLenum /* target */,
    const GLbyte* /* mailbox */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ConsumeTextureCHROMIUM(GLenum target,
                                                    const GLbyte* mailbox) {
  DoConsumeTextureCHROMIUM(target, mailbox);
}

GLuint GLES2ImplementationEfl::CreateAndConsumeTextureCHROMIUM(
    GLenum target,
    const GLbyte* mailbox) {
  return DoCreateAndConsumeTextureCHROMIUM(target, mailbox);
}

void GLES2ImplementationEfl::BindUniformLocationCHROMIUM(GLuint program,
                                                         GLint location,
                                                         const char* name) {
  DoBindUniformLocationCHROMIUM(program, location, name);
}

void GLES2ImplementationEfl::BindTexImage2DCHROMIUM(GLenum /* target */,
                                                    GLint /* imageId */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::BindTexImage2DWithInternalformatCHROMIUM(
    GLenum /* target */,
    GLenum /* internalformat */,
    GLint /* imageId */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ReleaseTexImage2DCHROMIUM(GLenum /* target */,
                                                       GLint /* imageId */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::TraceBeginCHROMIUM(const char* /* category_name */,
                                                const char* /* trace_name */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::TraceEndCHROMIUM() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::DiscardFramebufferEXT(
    GLenum /* target */,
    GLsizei /* count */,
    const GLenum* /* attachments */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::LoseContextCHROMIUM(GLenum /* current */,
                                                 GLenum /* other */) {
  NOTIMPLEMENTED();
}

GLuint64 GLES2ImplementationEfl::InsertFenceSyncCHROMIUM() {
  return DoInsertFenceSyncCHROMIUM();
}

void GLES2ImplementationEfl::GenSyncTokenCHROMIUM(GLuint64 /* fence_sync */,
                                                  GLbyte* /* sync_token */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::GenUnverifiedSyncTokenCHROMIUM(
    GLuint64 /* fence_sync */,
    GLbyte* /* sync_token */) {}

void GLES2ImplementationEfl::VerifySyncTokensCHROMIUM(
    GLbyte** /* sync_tokens */,
    GLsizei /* count */) {}

void GLES2ImplementationEfl::WaitSyncTokenCHROMIUM(const GLbyte* sync_token) {
  DoWaitSyncTokenCHROMIUM(sync_token);
}

void GLES2ImplementationEfl::DrawBuffersEXT(GLsizei /* count */,
                                            const GLenum* /* bufs */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::DiscardBackbufferCHROMIUM() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ScheduleOverlayPlaneCHROMIUM(
    GLint /* plane_z_order */,
    GLenum /* plane_transform */,
    GLuint /* overlay_texture_id */,
    GLint /* bounds_x */,
    GLint /* bounds_y */,
    GLint /* bounds_width */,
    GLint /* bounds_height */,
    GLfloat /* uv_x */,
    GLfloat /* uv_y */,
    GLfloat /* uv_width */,
    GLfloat /* uv_height */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ScheduleCALayerSharedStateCHROMIUM(
    GLfloat /* opacity */,
    GLboolean /* is_clipped */,
    const GLfloat* /* clip_rect */,
    GLint /* sorting_context_id */,
    const GLfloat* /* transform */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ScheduleCALayerCHROMIUM(
    GLuint /* contents_texture_id */,
    const GLfloat* /* contents_rect */,
    GLuint /* background_color */,
    GLuint /* edge_aa_mask */,
    const GLfloat* /* bounds_rect */,
    GLuint /* filter */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SetColorSpaceForScanoutCHROMIUM(
    GLuint /* texture_id */,
    GLColorSpace /* color_space */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ScheduleCALayerInUseQueryCHROMIUM(
    GLsizei /* count */,
    const GLuint* /* textures */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CommitOverlayPlanesCHROMIUM() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SwapInterval(GLint /* interval */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::FlushDriverCachesCHROMIUM() {
  NOTIMPLEMENTED();
}

GLuint GLES2ImplementationEfl::GetLastFlushIdCHROMIUM() {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::ScheduleDCLayerSharedStateCHROMIUM(
    GLfloat /* opacity */,
    GLboolean /* is_clipped */,
    const GLfloat* /* clip_rect */,
    GLint /* z_order */,
    const GLfloat* /* transform */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ScheduleDCLayerCHROMIUM(
    GLsizei /* num_textures */,
    const GLuint* /* contents_texture_ids */,
    const GLfloat* /* contents_rect */,
    GLuint /* background_color */,
    GLuint /* edge_aa_mask */,
    const GLfloat* /* bounds_rect */,
    GLuint /* filter */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::MatrixLoadfCHROMIUM(GLenum /* matrixMode */,
                                                 const GLfloat* /* m */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::MatrixLoadIdentityCHROMIUM(
    GLenum /* matrixMode */) {
  NOTIMPLEMENTED();
}

GLuint GLES2ImplementationEfl::GenPathsCHROMIUM(GLsizei /* range */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::DeletePathsCHROMIUM(GLuint /* path */,
                                                 GLsizei /* range */) {
  NOTIMPLEMENTED();
}

GLboolean GLES2ImplementationEfl::IsPathCHROMIUM(GLuint /* path */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::PathCommandsCHROMIUM(GLuint /* path */,
                                                  GLsizei /* numCommands */,
                                                  const GLubyte* /* commands */,
                                                  GLsizei /* numCoords */,
                                                  GLenum /* coordType */,
                                                  const GLvoid* /* coords */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::PathParameterfCHROMIUM(GLuint /* path */,
                                                    GLenum /* pname */,
                                                    GLfloat /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::PathParameteriCHROMIUM(GLuint /* path */,
                                                    GLenum /* pname */,
                                                    GLint /* value */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::PathStencilFuncCHROMIUM(GLenum /* func */,
                                                     GLint /* ref */,
                                                     GLuint /* mask */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::StencilFillPathCHROMIUM(GLuint /* path */,
                                                     GLenum /* fillMode */,
                                                     GLuint /* mask */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::StencilStrokePathCHROMIUM(GLuint /* path */,
                                                       GLint /* reference */,
                                                       GLuint /* mask */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CoverFillPathCHROMIUM(GLuint /* path */,
                                                   GLenum /* coverMode */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CoverStrokePathCHROMIUM(GLuint /* path */,
                                                     GLenum /* coverMode */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::StencilThenCoverFillPathCHROMIUM(
    GLuint /* path */,
    GLenum /* fillMode */,
    GLuint /* mask */,
    GLenum /* coverMode */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::StencilThenCoverStrokePathCHROMIUM(
    GLuint /* path */,
    GLint /* reference */,
    GLuint /* mask */,
    GLenum /* coverMode */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::StencilFillPathInstancedCHROMIUM(
    GLsizei /* numPaths */,
    GLenum /* pathNameType */,
    const GLvoid* /* paths */,
    GLuint /* pathBase */,
    GLenum /* fillMode */,
    GLuint /* mask */,
    GLenum /* transformType */,
    const GLfloat* /* transformValues */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::StencilStrokePathInstancedCHROMIUM(
    GLsizei /* numPaths */,
    GLenum /* pathNameType */,
    const GLvoid* /* paths */,
    GLuint /* pathBase */,
    GLint /* reference */,
    GLuint /* mask */,
    GLenum /* transformType */,
    const GLfloat* /* transformValues */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CoverFillPathInstancedCHROMIUM(
    GLsizei /* numPaths */,
    GLenum /* pathNameType */,
    const GLvoid* /* paths */,
    GLuint /* pathBase */,
    GLenum /* coverMode */,
    GLenum /* transformType */,
    const GLfloat* /* transformValues */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::CoverStrokePathInstancedCHROMIUM(
    GLsizei /* numPaths */,
    GLenum /* pathNameType */,
    const GLvoid* /* paths */,
    GLuint /* pathBase */,
    GLenum /* coverMode */,
    GLenum /* transformType */,
    const GLfloat* /* transformValues */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::StencilThenCoverFillPathInstancedCHROMIUM(
    GLsizei /* numPaths */,
    GLenum /* pathNameType */,
    const GLvoid* /* paths */,
    GLuint /* pathBase */,
    GLenum /* fillMode */,
    GLuint /* mask */,
    GLenum /* coverMode */,
    GLenum /* transformType */,
    const GLfloat* /* transformValues */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::StencilThenCoverStrokePathInstancedCHROMIUM(
    GLsizei /* numPaths */,
    GLenum /* pathNameType */,
    const GLvoid* /* paths */,
    GLuint /* pathBase */,
    GLint /* reference */,
    GLuint /* mask */,
    GLenum /* coverMode */,
    GLenum /* transformType */,
    const GLfloat* /* transformValues */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::BindFragmentInputLocationCHROMIUM(
    GLuint /* program */,
    GLint /* location */,
    const char* /* name */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ProgramPathFragmentInputGenCHROMIUM(
    GLuint /* program */,
    GLint /* location */,
    GLenum /* genMode */,
    GLint /* components */,
    const GLfloat* /* coeffs */) {
  NOTIMPLEMENTED();
}

void* GLES2ImplementationEfl::GetBufferSubDataAsyncCHROMIUM(
    GLenum /* target */,
    GLintptr /* offset */,
    GLsizeiptr /* size */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::CoverageModulationCHROMIUM(
    GLenum /* components */) {
  NOTIMPLEMENTED();
}

GLenum GLES2ImplementationEfl::GetGraphicsResetStatusKHR() {
  return 0;
}

void GLES2ImplementationEfl::BlendBarrierKHR() {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::ApplyScreenSpaceAntialiasingCHROMIUM() {
  NOTIMPLEMENTED();
}

GLuint GLES2ImplementationEfl::CreateTizenImageCHROMIUM(
    gfx::TbmBufferHandle /* handle */,
    GLsizei /* width */,
    GLsizei /* height */,
    GLenum /* internalformat */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::DestroyTizenImageCHROMIUM(GLuint /* image_id */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::BindFragDataLocationIndexedEXT(
    GLuint /* program */,
    GLuint /* colorNumber */,
    GLuint /* index */,
    const char* /* name */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::BindFragDataLocationEXT(GLuint /* program */,
                                                     GLuint /* colorNumber */,
                                                     const char* /* name */) {
  NOTIMPLEMENTED();
}

GLint GLES2ImplementationEfl::GetFragDataIndexEXT(GLuint /* program */,
                                                  const char* /* name */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::UniformMatrix4fvStreamTextureMatrixCHROMIUM(
    GLint /* location */,
    GLboolean /* transpose */,
    const GLfloat* /* transform */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::OverlayPromotionHintCHROMIUM(
    GLuint /* texture */,
    GLboolean /* promotion_hint */,
    GLint /* display_x */,
    GLint /* display_y */,
    GLint /* display_width */,
    GLint /* display_height */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SwapBuffersWithBoundsCHROMIUM(
    GLsizei /* count */,
    const GLint* /* rects */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SetDrawRectangleCHROMIUM(GLint /* x */,
                                                      GLint /* y */,
                                                      GLint /* width */,
                                                      GLint /* height */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::SetEnableDCLayersCHROMIUM(
    GLboolean /* enabled */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::InitializeDiscardableTextureCHROMIUM(
    GLuint /* texture_id */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::UnlockDiscardableTextureCHROMIUM(
    GLuint /* texture_id */) {
  NOTIMPLEMENTED();
}

bool GLES2ImplementationEfl::LockDiscardableTextureCHROMIUM(
    GLuint /* texture_id */) {
  NOTIMPLEMENTED();
  return 0;
}

void GLES2ImplementationEfl::BeginRasterCHROMIUM(
    GLuint /* texture_id */,
    GLuint /* sk_color */,
    GLuint /* msaa_sample_count */,
    GLboolean /* can_use_lcd_text */,
    GLboolean /* use_distance_field_text */,
    GLint /* pixel_config */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::RasterCHROMIUM(
    const cc::DisplayItemList* /* list */,
    GLint /* x */,
    GLint /* y */,
    GLint /* w */,
    GLint /* h */) {
  NOTIMPLEMENTED();
}

void GLES2ImplementationEfl::EndRasterCHROMIUM() {
  NOTIMPLEMENTED();
}

#endif  // GPU_COMMAND_BUFFER_CLIENT_GLES2_IMPLEMENTATION_EFL_AUTOGEN_H_
