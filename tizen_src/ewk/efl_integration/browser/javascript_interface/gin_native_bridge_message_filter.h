// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_INTERFACE_GIN_NATIVE_BRIDGE_MESSAGE_FILTER_H_
#define EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_INTERFACE_GIN_NATIVE_BRIDGE_MESSAGE_FILTER_H_

#include <stdint.h>

#include <map>
#include <set>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "browser/javascript_interface/gin_native_bound_object.h"
#include "common/gin_native_bridge_errors.h"
#include "content/public/browser/browser_message_filter.h"

namespace base {
class ListValue;
}

namespace IPC {
class Message;
}

namespace content {

class GinNativeBridgeDispatcherHost;
class RenderFrameHost;

class GinNativeBridgeMessageFilter : public BrowserMessageFilter {
 public:
  // BrowserMessageFilter
  void OnDestruct() const override;
  bool OnMessageReceived(const IPC::Message& message) override;
  base::TaskRunner* OverrideTaskRunnerForMessage(
      const IPC::Message& message) override;

  // Called on the UI thread.
  void AddRoutingIdForHost(GinNativeBridgeDispatcherHost* host,
                           RenderFrameHost* render_frame_host);
  void RemoveHost(GinNativeBridgeDispatcherHost* host);

  static scoped_refptr<GinNativeBridgeMessageFilter> FromHost(
      GinNativeBridgeDispatcherHost* host,
      bool create_if_not_exists);

 private:
  friend class BrowserThread;
  friend class base::DeleteHelper<GinNativeBridgeMessageFilter>;

  // WebView (who owns GinNativeBridgeDispatcherHost) outlives
  // WebContents, and GinNativeBridgeDispatcherHost removes itself form the map
  // on WebContents destruction, so there is no risk that the pointer would
  // become stale.
  //
  // The filter keeps its own routing map of RenderFrames for two reasons:
  //  1. Message dispatching must be done on the background thread,
  //     without resorting to the UI thread, which can be in fact currently
  //     blocked, waiting for an event from an injected Native object.
  //  2. As RenderFrames pass away earlier than JavaScript wrappers,
  //     messages form the latter can arrive after the RenderFrame has been
  //     removed from the WebContent's routing table.
  typedef std::map<int32_t, GinNativeBridgeDispatcherHost*> HostMap;

  GinNativeBridgeMessageFilter();
  ~GinNativeBridgeMessageFilter() override;

  // Called on the background thread.
  GinNativeBridgeDispatcherHost* FindHost();
  void OnHasMethod(GinNativeBoundObject::ObjectID object_id,
                   const std::string& method_name,
                   bool* result);
  void OnInvokeMethod(GinNativeBoundObject::ObjectID object_id,
                      const std::string& method_name,
                      const base::ListValue& arguments,
                      base::ListValue* result,
                      content::GinNativeBridgeError* error_code);

  // Accessed both from UI and background threads.
  HostMap hosts_;
  base::Lock hosts_lock_;

  // The routing id of the RenderFrameHost whose request we are processing.
  // Used on the backgrount thread.
  int32_t current_routing_id_;
};

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_INTERFACE_GIN_NATIVE_BRIDGE_MESSAGE_FILTER_H_
