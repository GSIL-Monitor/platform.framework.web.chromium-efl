// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/gles2_implementation_efl.h"

#include "base/logging.h"
#include "gpu/command_buffer/client/shared_mailbox_manager.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "ui/gl/gl_shared_context_efl.h"

#if defined(TIZEN_TBM_SUPPORT)
#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifndef EGL_NATIVE_SURFACE_TIZEN
#define EGL_NATIVE_SURFACE_TIZEN 0x32A1
#endif
#endif

// Defined in gl_current_context_efl.cc because of conflicts of
// texture_manager.h with efl GL API wrappers.
namespace content {
extern GLuint GetTextureIdFromTexture(gpu::gles2::TextureBase* texture);
}

namespace gpu {
namespace gles2 {

// The size of provisional textures array.
static const unsigned kMaxTextureId = 1024;

GLES2ImplementationEfl::GLES2ImplementationEfl(Evas_GL_API* evas_gl_api,
                                               Evas_GL* evas_gl)
    : evas_gl_api_(evas_gl_api),
      evas_gl_(evas_gl),
      current_program_(0),
      bound_texture_id_(0) {}

GLES2ImplementationEfl::~GLES2ImplementationEfl() {}

// Include the auto-generated part of this class. We split this because
// it means we can easily edit the non-auto generated parts right here in
// this file instead of having to edit some template or the code generator.
#include "gpu/command_buffer/client/gles2_implementation_efl_autogen.h"

bool GLES2ImplementationEfl::IsSyncTokenSignaled(
    const gpu::SyncToken& sync_token) {
  // TODO: Check if this has to return false.
  return true;
}

uint64_t GLES2ImplementationEfl::ShareGroupTracingGUID() const {
  // This method should return the command buffer id.
  // However, evasgl renderer does not use command buffer.
  // Hence, we return 0.
  return 0;
}

bool GLES2ImplementationEfl::ThreadSafeShallowLockDiscardableTexture(
    uint32_t texture_id) {
  return false;
}

GLES2ImplementationEfl::ProgramData& GLES2ImplementationEfl::GetProgramData(
    GLint program) {
  if (program == -1)
    program = current_program_;
  auto it_program = program_map_.find(program);
  DCHECK(it_program != program_map_.end());
  return it_program->second;
}

GLuint GLES2ImplementationEfl::DoCreateProgram() {
  GLuint program = evas_gl_api_->glCreateProgram();
  program_map_[program] = ProgramData();
  auto it_program = program_map_.find(program);
  DCHECK(it_program != program_map_.end());
  it_program->second.service_id_ = program;
  return program;
}

void GLES2ImplementationEfl::DoUseProgram(GLuint program) {
  evas_gl_api_->glUseProgram(program);
  evas_gl_api_->glGetIntegerv(GL_CURRENT_PROGRAM, &current_program_);
}

void GLES2ImplementationEfl::DoDeleteProgram(GLuint program) {
  program_map_.erase(program);
  evas_gl_api_->glDeleteProgram(program);
}

GLint GLES2ImplementationEfl::GetBoundUniformLocation(GLint location) {
  const ProgramData& program_data = GetProgramData();

  auto it_location = program_data.uniform_location_map_.find(location);
  // DoBindUniformLocationCHROMIUM API is not used by Skia.
  // So this method might fail when we use Skia GL on browser side.
  if (it_location == program_data.uniform_location_map_.end()) {
    LOG(ERROR) << __FUNCTION__ << " location not found.";
    return location;
  }
  const std::string& name = it_location->second;

  GLint newloc = evas_gl_api_->glGetUniformLocation(program_data.service_id_,
                                                    name.c_str());
  return newloc;
}

void GLES2ImplementationEfl::DoBindUniformLocationCHROMIUM(GLuint program,
                                                           GLint location,
                                                           const char* name) {
  DCHECK(program);

  ProgramData& program_data = GetProgramData(program);
  program_data.uniform_location_map_[location] = name;
}

void GLES2ImplementationEfl::DoUniform1f(GLint location, GLfloat x) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform1f(location, x);
}

void GLES2ImplementationEfl::DoUniform1fv(GLint location,
                                          GLsizei count,
                                          const GLfloat* v) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform1fv(location, count, v);
}

