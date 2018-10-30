// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/gin_native_bridge_dispatcher.h"

#include "common/gin_native_bridge_messages.h"
#include "content/public/renderer/render_frame.h"
#include "renderer/gin_native_bridge_object.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

namespace content {

/* LCOV_EXCL_START */
GinNativeBridgeDispatcher::GinNativeBridgeDispatcher(RenderFrame* render_frame)
    : RenderFrameObserver(render_frame),
      inside_did_clear_window_object_(false) {}

GinNativeBridgeDispatcher::~GinNativeBridgeDispatcher() {}

bool GinNativeBridgeDispatcher::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(GinNativeBridgeDispatcher, msg)
    IPC_MESSAGE_HANDLER(GinNativeBridgeMsg_AddNamedObject, OnAddNamedObject)
    IPC_MESSAGE_HANDLER(GinNativeBridgeMsg_RemoveNamedObject,
                        OnRemoveNamedObject)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}
/* LCOV_EXCL_STOP */

namespace {

class ScopedFlag {
 public:
  explicit ScopedFlag(bool* flag) : flag_(flag) {  // LCOV_EXCL_LINE
    DCHECK(!*flag_);
    *flag_ = true;  // LCOV_EXCL_LINE
  }
  ~ScopedFlag() {
    DCHECK(*flag_);
    *flag_ = false;  // LCOV_EXCL_LINE
  }                  // LCOV_EXCL_LINE

 private:
  bool* flag_;

  DISALLOW_COPY_AND_ASSIGN(ScopedFlag);
};
}  // namespace

/* LCOV_EXCL_START */
void GinNativeBridgeDispatcher::DidClearWindowObject() {
  // Accessing window object when adding properties to it may trigger
  // a nested call to DidClearWindowObject.
  if (inside_did_clear_window_object_)
    return;
  ScopedFlag flag(&inside_did_clear_window_object_);

  for (NamedObjectMap::const_iterator iter = named_objects_.begin();
       iter != named_objects_.end(); ++iter) {
    // Always create a new GinJavaBridgeObject, so we don't pull any of the V8
    // wrapper's custom properties into the context of the page we have
    // navigated to. The old GinJavaBridgeObject will be automatically
    // deleted after its wrapper will be collected.
    // On the browser side, we ignore wrapper deletion events for named objects,
    // as they are only removed upon embedder's request (RemoveNamedObject).
    if (objects_.Lookup(iter->second))
      objects_.Remove(iter->second);

    GinNativeBridgeObject* object = GinNativeBridgeObject::InjectNamed(
        render_frame()->GetWebFrame(), AsWeakPtr(), iter->first, iter->second);

    if (object) {
      objects_.AddWithID(object, iter->second);
    } else {
      // FIXME : Inform the host about wrapper creation failure.
    }
  }
}

void GinNativeBridgeDispatcher::OnAddNamedObject(const std::string& name,
                                                 ObjectID object_id) {
  // Added objects only become available after page reload, so here they
  // are only added into the internal map.
  named_objects_.insert(std::make_pair(name, object_id));
}

void GinNativeBridgeDispatcher::OnRemoveNamedObject(const std::string& name) {
  // Removal becomes in effect on next reload. We simply removing the entry
  // from the map here.
  NamedObjectMap::iterator iter = named_objects_.find(name);
  DCHECK(iter != named_objects_.end());
  named_objects_.erase(iter);
}

void GinNativeBridgeDispatcher::GetNativeMethods(
    ObjectID object_id,
    std::set<std::string>* methods) {
  render_frame()->Send(
      new GinNativeBridgeHostMsg_GetMethods(routing_id(), object_id, methods));
}

bool GinNativeBridgeDispatcher::HasNativeMethod(
    ObjectID object_id,
    const std::string& method_name) {
  bool result;
  render_frame()->Send(new GinNativeBridgeHostMsg_HasMethod(
      routing_id(), object_id, method_name, &result));
  return result;
}

std::unique_ptr<base::Value> GinNativeBridgeDispatcher::InvokeNativeMethod(
    ObjectID object_id,
    const std::string& method_name,
    const base::ListValue& arguments,
    GinNativeBridgeError* error) {
  base::ListValue result_wrapper;
  render_frame()->Send(new GinNativeBridgeHostMsg_InvokeMethod(
      routing_id(), object_id, method_name, arguments, &result_wrapper, error));
  base::Value* result;
  if (result_wrapper.Get(0, &result))
    return std::unique_ptr<base::Value>(result->DeepCopy());
  else
    return std::unique_ptr<base::Value>();
}

GinNativeBridgeObject* GinNativeBridgeDispatcher::GetObject(
    ObjectID object_id) {
  GinNativeBridgeObject* result = objects_.Lookup(object_id);
  if (!result) {
    result = GinNativeBridgeObject::InjectAnonymous(AsWeakPtr(), object_id);
    if (result)
      objects_.AddWithID(result, object_id);
  }
  return result;
}

void GinNativeBridgeDispatcher::OnDestruct() {
  delete this;
}
/* LCOV_EXCL_STOP */

}  // namespace content
