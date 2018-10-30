// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_GLES2_IMPLEMENTATION_EFL_H_
#define GPU_COMMAND_BUFFER_CLIENT_GLES2_IMPLEMENTATION_EFL_H_

#include <Evas_GL.h>

#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_impl_export.h"
#include "gpu/command_buffer/client/gles2_interface.h"

#include <map>
#include <set>

#if defined(TIZEN_TBM_SUPPORT)
#include <tbm_surface.h>
#endif

namespace gpu {
namespace gles2 {

class GLES2_IMPL_EXPORT GLES2ImplementationEfl : public GLES2Interface,
                                                 public ContextSupport {
 public:
  GLES2ImplementationEfl(Evas_GL_API* evas_gl_api, Evas_GL* evas_gl);
  ~GLES2ImplementationEfl() override;

// Include the auto-generated part of this class. We split this because
// it means we can easily edit the non-auto generated parts right here in
// this file instead of having to edit some template or the code generator.
#include "gpu/command_buffer/client/gles2_implementation_efl_header_autogen.h"

  // gpu::ContextSupport implementation.
  void FlushPendingWork() override {}
  void SignalSyncToken(const gpu::SyncToken& sync_token,
                       const base::Closure& callback) override {}
  bool IsSyncTokenSignaled(const gpu::SyncToken& sync_token) override;
  void SignalQuery(uint32_t query, const base::Closure& callback) override {}
  void SetAggressivelyFreeResources(bool aggressively_free_resources) override {
  }
  void Swap() override {}
  void SwapWithBounds(const std::vector<gfx::Rect>& rects) override {}
  void PartialSwapBuffers(const gfx::Rect& sub_buffer) override {}
  void CommitOverlayPlanes() override {}
  void ScheduleOverlayPlane(int plane_z_order,
                            gfx::OverlayTransform plane_transform,
                            unsigned overlay_texture_id,
                            const gfx::Rect& display_bounds,
                            const gfx::RectF& uv_rect) override {}
  uint64_t ShareGroupTracingGUID() const override;
  void SetErrorMessageCallback(
      const base::Callback<void(const char*, int32_t)>& callback) override {}
  void AddLatencyInfo(
      const std::vector<ui::LatencyInfo>& latency_info) override {}
  bool ThreadSafeShallowLockDiscardableTexture(uint32_t texture_id) override;
  void CompleteLockDiscardableTexureOnContextThread(
      uint32_t texture_id) override {}

  GLuint DoCreateProgram();
  void DoUseProgram(GLuint program);
  void DoDeleteProgram(GLuint program);
  void DoUniform1f(GLint location, GLfloat x);
  void DoUniform1fv(GLint location, GLsizei count, const GLfloat* v);
  void DoUniform1i(GLint location, GLint x);
  void DoUniform1iv(GLint location, GLsizei count, const GLint* v);
  void DoUniform2f(GLint location, GLfloat x, GLfloat y);
  void DoUniform2fv(GLint location, GLsizei count, const GLfloat* v);
  void DoUniform2i(GLint location, GLint x, GLint y);
  void DoUniform2iv(GLint location, GLsizei count, const GLint* v);
  void DoUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z);
  void DoUniform3fv(GLint location, GLsizei count, const GLfloat* v);
  void DoUniform3i(GLint location, GLint x, GLint y, GLint z);
  void DoUniform3iv(GLint location, GLsizei count, const GLint* v);
  void DoUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void DoUniform4fv(GLint location, GLsizei count, const GLfloat* v);
  void DoUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w);
  void DoUniform4iv(GLint location, GLsizei count, const GLint* v);
  void DoUniformMatrix2fv(GLint location,
                          GLsizei count,
                          GLboolean transpose,
                          const GLfloat* value);
  void DoUniformMatrix3fv(GLint location,
                          GLsizei count,
                          GLboolean transpose,
                          const GLfloat* value);
  void DoUniformMatrix4fv(GLint location,
                          GLsizei count,
                          GLboolean transpose,
                          const GLfloat* value);
  void DoBindUniformLocationCHROMIUM(GLuint program,
                                     GLint location,
                                     const char* name);

  void DoGenTextures(GLsizei n, GLuint* textures);
  void DoConsumeTextureCHROMIUM(GLenum target, const GLbyte* data);
  GLuint DoCreateAndConsumeTextureCHROMIUM(GLenum target, const GLbyte* data);
  void DoBindTexture(GLenum target, GLuint texture);
  void DoDeleteTextures(GLsizei n, const GLuint* textures);

  GLuint DoInsertFenceSyncCHROMIUM();
  void DoWaitSyncTokenCHROMIUM(const GLbyte* sync_token);

#if defined(TIZEN_TBM_SUPPORT)
  EvasGLImage CreateImage(tbm_surface_h surface);
  void DestroyImage(EvasGLImage image);
  void ImageTargetTexture2DOES(EvasGLImage image);
#endif

 private:
  Evas_GL_API* evas_gl_api_;
  Evas_GL* evas_gl_;

  typedef struct {
    GLint service_id_;
    typedef std::map<GLint, std::string> UniformLocationMap;
    UniformLocationMap uniform_location_map_;
  } ProgramData;

  GLint current_program_;

  typedef std::map<GLint, ProgramData> ProgramMap;
  ProgramMap program_map_;

  GLuint bound_texture_id_;

  typedef std::map<GLuint, GLuint> TextureConsumedMap;
  TextureConsumedMap texture_consumed_map_;

  typedef std::map<GLenum, GLuint> TextureBoundMap;
  TextureBoundMap texture_bound_map_;

  std::set<GLuint> textures_;

  GLint GetBoundUniformLocation(GLint location);
  ProgramData& GetProgramData(GLint program = -1);
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_GLES2_IMPLEMENTATION_EFL_H_
