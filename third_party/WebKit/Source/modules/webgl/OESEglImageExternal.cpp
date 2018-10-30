/*
 * Copyright 2017 Samsung Electronics Inc. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "modules/webgl/OESEglImageExternal.h"

#include "core/html/HTMLVideoElement.h"

namespace blink {

OESEglImageExternal::OESEglImageExternal(WebGLRenderingContextBase* context)
    : WebGLExtension(context) {
  context->ExtensionsUtil()->EnsureExtensionEnabled(
      "GL_OES_EGL_image_external");
}

WebGLExtensionName OESEglImageExternal::GetName() const {
  return kOESEglImageExternalName;
}

OESEglImageExternal* OESEglImageExternal::Create(
    WebGLRenderingContextBase* context) {
  return new OESEglImageExternal(context);
}

bool OESEglImageExternal::Supported(WebGLRenderingContextBase* context) {
  return context->ExtensionsUtil()->SupportsExtension(
      "GL_OES_EGL_image_external");
}

void OESEglImageExternal::EGLImageTargetTexture2DOES(
    ExecutionContext* execution_context,
    GLenum target,
    HTMLVideoElement* video,
    ExceptionState& exceptionState) {
  WebGLExtensionScopedContext scoped(this);
  if (scoped.IsLost())
    return;

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  if (video)
    scoped.Context()->texImage2D(execution_context, target, 0, GL_RGB, GL_RGB,
                                 GL_UNSIGNED_BYTE, video, exceptionState);
#endif
}

const char* OESEglImageExternal::ExtensionName() {
  return "OES_EGL_image_external";
}

}  // namespace blink
