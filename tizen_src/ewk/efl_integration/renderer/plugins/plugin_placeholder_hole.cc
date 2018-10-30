// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/plugins/plugin_placeholder_hole.h"

#include <algorithm>
#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/child/child_process.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/render_thread_impl.h"
#include "gin/handle.h"
#include "renderer/plugins/hole_layer.h"
#include "third_party/WebKit/Source/platform/ScriptForbiddenScope.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPluginContainer.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebView.h"

namespace {
#if defined(TIZEN_DESKTOP_SUPPORT)
#define ENSURE CHECK
#else
#define ENSURE DCHECK
#endif
}

// JS functions defined in Javascript and called from c++:
//
// window.supportsMimeType(type): return true if must use PluginPlaceholderHole.
// window.onObjectCreated(object): pass the object tag v8 object to the app.
// object_tag.setWindow(x, y, w, h): notify hole geometry.

// JS function defined in c++ and called from Javascript
// object_tag.setTransparent(bool): pass true to make the hole, false for black.

gin::WrapperInfo PluginPlaceholderHole::kWrapperInfo = {
    gin::kEmbedderNativeGin};

// static
bool PluginPlaceholderHole::SupportsMimeType(blink::WebLocalFrame* frame,
                                             const std::string& mimetype) {
  ENSURE(frame);
  ENSURE(content::ChildProcess::current()
             ->main_thread()
             ->message_loop()
             ->task_runner()
             ->BelongsToCurrentThread());

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  ENSURE(isolate);

  v8::HandleScope isolate_scope(isolate);

  std::string script = "window.supportsMimeType('" + mimetype + "')";
  v8::Local<v8::Value> result = frame->ExecuteScriptAndReturnValue(
      blink::WebScriptSource(blink::WebString::FromUTF8(script)));
  if (!result.IsEmpty() && result->IsBoolean() &&
      v8::Local<v8::Boolean>::Cast(result)->IsTrue()) {
    return true;
  }

  LOG(INFO) << "PluginPlaceholderHole: unsupported mimetype " << mimetype;

  return false;
}

// static
void PluginPlaceholderHole::SetTransparentV8Impl(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  ENSURE(content::ChildProcess::current()
             ->main_thread()
             ->message_loop()
             ->task_runner()
             ->BelongsToCurrentThread());

  v8::Isolate* isolate = args.GetIsolate();

  if (args.Length() == 0 || !args[0]->IsBoolean()) {
    args.GetReturnValue().Set(v8::Boolean::New(isolate, false));
    LOG(ERROR) << "PluginPlaceholderHole: wrong SetTransparentV8Impl call";
    return;
  }

  const bool transparent = v8::Local<v8::Boolean>::Cast(args[0])->IsTrue();

  LOG(INFO) << "PluginPlaceholderHole::v8SetTransparent " << transparent;

  v8::Local<v8::External> data = v8::Local<v8::External>::Cast(args.Data());

  // Thanks to PluginPlaceholderHole::GetV8Handle (see WebViewPlugin) the life
  // time of PluginPlaceholder is the same as the frame.
  PluginPlaceholderHole* plugin_placeholder_hole =
      static_cast<PluginPlaceholderHole*>(data->Value());
  ENSURE(plugin_placeholder_hole);

  plugin_placeholder_hole->SetTransparentHelper(transparent);

  args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
}

// static
PluginPlaceholderHole* PluginPlaceholderHole::Create(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params) {
  return new PluginPlaceholderHole(render_frame, frame, params);
}

PluginPlaceholderHole::PluginPlaceholderHole(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params)
    : plugins::PluginPlaceholderBase(render_frame,
                                     frame,
                                     params,
                                     "" /* html_data */),
      web_layer_(nullptr),
      local_rect_(0, 0, 0, 0),
      local_position_in_root_frame_(0, 0),
      main_frame_(frame),
      weak_factory_(this) {
  LOG(INFO) << "PluginPlaceholderHole: create with type "
            << params.mime_type.Utf8() << ", ptr: " << this
            << ", frame: " << main_frame_;
  ENSURE(main_frame_);

  // These 3 getter return the same thread.
  ENSURE(content::ChildProcess::current()->main_thread());
  ENSURE(content::RenderThread::Get());
  ENSURE(content::RenderThreadImpl::current());

  // RenderThreadImpl is the main thread of the Renderer Process.
  ENSURE(content::RenderThreadImpl::current() ==
         content::ChildProcess::current()->main_thread());
  ENSURE(content::RenderThreadImpl::current() == content::RenderThread::Get());

  // The PluginPlaceholderHole is created from the main thread.
  main_task_runner_ = content::ChildProcess::current()
                          ->main_thread()
                          ->message_loop()
                          ->task_runner();
  ENSURE(main_task_runner_->BelongsToCurrentThread());
}

