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
#ifndef MIRRORED_BLUR_EFFECT_H
#define MIRRORED_BLUR_EFFECT_H

#include <Ecore_Evas.h>
#include "content/browser/compositor/evasgl_delegated_frame_host.h"
#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT MirroredBlurEffect {
 public:
  // TODO(mallikarjun82): remove static keyword.
  static std::unique_ptr<MirroredBlurEffect> Create(Evas_GL_API* evas_gl_api) {
    return std::unique_ptr<MirroredBlurEffect>(
        new MirroredBlurEffect(evas_gl_api));
  };

  // TODO(mallikarjun82): name the members uniform.
  MirroredBlurEffect(Evas_GL_API*);
  static void InitMatrix(float*);
  static void MultiplyMatrix(float*, const float*, const float*);
  int InitBlurShaders();
  void InitShaderForEffect(int, int);
  void DrawBlur(int, int);
  void DrawToSurface(int, int);
  void InitializeShaders();
  void InitializeFBO(int, int);
  void InitializeForEffect();
  void PaintToBackupTextureEffect();
  void ClearBlurEffectResources();
  GLuint GetFBO() { return fbo_; }
  GLuint LoadAndCompileShaders(const char*, const char*);

 private:
  Evas_GL_API* evas_gl_api_;
  int m_initShaderAndFBO;
  GLuint fbo_texture_;
  GLuint fbo_depth_;
  GLuint fbo_;
  GLuint program_;
  GLuint blur_program;
  GLuint mvp_matrix_id;
  GLuint texture_id;
  GLfloat bias_;
  GLuint blur_mvp_matrix_id;
  GLuint blur_texture_id;
  GLuint position_attrib_;
  GLuint texture_attrib_;
  GLuint blur_position_attrib_;
  GLuint blur_texture_attrib_;
  GLuint index_buffer_obj_;
};
}  // namespace content
#endif
