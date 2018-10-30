// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/direct_canvas_gles2_interface.h"

namespace gpu {
namespace gles2 {

void DirectCanvasGLES2Interface::BindFramebuffer(GLenum target,
                                                 GLuint framebuffer) {
  gl_->BindFramebuffer(target, framebuffer);
}

GLenum DirectCanvasGLES2Interface::CheckFramebufferStatus(GLenum target) {
  return GL_FRAMEBUFFER_COMPLETE;
}

void DirectCanvasGLES2Interface::GenRenderbuffers(GLsizei n,
                                                  GLuint* renderbuffers) {
  for (GLsizei i = 0; i < n; ++i)
    renderbuffers[i] = 1;
}

void DirectCanvasGLES2Interface::GetIntegerv(GLenum ptype, GLint* value) {
  gl_->GetIntegerv(ptype, value);
}

GLuint64 DirectCanvasGLES2Interface::InsertFenceSyncCHROMIUM() {
  return gl_->InsertFenceSyncCHROMIUM();
}

void DirectCanvasGLES2Interface::GenSyncTokenCHROMIUM(GLuint64 fence_sync,
                                                      GLbyte* sync_token) {
  gl_->GenSyncTokenCHROMIUM(fence_sync, sync_token);
}

void DirectCanvasGLES2Interface::Clear(GLbitfield mask) {
  gl_->Clear(mask);
}

void DirectCanvasGLES2Interface::Flush() {
  gl_->Flush();
}

void DirectCanvasGLES2Interface::SwapBuffers() {
  gl_->SwapBuffers();
}

void DirectCanvasGLES2Interface::TexImage2D(GLenum target,
                                            GLint level,
                                            GLint internalformat,
                                            GLsizei width,
                                            GLsizei height,
                                            GLint border,
                                            GLenum format,
                                            GLenum type,
                                            const void* pixels) {
  gl_->TexImage2D(0, level, internalformat, width, height, border, format, type,
                  pixels);
}

}  // namespace gles2
}  // namespace gpu
