// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CLIENT_EFL
#define CONTENT_BROWSER_CLIENT_EFL

#include "components/nacl/common/features.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "ppapi/features/features.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/WebKit/public/web/WebWindowFeatures.h"

#if defined(OS_TIZEN)
#include <tts.h>
#endif

namespace base {
class CommandLine;
}

class EWebContext;
class Ewk_Notification;

namespace content {
class BrowserContextEfl;
class BrowserMainPartsEfl;
class NotificationControllerEfl;
class ResourceDispatcherHostDelegateEfl;
#if defined(TIZEN_WEB_SPEECH_RECOGNITION)
class SpeechRecognitionManagerDelegate;
#endif
class WebContents;
class WebContentsView;

class ContentBrowserClientEfl : public ContentBrowserClient {
 public:
  typedef void (*Notification_Show_Callback)(Ewk_Notification*, void*);
  typedef void (*Notification_Cancel_Callback)(uint64_t, void*);
  typedef base::Callback<void(const std::string&)> AcceptLangsChangedCallback;

  ContentBrowserClientEfl();
  ~ContentBrowserClientEfl() override;

  virtual BrowserMainParts* CreateBrowserMainParts(
      const MainFunctionParams& parameters) override;

  void GetQuotaSettings(
      content::BrowserContext* context,
      content::StoragePartition* partition,
      storage::OptionalQuotaSettingsCallback callback) override;

  // Allows the embedder to pass extra command line flags.
  // switches::kProcessType will already be set at this point.
  virtual void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                              int child_process_id) override;

  bool CanCreateWindow(RenderFrameHost* opener,
                       const GURL& opener_url,
                       const GURL& opener_top_level_frame_url,
                       const GURL& source_origin,
                       content::mojom::WindowContainerType container_type,
                       const GURL& target_url,
                       const Referrer& referrer,
                       const std::string& frame_name,
                       WindowOpenDisposition disposition,
                       const blink::mojom::WindowFeatures& features,
                       bool user_gesture,
                       bool opener_suppressed,
                       bool* no_javascript_access) override;

  virtual void ResourceDispatcherHostCreated() override;
#if defined(TIZEN_WEB_SPEECH_RECOGNITION)
  SpeechRecognitionManagerDelegate* CreateSpeechRecognitionManagerDelegate()
      override;
#endif

  QuotaPermissionContext* CreateQuotaPermissionContext() override;

  virtual void AllowCertificateError(
      WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      ResourceType resource_type,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(CertificateRequestResultType)>& callback)
      override;

  virtual PlatformNotificationService* GetPlatformNotificationService()
      override;

  NotificationControllerEfl* GetNotificationController() const;

  virtual bool AllowGetCookie(const GURL& url,
                              const GURL& first_party,
                              const net::CookieList& cookie_list,
                              content::ResourceContext* context,
                              int render_process_id,
                              int render_frame_id) override;

  virtual bool AllowSetCookie(const GURL& url,
                              const GURL& first_party,
                              const std::string& cookie_line,
                              content::ResourceContext* context,
                              int render_process_id,
                              int render_frame_id,
                              const net::CookieOptions& options) override;

  virtual void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                                   content::WebPreferences* prefs) override;

  virtual void RenderProcessWillLaunch(
      content::RenderProcessHost* host) override;

  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;

#if BUILDFLAG(ENABLE_NACL)
  content::BrowserPpapiHost* GetExternalBrowserPpapiHost(int plugin_process_id) override;
#endif

#if BUILDFLAG(ENABLE_NACL) || \
    (defined(TIZEN_PEPPER_EXTENSIONS) && BUILDFLAG(ENABLE_PLUGINS))
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
#endif

  std::string GetApplicationLocale() override;

  WebContentsViewDelegate* GetWebContentsViewDelegate(
      WebContents* web_contents) override;

  void BindInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle interface_pipe) override;
  std::vector<std::unique_ptr<content::NavigationThrottle>>
  CreateThrottlesForNavigation(content::NavigationHandle* handle) override;

  void SetBrowserContext(BrowserContext* context);

#if defined(OS_TIZEN)
  void SetTTSMode(tts_mode_e mode) { tts_mode_ = mode; }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  void AddDynamicCertificatePath(const std::string& host,
                                 const std::string& cert_path) override;
  void SelectClientCertificate(
      WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList client_certs,
      std::unique_ptr<ClientCertificateDelegate> delegate) override;

  void SetWebSDI(bool enable) { enabled_websdi_ = enable; }
  bool GetWebSDI() const { return enabled_websdi_; }
  void GetGeolocationRequestContext(
      base::OnceCallback<void(scoped_refptr<net::URLRequestContextGetter>)>
          callback) override;
#endif
  void CleanUpBrowserContext();

#if defined(TIZEN_PEPPER_EXTENSIONS)
  bool AllowPepperSocketAPI(BrowserContext* browser_context,
                            const GURL& url,
                            bool private_api,
                            const SocketPermissionRequest* params) override;
  bool IsPluginAllowedToUseDevChannelAPIs(BrowserContext* browser_context,
                                          const GURL& url) override;
#endif

#if defined(OS_TIZEN)
  void OpenURLInTab(
      const OpenURLParams& params,
      int worker_process_id,
      const base::Callback<void(WebContents*)>& callback) override;
#endif
  std::string GetAcceptLangs(BrowserContext* context) override;
  void SetAcceptLangs(const std::string& accept_langs);
  void SetPreferredLangs(const std::string& preferred_langs);
  void AddAcceptLangsChangedCallback(
      const AcceptLangsChangedCallback& callback);
  void RemoveAcceptLangsChangedCallback(
      const AcceptLangsChangedCallback& callback);

  std::vector<ServiceManifestInfo> GetExtraServiceManifests() override;
  void EnableAppControl(bool enabled) { enabled_app_control = enabled; }

 private:
  static void DispatchPopupBlockedOnUIThread(int render_process_id,
                                             int opener_render_view_id,
                                             const GURL& target_url);
#if defined(OS_TIZEN_TV_PRODUCT)
  void GetRequestContextOnUIThread();
  void RespondOnOriginatingThread(
      base::OnceCallback<void(scoped_refptr<net::URLRequestContextGetter>)>
          callback);
#endif
  void AppendInjectedBundleCommandLine(const EWebContext&, base::CommandLine*);
  void AppendExtraCommandLineSwitchesInternal(base::CommandLine*, int);

  void InitFrameInterfaces();

  void NotifyAcceptLangsChanged();

  std::unique_ptr<ResourceDispatcherHostDelegateEfl>
      resource_disp_host_del_efl_;
  std::unique_ptr<
      service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>>
      frame_interfaces_;
  BrowserMainPartsEfl* browser_main_parts_efl_;
  std::unique_ptr<content::BrowserContext> browser_context_;

  std::unique_ptr<NotificationControllerEfl> notification_controller_;
#if defined(OS_TIZEN_TV_PRODUCT)
  bool enabled_websdi_;
  net::URLRequestContextGetter* system_request_context_;
#endif
  std::string accept_langs_;
  std::string preferred_langs_;
  std::vector<AcceptLangsChangedCallback> accept_langs_changed_callbacks_;
  bool shutting_down_;

#if defined(OS_TIZEN)
  tts_mode_e tts_mode_;
#endif
  bool enabled_app_control;

  DISALLOW_COPY_AND_ASSIGN(ContentBrowserClientEfl);
};
}

#endif  // CONTENT_BROWSER_CLIENT_EFL
