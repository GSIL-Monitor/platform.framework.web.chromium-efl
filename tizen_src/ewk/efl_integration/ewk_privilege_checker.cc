// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk/efl_integration/ewk_privilege_checker.h"

#if defined(OS_TIZEN)
#include <app_manager.h>
#include <cynara-client.h>
#include <pkgmgr-info.h>
#include <privilege_manager.h>
#include <unistd.h>
#endif  // defined(OS_TIZEN)

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/compiler_specific.h"

#if defined(OS_TIZEN)
namespace {
bool GetPkgApiVersion(std::string* api_version) {
  char* app_id = nullptr;
  char* pkgid = nullptr;
  // api_ver memory is released by pkgmgrinfo_pkginfo_destroy_pkginfo method
  char* api_ver = nullptr;
  app_info_h app_handle = nullptr;
  pkgmgrinfo_pkginfo_h pkginfo_handle = nullptr;

  pid_t pid = getpid();
  int ret = app_manager_get_app_id(pid, &app_id);
  if (ret != APP_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "Can`t get app_id";
    return false;
  }

  ret = app_info_create(app_id, &app_handle);
  free(app_id);
  if (ret != APP_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "Can`t get app_handle";
    return false;
  }

  ret = app_info_get_package(app_handle, &pkgid);
  app_info_destroy(app_handle);
  if (ret != APP_MANAGER_ERROR_NONE || pkgid == nullptr) {
    LOG(ERROR) << "Can`t get pkgid";
    return false;
  }

  ret = pkgmgrinfo_pkginfo_get_usr_pkginfo(pkgid, getuid(), &pkginfo_handle);
  free(pkgid);
  if (ret != PMINFO_R_OK) {
    LOG(ERROR) << "Can`t get pkginfo_handle";
    return false;
  }

  ret = pkgmgrinfo_pkginfo_get_api_version(pkginfo_handle, &api_ver);
  if (ret != PMINFO_R_OK) {
    LOG(ERROR) << "Can`t get api_ver";
    return false;
  }

  *api_version = api_ver;
  pkgmgrinfo_pkginfo_destroy_pkginfo(pkginfo_handle);
  return true;
}

bool GetPrivilegeMapping(const std::string& privilege_name,
                         const std::string& api_version,
                         std::vector<std::string>* privilege_mapping) {
  if (!privilege_mapping)
    return false;
  char* local_privilege_name = strdup(privilege_name.c_str());
  GList* privilege_list = nullptr;
  privilege_list = g_list_append(privilege_list, local_privilege_name);

  auto g_list_deleter = [](GList* p) {
    auto data_deleter = [](gpointer data, gpointer user_data) {
      ALLOW_UNUSED_LOCAL(user_data);
      char* char_data = static_cast<char*>(data);
      free(char_data);
    };
    p = g_list_first(p);
    g_list_foreach(p, data_deleter, nullptr);
    g_list_free(p);
  };

  auto privilege_list_holder = std::unique_ptr<GList, decltype(g_list_deleter)>{
      privilege_list, g_list_deleter};

  GList* mapped_privilege_list = nullptr;
  int ret = privilege_manager_get_mapped_privilege_list(
      api_version.c_str(), PRVMGR_PACKAGE_TYPE_WRT, privilege_list_holder.get(),
      &mapped_privilege_list);

  auto mapped_list_holder = std::unique_ptr<GList, decltype(g_list_deleter)>{
      mapped_privilege_list, g_list_deleter};
  if (ret != PRVMGR_ERR_NONE) {
    LOG(ERROR) << "Mapping returned with code: " << ret;
    return false;
  }

  // If privilege was successfully resolved but returned empty list, we always
  // return false for security reasons.
  guint size = g_list_length(mapped_list_holder.get());
  if (!size) {
    LOG(WARNING) << "No mapping for privilege " << privilege_name.c_str();
    return false;
  }
  GList* element = g_list_first(mapped_list_holder.get());
  while (element) {
    char* privilege = static_cast<char*>(element->data);
    privilege_mapping->emplace_back(privilege);
    element = g_list_next(element);
  }
  return true;
}
}  // namespace
#endif  // defined(OS_TIZEN)

namespace content {

// static
EwkPrivilegeChecker* EwkPrivilegeChecker::GetInstance() {
  return base::Singleton<EwkPrivilegeChecker>::get();
}

bool EwkPrivilegeChecker::CheckPrivilege(const std::string& privilege_name) {
#if defined(OS_TIZEN)
  static constexpr char kSmackLabelFilePath[] = "/proc/self/attr/current";

  int ret;
  cynara* p_cynara = nullptr;
  ret = cynara_initialize(&p_cynara, 0);
  if (ret != CYNARA_API_SUCCESS) {
    LOG(ERROR) << "Couldn`t initialize Cynara";
    return false;
  }

  auto cynara_deleter = [](cynara* p) { cynara_finish(p); };
  auto cynara_holder = std::unique_ptr<cynara, decltype(cynara_deleter)>{
      p_cynara, cynara_deleter};

  std::string uid = std::to_string(getuid());

  // Get smack label
  std::ifstream file(kSmackLabelFilePath);
  if (!file.is_open()) {
    LOG(ERROR) << "Failed to open " << kSmackLabelFilePath;
    return false;
  }

  std::string smack_label{std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>()};

  // Get widget api version, which is needed to resolve older privileges from
  // previous platforms. Api version is set inside of config.xml in widget.
  std::string api_version;
  if (!GetPkgApiVersion(&api_version)) {
    LOG(ERROR) << "Failed to acquire api version. "
               << "Can`t resolve properly privilege mapping!";
    return false;
  }

  // Resolve privileges for their requested api_version on currently running
  // Tizen version.
  std::vector<std::string> privilege_mapping;
  if (!GetPrivilegeMapping(privilege_name, api_version, &privilege_mapping)) {
    LOG(ERROR) << "Failed to acquire mapping for privilege: "
               << privilege_name.c_str();
    return false;
  }

  for (const auto& str : privilege_mapping) {
    ret = cynara_check(p_cynara, smack_label.c_str(), "", uid.c_str(),
                       str.c_str());
    if (ret != CYNARA_API_ACCESS_ALLOWED) {
      return false;
    }
  }
  return true;
#else  // defined(OS_TIZEN)
  ALLOW_UNUSED_LOCAL(privilege_name);
  return false;
#endif
}
}  // namespace content
