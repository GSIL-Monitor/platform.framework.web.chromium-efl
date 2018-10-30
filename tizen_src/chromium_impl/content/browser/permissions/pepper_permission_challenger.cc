/*
 * Copyright (c) 2016 Samsung Electronics, Visual Display Division.
 * All Rights Reserved.
 *
 * @author  Michal Jurkiewicz <m.jurkiewicz@samsung.com>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium-LICENSE file.
 */

#include "content/browser/permissions/pepper_permission_challenger.h"

#include <vconf/vconf.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "content/browser/zygote_host/zygote_communication_linux.h"
#include "content/public/browser/zygote_handle_linux.h"
#include "ewk_privilege_checker.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "ppapi/shared_impl/ppapi_switches.h"
#if BUILDFLAG(ENABLE_NACL)
#include "components/nacl/common/nacl_switches.h"
#endif

namespace {
const char kTestingTizenPrivilegeName[] =
    "http://tizen.org/privilege/appmanager.certificate";
const char kNaClNonSFITizenPrivilegeName[] =
    "http://developer.samsung.com/privilege/hostedapp_deviceapi_allow";
}  // namespace

namespace content {

PepperPermissionChallenger* PepperPermissionChallenger::GetInstance() {
  return base::Singleton<PepperPermissionChallenger>::get();
}

void PepperPermissionChallenger::PopulateSwitches() {
  std::vector<std::string> switches;

  AppendTestingSwitch(switches);
#if BUILDFLAG(ENABLE_NACL)
  AppendNaClDebugSwitch(switches);
  AppendNaClNonSFIVariantSwitch(switches);
#endif

  if (switches.empty())
    return;

  for (auto& new_switch : switches) {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(new_switch);
  }
  content::GetGenericZygote()->AppendCommandLineSwitches(switches);
}

void PepperPermissionChallenger::AppendTestingSwitch(
    std::vector<std::string>& switches) {
  bool result = EwkPrivilegeChecker::GetInstance()->CheckPrivilege(
      kTestingTizenPrivilegeName);

  if (result) {
    LOG(INFO) << "Granted testing privilege";
    switches.push_back(switches::kEnablePepperTesting);
  }
}

#if BUILDFLAG(ENABLE_NACL)
void PepperPermissionChallenger::AppendNaClDebugSwitch(
    std::vector<std::string>& switches) {
#if !defined(TIZEN_EMULATOR_SUPPORT)
  bool has_privilege = EwkPrivilegeChecker::GetInstance()->CheckPrivilege(
      kTestingTizenPrivilegeName);

  if (!has_privilege) {
    LOG(INFO) << "Testing privilege not granted";
    return;
  }
#endif  // !defined(TIZEN_EMULATOR_SUPPORT)

  static const char kNaClDebugVconfKey[] = "db/nacl/debug";
  int result = 0;
  if (vconf_get_bool(kNaClDebugVconfKey, &result) == 0 && result) {
    LOG(INFO) << "Enabling NaCl debug";
    switches.push_back(switches::kEnableNaClDebug);
  }
}

void PepperPermissionChallenger::AppendNaClNonSFIVariantSwitch(
    std::vector<std::string>& switches) {
#if !defined(TIZEN_EMULATOR_SUPPORT)
  // We do not support NaCl Non-SFI mode for x86 architecture
  bool result = EwkPrivilegeChecker::GetInstance()->CheckPrivilege(
      kNaClNonSFITizenPrivilegeName);

  if (result) {
    LOG(INFO) << "Enabling NaCl Non-SFI mode";
    switches.push_back(switches::kEnableNaClNonSfiMode);
  }
#endif  // !defined(TIZEN_EMULATOR_SUPPORT)
}
#endif  // BUILDFLAG(ENABLE_NACL)

}  // namespace content
