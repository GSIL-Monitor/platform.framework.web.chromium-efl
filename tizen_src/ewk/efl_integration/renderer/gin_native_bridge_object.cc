// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/gin_native_bridge_object.h"

#include "common/gin_native_bridge_messages.h"
#include "content/public/renderer/render_thread.h"
#include "gin/function_template.h"
#include "renderer/gin_native_function_invocation_helper.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace content {
/* LCOV_EXCL_START */
// static
GinNativeBridgeObject* GinNativeBridgeObject::InjectNamed(
    blink::WebFrame* frame,
    const base::WeakPtr<GinNativeBridgeDispatcher>& dispatcher,
    const std::string& object_name,
    GinNativeBridgeDispatcher::ObjectID object_id) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  if (!frame->IsWebLocalFrame())
    return NULL;

  v8::Local<v8::Context> context =
      frame->ToWebLocalFrame()->MainWorldScriptContext();
  if (context.IsEmpty())
    return NULL;

  GinNativeBridgeObject* object =
      new GinNativeBridgeObject(isolate, dispatcher, object_id);

  v8::Context::Scope context_scope(context);
  v8::Handle<v8::Object> global = context->Global();
  gin::Handle<GinNativeBridgeObject> controller =
      gin::CreateHandle(isolate, object);
  // WrappableBase instance deletes itself in case of a wrapper
  // creation failure, thus there is no need to delete |object|.
  if (controller.IsEmpty())
    return NULL;

  global->Set(gin::StringToV8(isolate, object_name), controller.ToV8());
  return object;
}

// static
GinNativeBridgeObject* GinNativeBridgeObject::InjectAnonymous(
    const base::WeakPtr<GinNativeBridgeDispatcher>& dispatcher,
    GinNativeBridgeDispatcher::ObjectID object_id) {
  return new GinNativeBridgeObject(blink::MainThreadIsolate(), dispatcher,
                                   object_id);
}

GinNativeBridgeObject::GinNativeBridgeObject(
    v8::Isolate* isolate,
    const base::WeakPtr<GinNativeBridgeDispatcher>& dispatcher,
    GinNativeBridgeDispatcher::ObjectID object_id)
    : gin::NamedPropertyInterceptor(isolate, this),
      dispatcher_(dispatcher),
      object_id_(object_id),
      frame_routing_id_(dispatcher_->routing_id()),
      template_cache_(isolate) {}

GinNativeBridgeObject::~GinNativeBridgeObject() {}

gin::ObjectTemplateBuilder GinNativeBridgeObject::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<GinNativeBridgeObject>::GetObjectTemplateBuilder(
             isolate)
      .AddNamedPropertyInterceptor();
}

v8::Local<v8::Value> GinNativeBridgeObject::GetNamedProperty(
    v8::Isolate* isolate,
    const std::string& property) {
  std::map<std::string, bool>::iterator method_pos =
      known_methods_.find(property);
  if (method_pos == known_methods_.end()) {
    if (!dispatcher_)
      return v8::Local<v8::Value>();

    known_methods_[property] =
        dispatcher_->HasNativeMethod(object_id_, property);
  }
  if (known_methods_[property])
    return GetFunctionTemplate(isolate, property)->GetFunction();
  else
    return v8::Local<v8::Value>();
}

std::vector<std::string> GinNativeBridgeObject::EnumerateNamedProperties(
    v8::Isolate* isolate) {
  std::set<std::string> method_names;
  if (dispatcher_)
    dispatcher_->GetNativeMethods(object_id_, &method_names);
  return std::vector<std::string>(method_names.begin(), method_names.end());
}

v8::Local<v8::FunctionTemplate> GinNativeBridgeObject::GetFunctionTemplate(
    v8::Isolate* isolate,
    const std::string& name) {
  v8::Local<v8::FunctionTemplate> function_template = template_cache_.Get(name);
  if (!function_template.IsEmpty())
    return function_template;
  function_template = gin::CreateFunctionTemplate(
      isolate, base::Bind(&GinNativeFunctionInvocationHelper::Invoke,
                          base::Owned(new GinNativeFunctionInvocationHelper(
                              name, dispatcher_))));
  template_cache_.Set(name, function_template);
  return function_template;
}
/* LCOV_EXCL_STOP */

gin::WrapperInfo GinNativeBridgeObject::kWrapperInfo = {
    gin::kEmbedderNativeGin};

}  // namespace content