PluginPlaceholderHole::~PluginPlaceholderHole() {
  ENSURE(main_task_runner_->BelongsToCurrentThread());

  v8_object_.Reset();
}

void PluginPlaceholderHole::PluginDestroyed() {
  ENSURE(main_task_runner_->BelongsToCurrentThread());

  if (v8_object_.IsEmpty()) {
    LOG(INFO) << "PluginPlaceholderHole: no object tag to release";
    PluginPlaceholderBase::PluginDestroyed();
    return;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  ENSURE(isolate);

  blink::ScriptForbiddenScope::AllowUserAgentScript allowScript;

  v8::HandleScope isolate_scope(isolate);
  v8::Isolate::AllowJavascriptExecutionScope allowjs(isolate);

  v8::Local<v8::Context> context = v8::Context::New(isolate);
  if (context.IsEmpty()) {
    LOG(ERROR) << "PluginPlaceholderHole: Context is Empty - can't release";
    // We know the v8_object is not empty, so release it (to reduce the
    // amount of memory leaked) before dumping the plugin. If we don't exit
    // here we try and de-reference the env in the context and the browser
    // crashes.
    v8_object_.Reset();
    PluginPlaceholderBase::PluginDestroyed();
    return;
  }

  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> v8_tag_object =
      v8::Local<v8::Object>::New(isolate, v8_object_);

  v8::Local<v8::String> func_name =
      v8::String::NewFromUtf8(isolate, "release", v8::NewStringType::kNormal)
          .ToLocalChecked();
  v8::Local<v8::Value> func_val;
  if (v8_tag_object->Get(context, func_name).ToLocal(&func_val) &&
      func_val->IsFunction()) {
    LOG(INFO) << "PluginPlaceholderHole: call release function for mimetype: "
              << GetPluginParams().mime_type.Utf8();

    v8::Local<v8::Function> func_handle =
        v8::Local<v8::Function>::Cast(func_val);
    v8::TryCatch try_catch(isolate);

    v8::Local<v8::Value> result_val;
    v8::Local<v8::Value> argv[] = {v8_tag_object};
    if (!func_handle->Call(context, v8_tag_object, arraysize(argv), argv)
             .ToLocal(&result_val)) {
      v8::String::Utf8Value error(try_catch.Exception());
      LOG(ERROR) << "PluginPlaceholderHole: release function failed with "
                 << *error;
    }
  }

  v8_object_.Reset();

  PluginPlaceholderBase::PluginDestroyed();
}

v8::Local<v8::Value> PluginPlaceholderHole::GetV8Handle(v8::Isolate* isolate) {
  ENSURE(main_task_runner_->BelongsToCurrentThread());

  GetV8ScriptableObject(isolate);
  return gin::CreateHandle(isolate, this).ToV8();
}

v8::Local<v8::Object> PluginPlaceholderHole::GetV8ScriptableObject(
    v8::Isolate* isolate) const {
  ENSURE(main_task_runner_->BelongsToCurrentThread());

  if (!v8_object_.IsEmpty())
    return v8::Local<v8::Object>::New(isolate, v8_object_);

  if (!main_frame_) {
    LOG(ERROR) << "PluginPlaceholderHole: main frame is null";
    return v8::Local<v8::Object>::New(isolate, v8_object_);
  }

  v8::Local<v8::Context> context = main_frame_->MainWorldScriptContext();
  if (context.IsEmpty()) {
    LOG(ERROR) << "PluginPlaceholderHole: context is empty";
    return v8::Local<v8::Object>::New(isolate, v8_object_);
  }

  v8::Context::Scope context_scope(context);

  v8::Local<v8::String> func_name =
      v8::String::NewFromUtf8(isolate, "onObjectCreated",
                              v8::NewStringType::kNormal)
          .ToLocalChecked();
  v8::Local<v8::Value> func_val;
  if (!context->Global()->Get(context, func_name).ToLocal(&func_val) ||
      !func_val->IsFunction()) {
    LOG(INFO) << "PluginPlaceholderHole: no onObjectCreated function";
    return PluginPlaceholderBase::GetV8ScriptableObject(isolate);
  }

  v8::Local<v8::Object> obj_tag = v8::Object::New(isolate);
  std::string mimeType = GetPluginParams().mime_type.Utf8();
  obj_tag->Set(v8::String::NewFromUtf8(isolate, "type"),
               v8::String::NewFromUtf8(isolate, mimeType.c_str()));

  v8::Local<v8::External> plugin_placeholder_hole =
      v8::External::New(isolate, const_cast<PluginPlaceholderHole*>(this));
  obj_tag->Set(v8::String::NewFromUtf8(isolate, "setTransparent"),
               v8::Function::New(isolate, SetTransparentV8Impl,
                                 plugin_placeholder_hole));

  v8::Local<v8::Function> func_handle = v8::Local<v8::Function>::Cast(func_val);
  v8::TryCatch try_catch(isolate);

  // onObjectCreated might call setTransparent.
  v8_object_.Reset(isolate, obj_tag);

  v8::Local<v8::Value> result_val;
  v8::Local<v8::Value> argv[] = {obj_tag};
  if (!func_handle->Call(context, context->Global(), arraysize(argv), argv)
           .ToLocal(&result_val)) {
    v8::String::Utf8Value error(try_catch.Exception());
    LOG(ERROR) << "PluginPlaceholderHole: onObjectCreated failed with "
               << *error;
    return PluginPlaceholderBase::GetV8ScriptableObject(isolate);
  }

  NotitfyAttributes();

  LOG(INFO) << "PluginPlaceholderHole: v8 scriptable object created for type "
            << mimeType << ", initial rect is " << local_rect_.ToString();

  // Useful when OnUnobscuredRectUpdate is called before creating the v8 object.
  if (local_rect_.width() > 0 && local_rect_.height() > 0)
    const_cast<PluginPlaceholderHole*>(this)->NotifyGeometryChanged();

  return v8::Local<v8::Object>::New(isolate, v8_object_);
}

void PluginPlaceholderHole::OnUnobscuredRectUpdate(
    const gfx::Rect& unobscured_rect) {
  ENSURE(main_task_runner_->BelongsToCurrentThread());

  bool should_notify = false;

  if (local_rect_ != unobscured_rect) {
    // Do not consider lines or points as visible.
    if (unobscured_rect.width() < 1 || unobscured_rect.height() < 1) {
      LOG(ERROR) << "PluginPlaceholderHole: discard malformed rect "
                 << unobscured_rect.ToString();
    } else {
      should_notify = true;
    }
  }

  gfx::Point position_in_root_frame =
      plugin()->Container()->LocalToRootFramePoint(unobscured_rect.origin());

  if (local_position_in_root_frame_ != position_in_root_frame) {
    local_position_in_root_frame_ = position_in_root_frame;
    should_notify = true;
  }

  if (!should_notify)
    return;

  // Do not move or remove this line, it is to cache the rect because there are
  // cases where OnUnobscuredRectUpdate is called only one time and before the
  // v8 object is created. It will call NotifyGeometryChanged at creating time
  // in that case.
  local_rect_ = unobscured_rect;
  LOG(INFO) << "PluginPlaceholderHole: hole rect changed "
            << local_rect_.ToString();

  if (v8_object_.IsEmpty() || !web_layer_) {
    PluginPlaceholderBase::OnUnobscuredRectUpdate(unobscured_rect);

    LOG(INFO) << "PluginPlaceholderHole: delay rect, is object empty: "
              << v8_object_.IsEmpty() << ", is layer empty: " << !web_layer_;
    return;
  }

  NotifyGeometryChanged();
}

void PluginPlaceholderHole::SetTransparentHelper(bool transparent) {
  ENSURE(main_task_runner_->BelongsToCurrentThread());

  if (!web_layer_) {
    if (!plugin()) {
      LOG(ERROR) << "PluginPlaceholderHole: failed to set transparent "
                 << transparent << ", because plugin is null.";
      return;
    }
    blink::WebPluginContainer* container = plugin()->Container();
    if (!container) {
      LOG(ERROR) << "PluginPlaceholderHole: failed to set transparent "
                 << transparent << ", because container is null.";
      return;
    }
    scoped_refptr<cc::HoleLayer> layer_ = new cc::HoleLayer;
    web_layer_ = base::WrapUnique(new cc_blink::WebLayerImpl(layer_));
    container->SetWebLayer(web_layer_.get());
    LOG(INFO) << "PluginPlaceholderHole: hole layer created";

    // Useful when OnUnobscuredRectUpdate is called before creating the layer.
    if (local_rect_.width() > 0 && local_rect_.height() > 0)
      const_cast<PluginPlaceholderHole*>(this)->NotifyGeometryChanged();
  }

  // Transparent color + opaque means hole punch through.
  web_layer_->SetBackgroundColor(transparent ? SK_ColorTRANSPARENT
                                             : SK_ColorBLACK);
  web_layer_->SetOpaque(transparent);
  web_layer_->SetContentsOpaqueIsFixed(transparent);

  LOG(INFO) << "PluginPlaceholderHole: SetTransparentHelper " << transparent;
}

void PluginPlaceholderHole::NotifyGeometryChanged() {
  ENSURE(main_task_runner_->BelongsToCurrentThread());

  if (v8_object_.IsEmpty()) {
    LOG(ERROR) << "PluginPlaceholderHole::NotifyGeometryChanged, wrong state,"
               << "v8 object should not be empty.";
    return;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  ENSURE(isolate);

  v8::HandleScope isolate_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> v8_tag_object =
      v8::Local<v8::Object>::New(isolate, v8_object_);

  v8::Local<v8::String> func_name =
      v8::String::NewFromUtf8(isolate, "setWindow", v8::NewStringType::kNormal)
          .ToLocalChecked();
  v8::Local<v8::Value> func_val;
  if (!v8_tag_object->Get(context, func_name).ToLocal(&func_val) ||
      !func_val->IsFunction()) {
    LOG(ERROR) << "PluginPlaceholderHole: no setWindow for mimetype: "
               << GetPluginParams().mime_type.Utf8();
    return;
  }

  v8::Local<v8::Function> func_handle = v8::Local<v8::Function>::Cast(func_val);
  v8::TryCatch try_catch(isolate);

  gfx::Point position_in_root_frame =
      plugin()->Container()->LocalToRootFramePoint(local_rect_.origin());

  v8::Local<v8::Value> result_val;
  v8::Local<v8::Value> argv[] = {
      v8::Integer::New(isolate, position_in_root_frame.x()),
      v8::Integer::New(isolate, position_in_root_frame.y()),
      v8::Integer::New(isolate, local_rect_.width()),
      v8::Integer::New(isolate, local_rect_.height())};
  if (!func_handle->Call(context, v8_tag_object, arraysize(argv), argv)
           .ToLocal(&result_val)) {
    v8::String::Utf8Value error(try_catch.Exception());
    LOG(ERROR) << "PluginPlaceholderHole: setWindow failed with " << *error;
    return;
  }

  LOG(INFO) << "PluginPlaceholderHole: setWindow succeeded";
}

void PluginPlaceholderHole::NotitfyAttributes() const {
  ENSURE(main_task_runner_->BelongsToCurrentThread());

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  ENSURE(isolate);

  v8::HandleScope isolate_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  v8::Context::Scope context_scope(context);

  v8::Local<v8::Object> v8_tag_object =
      v8::Local<v8::Object>::New(isolate, v8_object_);

  v8::Local<v8::String> func_name =
      v8::String::NewFromUtf8(isolate, "setAttributes",
                              v8::NewStringType::kNormal)
          .ToLocalChecked();
  v8::Local<v8::Value> func_val;
  if (!v8_tag_object->Get(context, func_name).ToLocal(&func_val) ||
      !func_val->IsFunction()) {
    LOG(ERROR) << "PluginPlaceholderHole: no setAttributes for mimetype: "
               << GetPluginParams().mime_type.Utf8();
    return;
  }

  size_t params_size = GetPluginParams().attribute_names.size();
  if (params_size == 0)
    return;

  v8::Local<v8::Array> params = v8::Array::New(isolate, params_size);

  for (size_t i = 0; i < params_size; ++i) {
    v8::Local<v8::Object> attribute = v8::Object::New(isolate);
    attribute->Set(
        v8::String::NewFromUtf8(
            isolate, GetPluginParams().attribute_names[i].Utf8().c_str()),
        v8::String::NewFromUtf8(
            isolate, GetPluginParams().attribute_values[i].Utf8().c_str()));
    params->Set(i, attribute);
  }

  v8::Local<v8::Function> func_handle = v8::Local<v8::Function>::Cast(func_val);
  v8::TryCatch try_catch(isolate);

  v8::Local<v8::Value> result_val;
  v8::Local<v8::Value> argv[] = {params};
  if (!func_handle->Call(context, v8_tag_object, arraysize(argv), argv)
           .ToLocal(&result_val)) {
    v8::String::Utf8Value error(try_catch.Exception());
    LOG(ERROR) << "PluginPlaceholderHole: setAttributes failed with " << *error;
    return;
  }

  LOG(INFO) << "PluginPlaceholderHole: setAttributes succeeded";
}
