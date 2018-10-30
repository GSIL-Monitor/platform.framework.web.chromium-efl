// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/content_client_efl.h"

#include "common/version_info.h"
#include "ipc/ipc_message.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/l10n/l10n_util.h"

#include "ppapi/features/features.h"

#if BUILDFLAG(ENABLE_PLUGINS)
#include "content/public/common/pepper_plugin_info.h"
#endif

#if BUILDFLAG(ENABLE_PLUGINS) && defined(TIZEN_PEPPER_EXTENSIONS)
#include "base/command_line.h"
#include "common/trusted_pepper_plugin_info_cache.h"
#include "common/trusted_pepper_plugin_util.h"
#include "content/public/common/content_switches.h"
#endif

#if BUILDFLAG(ENABLE_NACL)
#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "components/nacl/common/nacl_constants.h"
#include "components/nacl/common/nacl_process_type.h"
#include "content/common/paths_efl.h"
#include "components/nacl/renderer/plugin/ppapi_entrypoints.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#endif

#if BUILDFLAG(ENABLE_NACL) || (BUILDFLAG(ENABLE_PLUGINS) && defined(TIZEN_PEPPER_EXTENSIONS))
#include "base/command_line.h"
#endif

#if defined(TIZEN_PEPPER_EXTENSIONS)
namespace {
inline bool IsPepperPluginProcess() {
  auto& command_line = *base::CommandLine::ForCurrentProcess();
  auto type = command_line.GetSwitchValueASCII(switches::kProcessType);
  return type == switches::kPpapiPluginProcess;
}
}  // namespace
#endif

/* LCOV_EXCL_START */
std::string ContentClientEfl::GetProduct() const {
  return EflWebView::VersionInfo::GetInstance()->VersionForUserAgent();
}
/* LCOV_EXCL_STOP */

std::string ContentClientEfl::GetUserAgent() const {
  return EflWebView::VersionInfo::GetInstance()->DefaultUserAgent();
}

/* LCOV_EXCL_START */
base::string16 ContentClientEfl::GetLocalizedString(int message_id) const {
  // TODO(boliu): Used only by WebKit, so only bundle those resources for
  // Android WebView.
  return l10n_util::GetStringUTF16(message_id);
}
/* LCOV_EXCL_STOP */

base::StringPiece ContentClientEfl::GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const {
  // TODO(boliu): Used only by WebKit, so only bundle those resources for
  // Android WebView.
  return ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
    resource_id, scale_factor);
}

base::RefCountedMemory* ContentClientEfl::GetDataResourceBytes(
    int resource_id) const {
  return ResourceBundle::GetSharedInstance().LoadDataResourceBytes(resource_id);
}

/* LCOV_EXCL_START */
bool ContentClientEfl::CanSendWhileSwappedOut(const IPC::Message* message) {
  // For legacy API support we perform a few browser -> renderer synchronous IPC
  // messages that block the browser. However, the synchronous IPC replies might
  // be dropped by the renderer during a swap out, deadlocking the browser.
  // Because of this we should never drop any synchronous IPC replies.
  return message->type() == IPC_REPLY_ID;
}
/* LCOV_EXCL_STOP */
void ContentClientEfl::AddPepperPlugins(
    std::vector<content::PepperPluginInfo>* plugins) {
#if BUILDFLAG(ENABLE_PLUGINS) && defined(TIZEN_PEPPER_EXTENSIONS)
  DCHECK(plugins);
  if (!IsPepperPluginProcess() && !pepper::AreTrustedPepperPluginsDisabled())
    pepper::TrustedPepperPluginInfoCache::GetInstance()->GetPlugins(plugins);
#endif

#if BUILDFLAG(ENABLE_NACL)
  base::FilePath path;
  if (PathService::Get(PathsEfl::FILE_NACL_PLUGIN, &path)) {
    content::PepperPluginInfo nacl;

    // The nacl plugin is now built into the binary.
    nacl.is_internal = true;
    nacl.path = path;
    nacl.name = nacl::kNaClPluginName;
    content::WebPluginMimeType nacl_mime_type(nacl::kNaClPluginMimeType,
                                              nacl::kNaClPluginExtension,
                                              nacl::kNaClPluginDescription);
    nacl.mime_types.push_back(nacl_mime_type);
    content::WebPluginMimeType pnacl_mime_type(nacl::kPnaclPluginMimeType,
                                               nacl::kPnaclPluginExtension,
                                               nacl::kPnaclPluginDescription);
    nacl.mime_types.push_back(pnacl_mime_type);
    nacl.internal_entry_points.get_interface = nacl_plugin::PPP_GetInterface;
    nacl.internal_entry_points.initialize_module =
        nacl_plugin::PPP_InitializeModule;
    nacl.internal_entry_points.shutdown_module =
        nacl_plugin::PPP_ShutdownModule;
    nacl.permissions = ppapi::PERMISSION_PRIVATE | ppapi::PERMISSION_DEV;
    plugins->push_back(nacl);
  }
#endif
}

#if BUILDFLAG(ENABLE_NACL)
std::string ContentClientEfl::GetProcessTypeNameInEnglish(int type) {
  switch (type) {
    case PROCESS_TYPE_NACL_LOADER:
      return "Native Client module";
    case PROCESS_TYPE_NACL_BROKER:
      return "Native Client broker";
  }

  NOTREACHED() << "Unknown child process type!";
  return "Unknown";
}
#endif
