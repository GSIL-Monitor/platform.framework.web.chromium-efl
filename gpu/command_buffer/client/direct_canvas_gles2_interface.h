// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_DIRECT_CANVAS_GLES2_INTERFACE_H_
#define GPU_COMMAND_BUFFER_CLIENT_DIRECT_CANVAS_GLES2_INTERFACE_H_

#include "gpu/command_buffer/client/gles2_interface_stub.h"

namespace gpu {
namespace gles2 {

class DirectCanvasGLES2Interface : public GLES2InterfaceStub {
 public:
  DirectCanvasGLES2Interface(GLES2Interface* gl) : gl_(gl) {}
  ~DirectCanvasGLES2Interface() override {}

  void BindFramebuffer(GLenum target, GLuint framebuffer) override;

  GLenum CheckFramebufferStatus(GLenum target) override;

  // Override Renderbuffer related methods to clear depth and stencil bit for
  // the native surface.
  void GenRenderbuffers(GLsizei n, GLuint* renderbuffers) override;

  void GetIntegerv(GLenum ptype, GLint* value) override;

  GLuint64 InsertFenceSyncCHROMIUM() override;

  void GenSyncTokenCHROMIUM(GLuint64 fence_sync, GLbyte* sync_token) override;

  void Clear(GLbitfield mask) override;

  void Flush() override;

  void SwapBuffers() override;

  void TexImage2D(GLenum target,
                  GLint level,
                  GLint internalformat,
                  GLsizei width,
                  GLsizei height,
                  GLint border,
                  GLenum format,
                  GLenum type,
                  const void* pixels) override;

 private:
  GLES2Interface* gl_;
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_DIRECT_CANVAS_GLES2_INTERFACE_H_
