// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_INTERFACE_GIN_NATIVE_BOUND_OBJECT_H_
#define EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_INTERFACE_GIN_NATIVE_BOUND_OBJECT_H_

#include <stddef.h>
#include <stdint.h>

#include <set>
#include <string>

#include "base/memory/ref_counted.h"
#include "public/ewk_view.h"

namespace content {

class GinNativeBoundObject
    : public base::RefCountedThreadSafe<GinNativeBoundObject> {
 public:
  GinNativeBoundObject(Evas_Object* obj,
                       Ewk_View_Script_Message_Cb callback,
                       const std::string& name);
  GinNativeBoundObject(Evas_Object* obj,
                       Ewk_View_Script_Message_Cb callback,
                       const std::string& name,
                       const std::set<int32_t>& holders);

  typedef int32_t ObjectID;
  static GinNativeBoundObject* CreateNamed(Evas_Object* obj,
                                           Ewk_View_Script_Message_Cb callback,
                                           const std::string& name);
  static GinNativeBoundObject* CreateTransient(
      Evas_Object* obj,
      Ewk_View_Script_Message_Cb callback,
      const std::string& name,
      int32_t holder);
  void AddName() { ++names_count_; }
  void RemoveName() { --names_count_; }  // LCOV_EXCL_LINE
  Evas_Object* GetView() const { return obj_; }
  Ewk_View_Script_Message_Cb CallBack() const { return callback_; }
  const char* Name() const { return name_.c_str(); }

 private:
  friend class base::RefCountedThreadSafe<GinNativeBoundObject>;
  ~GinNativeBoundObject();

  Evas_Object* obj_;
  Ewk_View_Script_Message_Cb callback_;
  std::string name_;

  // An object must be kept in retained_object_set_ either if it has
  // names or if it has a non-empty holders set.
  int names_count_;
  std::set<int32_t> holders_;
};

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_INTERFACE_GIN_NATIVE_BOUND_OBJECT_H_
