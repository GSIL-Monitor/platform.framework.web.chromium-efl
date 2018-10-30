// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_BRIDGE_DISPATCHER_H_
#define EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_BRIDGE_DISPATCHER_H_

#include <map>
#include <set>
#include <string>

#include "base/containers/id_map.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "content/public/renderer/render_frame_observer.h"
#include "ewk/efl_integration/common/gin_native_bridge_errors.h"

namespace blink {
class WebFrame;
}

namespace content {

class GinNativeBridgeObject;

// This class handles injecting Native objects into the main frame of a
// RenderView. The 'add' and 'remove' messages received from the browser
// process modify the entries in a map of 'pending' objects. These objects are
// bound to the window object of the main frame when that window object is next
// cleared. These objects remain bound until the window object is cleared
// again.

class GinNativeBridgeDispatcher
    : public base::SupportsWeakPtr<GinNativeBridgeDispatcher>,
      public RenderFrameObserver {
 public:
  // GinNativeBridgeObjects are managed by gin. An object gets destroyed
  // when it is no more referenced from JS. As GinNativeBridgeObjects reports
  // deletion of self to GinNativeBridgeDispatcher, we would not have stale
  // pointers here.
  typedef base::IDMap<GinNativeBridgeObject*> ObjectMap;
  typedef ObjectMap::KeyType ObjectID;

  explicit GinNativeBridgeDispatcher(RenderFrame* render_frame);
  ~GinNativeBridgeDispatcher() override;

  // RenderFrameObserver override:
  void OnDestruct() override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void DidClearWindowObject() override;

  void GetNativeMethods(ObjectID object_id, std::set<std::string>* methods);
  bool HasNativeMethod(ObjectID object_id, const std::string& method_name);
  std::unique_ptr<base::Value> InvokeNativeMethod(
      ObjectID object_id,
      const std::string& method_name,
      const base::ListValue& arguments,
      GinNativeBridgeError* error);
  GinNativeBridgeObject* GetObject(ObjectID object_id);

 private:
  void OnAddNamedObject(const std::string& name, ObjectID object_id);
  void OnRemoveNamedObject(const std::string& name);

  typedef std::map<std::string, ObjectID> NamedObjectMap;
  NamedObjectMap named_objects_;
  ObjectMap objects_;
  bool inside_did_clear_window_object_;
  DISALLOW_COPY_AND_ASSIGN(GinNativeBridgeDispatcher);
};

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_RENDERER_GIN_NATIVE_BRIDGE_DISPATCHER_H_