void GLES2ImplementationEfl::DoUniform1i(GLint location, GLint x) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform1i(location, x);
}

void GLES2ImplementationEfl::DoUniform1iv(GLint location,
                                          GLsizei count,
                                          const GLint* v) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform1iv(location, count, v);
}

void GLES2ImplementationEfl::DoUniform2f(GLint location, GLfloat x, GLfloat y) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform2f(location, x, y);
}

void GLES2ImplementationEfl::DoUniform2fv(GLint location,
                                          GLsizei count,
                                          const GLfloat* v) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform2fv(location, count, v);
}

void GLES2ImplementationEfl::DoUniform2i(GLint location, GLint x, GLint y) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform2i(location, x, y);
}

void GLES2ImplementationEfl::DoUniform2iv(GLint location,
                                          GLsizei count,
                                          const GLint* v) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform2iv(location, count, v);
}

void GLES2ImplementationEfl::DoUniform3f(GLint location,
                                         GLfloat x,
                                         GLfloat y,
                                         GLfloat z) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform3f(location, x, y, z);
}

void GLES2ImplementationEfl::DoUniform3fv(GLint location,
                                          GLsizei count,
                                          const GLfloat* v) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform3fv(location, count, v);
}

void GLES2ImplementationEfl::DoUniform3i(GLint location,
                                         GLint x,
                                         GLint y,
                                         GLint z) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform3i(location, x, y, z);
}

void GLES2ImplementationEfl::DoUniform3iv(GLint location,
                                          GLsizei count,
                                          const GLint* v) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform3iv(location, count, v);
}

void GLES2ImplementationEfl::DoUniform4f(GLint location,
                                         GLfloat x,
                                         GLfloat y,
                                         GLfloat z,
                                         GLfloat w) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform4f(location, x, y, z, w);
}

void GLES2ImplementationEfl::DoUniform4fv(GLint location,
                                          GLsizei count,
                                          const GLfloat* v) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform4fv(location, count, v);
}

void GLES2ImplementationEfl::DoUniform4i(GLint location,
                                         GLint x,
                                         GLint y,
                                         GLint z,
                                         GLint w) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform4i(location, x, y, z, w);
}

void GLES2ImplementationEfl::DoUniform4iv(GLint location,
                                          GLsizei count,
                                          const GLint* v) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniform4iv(location, count, v);
}

void GLES2ImplementationEfl::DoUniformMatrix2fv(GLint location,
                                                GLsizei count,
                                                GLboolean transpose,
                                                const GLfloat* value) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniformMatrix2fv(location, count, transpose, value);
}

void GLES2ImplementationEfl::DoUniformMatrix3fv(GLint location,
                                                GLsizei count,
                                                GLboolean transpose,
                                                const GLfloat* value) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniformMatrix3fv(location, count, transpose, value);
}

void GLES2ImplementationEfl::DoUniformMatrix4fv(GLint location,
                                                GLsizei count,
                                                GLboolean transpose,
                                                const GLfloat* value) {
  location = GetBoundUniformLocation(location);
  evas_gl_api_->glUniformMatrix4fv(location, count, transpose, value);
}

void GLES2ImplementationEfl::DoGenTextures(GLsizei n, GLuint* textures) {
  GLuint texture_ids[kMaxTextureId];
  size_t index = 0;

  for (index = 0; index < kMaxTextureId; index++) {
    evas_gl_api_->glGenTextures(1, &texture_ids[index]);
    if (!texture_consumed_map_.count(texture_ids[index]))
      break;
  }

  // Delete provisionally generated but already consumed textures.
  for (size_t i = 0; i < index; i++)
    evas_gl_api_->glDeleteTextures(1, &texture_ids[i]);

  textures_.insert(texture_ids[index]);
  textures[0] = texture_ids[index];
}

