// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/extension_system_samsung_tizen.h"

namespace {
const char kCheckPrivilegeOperationName[] = "check_ace_privilege";
const char kGetWindowIdOperationName[] = "get_window_id";
#if defined(OS_TIZEN_TV_PRODUCT)
const char kSetIMERecommendedWordsName[] = "set_ime_recommended_words";
const char kSetIMERecommendedWordsTypeName[] = "set_ime_recommended_words_type";
#endif
}  // namespace

namespace pp {
ExtensionSystemSamsungTizen::ExtensionSystemSamsungTizen(
    const InstanceHandle& instance)
    : ExtensionSystemSamsung(instance) {}

ExtensionSystemSamsungTizen::~ExtensionSystemSamsungTizen() {}

bool ExtensionSystemSamsungTizen::CheckPrivilege(const Var& privilege) {
  if (!privilege.is_string())
    return false;
  std::string privilege_string = privilege.AsString();
  std::map<std::string, bool>::iterator it =
      privileges_result_.find(privilege_string);
  if (it != privileges_result_.end())
    return it->second;
  Var call_result = GenericSyncCall(kCheckPrivilegeOperationName, privilege);
  bool has_privilege = false;
  if (call_result.is_bool())
    has_privilege = call_result.AsBool();
  privileges_result_[privilege_string] = has_privilege;
  return has_privilege;
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool ExtensionSystemSamsungTizen::SetIMERecommendedWords(const Var& words) {
  Var call_result = GenericSyncCall(kSetIMERecommendedWordsName, words);
  if (call_result.is_bool())
    return call_result.AsBool();
  return false;
}

bool ExtensionSystemSamsungTizen::SetIMERecommendedWordsType(
    bool should_enable) {
  Var enable_var(should_enable);
  Var call_result =
      GenericSyncCall(kSetIMERecommendedWordsTypeName, enable_var);
  if (call_result.is_bool())
    return call_result.AsBool();
  return false;
}
#endif

int32_t ExtensionSystemSamsungTizen::GetWindowId() {
  // The 'dummy' variable is not used, but needed because of the signature
  // of the get_window_id operation.
  Var call_result = GenericSyncCall(kGetWindowIdOperationName, pp::Var());
  if (call_result.is_number())
    return call_result.AsInt();
  return -1;
}
}  // namespace pp
