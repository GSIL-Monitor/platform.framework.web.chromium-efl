// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/javascript_interface/gin_native_bound_object.h"

namespace content {

GinNativeBoundObject::GinNativeBoundObject(Evas_Object* obj,
                                           Ewk_View_Script_Message_Cb callback,
                                           const std::string& name)
    : obj_(obj), callback_(callback), name_(name), names_count_(0) {}

/* LCOV_EXCL_START */
GinNativeBoundObject::GinNativeBoundObject(Evas_Object* obj,
                                           Ewk_View_Script_Message_Cb callback,
                                           const std::string& name,
                                           const std::set<int32_t>& holders)
    : obj_(obj),
      callback_(callback),
      name_(name),
      names_count_(0),
      holders_(holders) {}
/* LCOV_EXCL_STOP */

GinNativeBoundObject::~GinNativeBoundObject() {}

// static
GinNativeBoundObject* GinNativeBoundObject::CreateNamed(
    Evas_Object* obj,
    Ewk_View_Script_Message_Cb callback,
    const std::string& name) {
  return new GinNativeBoundObject(obj, callback, name);
}

/* LCOV_EXCL_START */
// static
GinNativeBoundObject* GinNativeBoundObject::CreateTransient(
    Evas_Object* obj,
    Ewk_View_Script_Message_Cb callback,
    const std::string& name,
    int32_t holder) {
  std::set<int32_t> holders;
  holders.insert(holder);
  return new GinNativeBoundObject(obj, callback, name, holders);
}
/* LCOV_EXCL_STOP */

}  // namespace content
