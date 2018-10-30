// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/webui/version_ui_efl.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "common/url_constants_efl.h"
// TODO: consider using common/content_client_efl.h
#include "common/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/user_agent.h"
#include "content/public/common/webplugininfo.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/resources/grit/webui_resources_tizen.h"
#include "v8/include/v8.h"

namespace {

// Retrieves the executable path on the FILE thread.
void GetExecutablePath(base::string16* exec_path_out) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::FILE);

  base::FilePath executable_path = base::MakeAbsoluteFilePath(
      base::CommandLine::ForCurrentProcess()->GetProgram());
  *exec_path_out =
      !executable_path.empty()
          ? executable_path.LossyDisplayName()
          : l10n_util::GetStringUTF16(IDS_ABOUT_VERSION_PATH_NOTFOUND);
  ;
}

}  // namespace

class VersionHandlerEfl : public content::WebUIMessageHandler {
 public:
  VersionHandlerEfl() : weak_ptr_factory_(this) {}
  ~VersionHandlerEfl() override {}

  // content::WebUIMessageHandler implementation.
  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "requestVersionInfo",
        base::Bind(&VersionHandlerEfl::HandleRequestVersionInfo,
                   base::Unretained(this)));
  }

  // Callback for the "requestVersionInfo" message. This asynchronously
  // requests executable path and returns it later.
  virtual void HandleRequestVersionInfo(const base::ListValue* args) {
    // Grab the executable path on the FILE thread. It is returned in
    // OnGotExecutablePath.
    base::string16* exec_path_buffer = new base::string16;
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::FILE, FROM_HERE,
        base::Bind(&GetExecutablePath, base::Unretained(exec_path_buffer)),
        base::Bind(&VersionHandlerEfl::OnGotExecutablePath,
                   weak_ptr_factory_.GetWeakPtr(),
                   base::Owned(exec_path_buffer)));
  }

 private:
  // Callback which handles returning the executable path to the front end.
  void OnGotExecutablePath(base::string16* executable_path_data) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    base::Value exec_path(*executable_path_data);
    CallJavascriptFunction("returnExecutablePath", exec_path);
  }

  // Factory for the creating refs in callbacks.
  base::WeakPtrFactory<VersionHandlerEfl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VersionHandlerEfl);
};

using content::WebUIDataSource;
using EflWebView::VersionInfo;

namespace {

std::string ChannelToString(VersionInfo::Channel channel) {
  switch (channel) {
    case VersionInfo::CHANNEL_STABLE:
      return "(stable)";
    case VersionInfo::CHANNEL_BETA:
      return "(beta)";
    case VersionInfo::CHANNEL_DEV:
      return "(dev)";
    case VersionInfo::CHANNEL_CANARY:
      return "(canary)";
    case VersionInfo::CHANNEL_UNKNOWN:
    default:
      return "(unknown)";
  }
}

WebUIDataSource* CreateVersionUIEflDataSource() {
  WebUIDataSource* html_source =
      WebUIDataSource::Create(kChromeUIVersionEflHost);

  // Localizations are not available now.
  html_source->AddLocalizedString("title", IDS_ABOUT_VERSION_TITLE);

  // Our VersionInfo should be able to get application name at runtime with
  // VersionInfo::AppName, but it seems that no one sets it. If it is empty use
  // chromium's hardcoded IDS_PRODUCT_NAME.
  std::string app_name = VersionInfo::GetInstance()->AppName();
  if (app_name.empty())
    html_source->AddLocalizedString("application_label", IDS_PRODUCT_NAME);
  else
    html_source->AddString("application_label", app_name);

  html_source->AddString("version",
                         VersionInfo::GetInstance()->GetVersionNumber());
  html_source->AddString("version_modifier",
                         ChannelToString(VersionInfo::GetChannel()));

  html_source->AddLocalizedString("os_name", IDS_ABOUT_VERSION_OS);
  html_source->AddString("os_type", VersionInfo::GetInstance()->OSType());
  html_source->AddString("blink_version", content::GetWebKitVersion());
  html_source->AddString("js_engine", "V8");
  html_source->AddString("js_version", v8::V8::GetVersion());

  html_source->AddLocalizedString("company", IDS_ABOUT_VERSION_COMPANY_NAME);
  base::Time::Exploded exploded_time;
  base::Time::Now().LocalExplode(&exploded_time);
  html_source->AddString(
      "copyright",
      l10n_util::GetStringFUTF16(IDS_ABOUT_VERSION_COPYRIGHT,
                                 base::IntToString16(exploded_time.year)));
  html_source->AddLocalizedString("revision", IDS_ABOUT_VERSION_REVISION);
  // Getting last commit id is not implemented now.
  html_source->AddString("cl", "N/A");
#if defined(ARCH_CPU_64_BITS)
  html_source->AddLocalizedString("version_bitsize", IDS_ABOUT_VERSION_64BIT);
#else
  html_source->AddLocalizedString("version_bitsize", IDS_ABOUT_VERSION_32BIT);
#endif
  html_source->AddLocalizedString("user_agent_name",
                                  IDS_ABOUT_VERSION_USER_AGENT);
  html_source->AddString("useragent",
                         VersionInfo::GetInstance()->DefaultUserAgent());
  html_source->AddLocalizedString("command_line_name",
                                  IDS_ABOUT_VERSION_COMMAND_LINE);
  std::string command_line;
  typedef std::vector<std::string> ArgvList;
  const ArgvList& argv = base::CommandLine::ForCurrentProcess()->argv();
  for (ArgvList::const_iterator iter = argv.begin(); iter != argv.end(); iter++)
    command_line += " " + *iter;
  html_source->AddString("command_line", command_line);

  // Executable path is retrieved asynchronously.
  html_source->AddLocalizedString("executable_path_name",
                                  IDS_ABOUT_VERSION_EXECUTABLE_PATH);
  html_source->AddString("executable_path", std::string());

  html_source->SetJsonPath("strings.js");
  html_source->AddResourcePath("about_version.js", IDR_VERSION_UI_EFL_JS);
  html_source->AddResourcePath("about_version.css", IDR_VERSION_UI_EFL_CSS);
  html_source->SetDefaultResource(IDR_VERSION_UI_EFL_HTML);
  return html_source;
}

}  // namespace

VersionUIEfl::VersionUIEfl(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<VersionHandlerEfl>());
  content::BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();
  WebUIDataSource::Add(browser_context, CreateVersionUIEflDataSource());
}

VersionUIEfl::~VersionUIEfl() {}
