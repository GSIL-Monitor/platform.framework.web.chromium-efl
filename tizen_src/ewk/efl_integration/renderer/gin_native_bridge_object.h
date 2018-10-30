// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_BRIDGE_OBJECT_H_
#define EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_BRIDGE_OBJECT_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "gin/handle.h"
#include "gin/interceptor.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "renderer/gin_native_bridge_dispatcher.h"
#include "v8/include/v8-util.h"

namespace blink {
class WebFrame;
}

namespace content {

class GinNativeBridgeObject : public gin::Wrappable<GinNativeBridgeObject>,
                              public gin::NamedPropertyInterceptor {
 public:
  static gin::WrapperInfo kWrapperInfo;
  GinNativeBridgeDispatcher::ObjectID object_id() const { return object_id_; }

  // gin::Wrappable.
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  // gin::NamedPropertyInterceptor
  v8::Local<v8::Value> GetNamedProperty(v8::Isolate* isolate,
                                        const std::string& property) override;
  std::vector<std::string> EnumerateNamedProperties(
      v8::Isolate* isolate) override;

  static GinNativeBridgeObject* InjectNamed(
      blink::WebFrame* frame,
      const base::WeakPtr<GinNativeBridgeDispatcher>& dispatcher,
      const std::string& object_name,
      GinNativeBridgeDispatcher::ObjectID object_id);
  static GinNativeBridgeObject* InjectAnonymous(
      const base::WeakPtr<GinNativeBridgeDispatcher>& dispatcher,
      GinNativeBridgeDispatcher::ObjectID object_id);

 private:
  GinNativeBridgeObject(
      v8::Isolate* isolate,
      const base::WeakPtr<GinNativeBridgeDispatcher>& dispatcher,
      GinNativeBridgeDispatcher::ObjectID object_id);
  ~GinNativeBridgeObject() override;

  v8::Local<v8::FunctionTemplate> GetFunctionTemplate(v8::Isolate* isolate,
                                                      const std::string& name);

  base::WeakPtr<GinNativeBridgeDispatcher> dispatcher_;
  GinNativeBridgeDispatcher::ObjectID object_id_;
  int frame_routing_id_;
  std::map<std::string, bool> known_methods_;
  v8::StdGlobalValueMap<std::string, v8::FunctionTemplate> template_cache_;

  DISALLOW_COPY_AND_ASSIGN(GinNativeBridgeObject);
};

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_BRIDGE_OBJECT_H_
