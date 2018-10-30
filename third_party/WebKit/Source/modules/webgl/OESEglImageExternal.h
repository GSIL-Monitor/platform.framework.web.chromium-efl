/*
 * Copyright 2017 Samsung Electronics Inc. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef OESEglImageExternal_h
#define OESEglImageExternal_h

#include "modules/webgl/WebGLExtension.h"

namespace blink {

class HTMLVideoElement;

class OESEglImageExternal final : public WebGLExtension {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static OESEglImageExternal* Create(WebGLRenderingContextBase*);
  static bool Supported(WebGLRenderingContextBase*);
  static const char* ExtensionName();

  WebGLExtensionName GetName() const override;
  void EGLImageTargetTexture2DOES(ExecutionContext*,
                                  GLenum,
                                  HTMLVideoElement*,
                                  ExceptionState&);

 private:
  explicit OESEglImageExternal(WebGLRenderingContextBase*);
};

}  // namespace blink

#endif  // OESEglImageExternal_h