void GLES2ImplementationEfl::DoConsumeTextureCHROMIUM(GLenum target,
                                                      const GLbyte* data) {
  DCHECK(target == GL_TEXTURE_2D);
  const Mailbox& mailbox = *reinterpret_cast<const Mailbox*>(data);
  DCHECK(mailbox.Verify());

  TextureBase* texture =
      SharedMailboxManager::GetMailboxManager()->ConsumeTexture(mailbox);
  if (!texture) {
    LOG(ERROR) << __FUNCTION__ << " invalid mailbox name";
    return;
  }

  GLuint bound_texture_id = ++bound_texture_id_;

  while (textures_.count(bound_texture_id))
    bound_texture_id = ++bound_texture_id_;

  GLuint texid = ::content::GetTextureIdFromTexture(texture);
  texture_consumed_map_[bound_texture_id] = texid;
}

GLuint GLES2ImplementationEfl::DoCreateAndConsumeTextureCHROMIUM(
    GLenum target,
    const GLbyte* data) {
  DCHECK(target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES);
  const Mailbox& mailbox = *reinterpret_cast<const Mailbox*>(data);
  DCHECK(mailbox.Verify());

  TextureBase* texture =
      SharedMailboxManager::GetMailboxManager()->ConsumeTexture(mailbox);
  if (!texture) {
    LOG(ERROR) << __FUNCTION__ << " invalid mailbox name";
    return bound_texture_id_;
  }

  GLuint bound_texture_id = ++bound_texture_id_;

  while (textures_.count(bound_texture_id))
    bound_texture_id = ++bound_texture_id_;

  GLuint texid = ::content::GetTextureIdFromTexture(texture);
  texture_consumed_map_[bound_texture_id] = texid;
  return bound_texture_id;
}

void GLES2ImplementationEfl::DoBindTexture(GLenum target, GLuint texture) {
  DCHECK(target == GL_TEXTURE_2D || target == GL_TEXTURE_EXTERNAL_OES);
  auto it_texture_consumed = texture_consumed_map_.find(texture);
  if (it_texture_consumed != texture_consumed_map_.end())
    evas_gl_api_->glBindTexture(target, it_texture_consumed->second);
  else if (textures_.count(texture))
    evas_gl_api_->glBindTexture(target, texture);
  texture_bound_map_[target] = texture;
}

void GLES2ImplementationEfl::DoDeleteTextures(GLsizei n,
                                              const GLuint* textures) {
  auto it_texture_consumed = texture_consumed_map_.find(*textures);
  if (it_texture_consumed != texture_consumed_map_.end()) {
    for (auto it = texture_bound_map_.begin();
         it != texture_bound_map_.end();) {
      if (it->second == *textures) {
        evas_gl_api_->glBindTexture(it->first, 0);
        it = texture_bound_map_.erase(it);
      } else {
        ++it;
      }
    }
    texture_consumed_map_.erase(it_texture_consumed);
  } else {
    textures_.erase(textures[0]);
    evas_gl_api_->glDeleteTextures(n, textures);
  }
}

GLuint GLES2ImplementationEfl::DoInsertFenceSyncCHROMIUM() {
  return 1;
}

void GLES2ImplementationEfl::DoWaitSyncTokenCHROMIUM(
    const GLbyte* /* sync_token */) {}

#if defined(TIZEN_TBM_SUPPORT)
EvasGLImage GLES2ImplementationEfl::CreateImage(tbm_surface_h surface) {
  const int attrs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
  Evas_GL_Context* evas_context = evas_gl_current_context_get(evas_gl_);
  return evas_gl_api_->evasglCreateImageForContext(
      evas_gl_, evas_context, EGL_NATIVE_SURFACE_TIZEN, surface, attrs);
}

void GLES2ImplementationEfl::DestroyImage(EvasGLImage image) {
  evas_gl_api_->evasglDestroyImage(image);
}

void GLES2ImplementationEfl::ImageTargetTexture2DOES(EvasGLImage image) {
  evas_gl_api_->glEvasGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
}
#endif
}  // namespace gles2
}  // namespace gpu
