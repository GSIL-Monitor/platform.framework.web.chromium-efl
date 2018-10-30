// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_INTERFACE_GIN_NATIVE_BRIDGE_DISPATCHER_HOST_H_
#define EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_INTERFACE_GIN_NATIVE_BRIDGE_DISPATCHER_HOST_H_

#include <map>
#include <set>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "browser/javascript_interface/gin_native_bound_object.h"
#include "common/gin_native_bridge_errors.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/web_contents_observer.h"
#include "public/ewk_view.h"

namespace content {

class GinNativeBridgeDispatcherHost
    : public base::RefCountedThreadSafe<GinNativeBridgeDispatcherHost>,
      public WebContentsObserver {
 public:
  explicit GinNativeBridgeDispatcherHost(WebContents* web_contents);
  ~GinNativeBridgeDispatcherHost() override;

  // WebContentsObserver
  void RenderFrameCreated(RenderFrameHost* render_frame_host) override;
  void WebContentsDestroyed() override;
  void DocumentAvailableInMainFrame() override;

  bool AddNamedObject(Evas_Object* view,
                      Ewk_View_Script_Message_Cb callback,
                      const std::string& name);
  void RemoveNamedObject(const std::string& name);
  void OnHasMethod(GinNativeBoundObject::ObjectID object_id,
                   const std::string& method_name,
                   bool* result);

  void OnInvokeMethod(int routing_id,
                      GinNativeBoundObject::ObjectID object_id,
                      const std::string& method_name,
                      const base::ListValue& arguments,
                      base::ListValue* result,
                      content::GinNativeBridgeError* error_code);

 private:
  friend class base::RefCountedThreadSafe<GinNativeBridgeDispatcherHost>;
  typedef std::map<GinNativeBoundObject::ObjectID,
                   scoped_refptr<GinNativeBoundObject>>
      ObjectMap;

  // Run on the UI thread.
  void InstallFilterAndRegisterAllRoutingIds();

  bool FindObjectId();
  GinNativeBoundObject::ObjectID AddObject(Evas_Object* view,
                                           Ewk_View_Script_Message_Cb callback,
                                           const std::string& name,
                                           bool is_named,
                                           int32_t holder);
  scoped_refptr<GinNativeBoundObject> FindObject(
      GinNativeBoundObject::ObjectID object_id);

  std::string ConvertListValueToString(const base::ListValue* list);

  std::string ConvertDictionaryValueToString(const base::DictionaryValue* dict);

  typedef std::map<std::string, GinNativeBoundObject::ObjectID> NamedObjectMap;
  NamedObjectMap named_objects_;

  GinNativeBoundObject::ObjectID next_object_id_;

  // Note that retained_object_set_ does not need to be consistent
  // with objects_.
  ObjectMap objects_;
  base::Lock objects_lock_;

  struct _JavaScript_Values {
    Eina_Bool bool_buf;  /**< Buffer for boolean */
    int int_buf;         /**< Buffer for integer  */
    double double_buf;   /**< Buffer for double  */
    std::string str_buf; /**< Buffer for string  */
  };
  typedef struct _JavaScript_Values JavaScript_Values;

  DISALLOW_COPY_AND_ASSIGN(GinNativeBridgeDispatcherHost);
};

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_INTERFACE_GIN_NATIVE_BRIDGE_DISPATCHER_HOST_H_
