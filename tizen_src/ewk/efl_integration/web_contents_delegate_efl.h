// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_CONTENTS_DELEGATE_EFL
#define WEB_CONTENTS_DELEGATE_EFL

#include <deque>

#include "browser/favicon/favicon_downloader.h"
#include "browser/javascript_dialog_manager_efl.h"
#include "browser_context_efl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/frame_navigate_params.h"
#include "content/public/common/manifest.h"
#include "eweb_view.h"
#include "private/ewk_manifest_private.h"
#include "public/ewk_view_internal.h"
#include "ui/base/ime/text_input_type.h"
#include "url/gurl.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "public/ewk_autofill_profile_product.h"
#endif
class DidPrintPagesParams;
class Ewk_Wrt_Message_Data;
class JavaScriptDialogManagerEfl;
class LoginDelegateEfl;

namespace content {
struct DateTimeSuggestion;

class WebContentsDelegateEfl : public WebContentsDelegate,
                               public WebContentsObserver {
 public:
  WebContentsDelegateEfl(EWebView*);
  ~WebContentsDelegateEfl();

  // Overridden from content::WebContentsDelegate:
  void AddNewContents(WebContents* source,
                      WebContents* new_contents,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override;

  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  void NavigationStateChanged(content::WebContents* source,
                              content::InvalidateTypes changed_flags) override;
  void LoadProgressChanged(WebContents* source, double progress) override;

  bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      int32_t route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;
  void CloseContents(content::WebContents* source) override;
  void NotifyUrlForPlayingVideo(const GURL& url) override;
  void EnterFullscreenModeForTab(content::WebContents* web_contents,
                                 const GURL& origin) override;
  void ExitFullscreenModeForTab(content::WebContents* web_contents) override;
  bool IsFullscreenForTabOrPending(
      const content::WebContents* web_contents) const override;

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  virtual bool CheckMediaAccessPermission(WebContents* web_contents,
                                          const GURL& security_origin,
                                          MediaStreamType type) override;

  virtual void RequestMediaAccessPermission(
      WebContents* web_contents,
      const MediaStreamRequest& request,
      const MediaResponseCallback& callback) override;

  void RequestMediaAccessAllow(const MediaStreamRequest& request,
                               const MediaResponseCallback& callback);

  void RequestMediaAccessDeny(const MediaStreamRequest& request,
                              const MediaResponseCallback& callback);
#endif

  void FindReply(content::WebContents* web_contents,
                 int request_id,
                 int number_of_matches,
                 const gfx::Rect& selection_rect,
                 int active_match_ordinal,
                 bool final_update) override;

  void DidRenderFrame() override;
  void DidRenderRotatedFrame(int rotation,
                             int frame_data_output_rect_width) override;
  void QueryNumberFieldAttributes() override;
  void WillImePanelShow() override;

  void DidTopControlsMove(int offset) override;
  void SetTooltipText(const base::string16& text) override;

  void RequestCertificateConfirm(
      WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& url,
      ResourceType resource_type,
      bool strict_enforcement,
      const base::Callback<void(CertificateRequestResultType)>& callback);

  JavaScriptDialogManager* GetJavaScriptDialogManager(
      WebContents* source) override;
  void OnDidChangeInputType(bool is_password_input);

  void ActivateContents(WebContents* contents) override;

  void OnUpdateSettings(const Ewk_Settings* settings);
  void SetContentSecurityPolicy(const std::string& policy,
                                Ewk_CSP_Header_Type header_type);

  void DidFirstVisuallyNonEmptyPaint() override;
  void DidStartLoading() override;

  // Invoked if an IPC message is coming from a specific RenderFrameHost.
  bool OnMessageReceived(const IPC::Message& message,
                         RenderFrameHost* render_frame_host) override;
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnPrintedMetafileReceived(const DidPrintPagesParams& params);
  void NavigationEntryCommitted(
      const LoadCommittedDetails& load_details) override;
  void RenderViewCreated(RenderViewHost* render_view_host) override;
  void RenderProcessGone(base::TerminationStatus status) override;
  bool DidAddMessageToConsole(WebContents* source,
                              int32_t level,
                              const base::string16& message,
                              int32_t line_no,
                              const base::string16& source_id) override;
  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      const FileChooserParams& params) override;
  ColorChooser* OpenColorChooser(
      WebContents* web_contents,
      SkColor color,
      const std::vector<ColorSuggestion>& suggestions) override;
  void OpenDateTimeDialog(
      ui::TextInputType dialog_type,
      double dialog_value,
      double min,
      double max,
      double step,
      const std::vector<DateTimeSuggestion>& suggestions) override;
  bool PreHandleGestureEvent(WebContents* source,
                             const blink::WebGestureEvent& event) override;

  void VisibleSecurityStateChanged(WebContents* source) override;

  // WebContentsObserver---------------------------------------------------
  void DidStartNavigation(NavigationHandle* navigation_handle) override;

  void DidRedirectNavigation(NavigationHandle* navigation_handle) override;

  void DidFinishNavigation(NavigationHandle* navigation_handle) override;

  void DidFailProvisionalLoad(
      RenderFrameHost* render_frame_host,
      const GURL& validated_url,
      int error_code,
      const base::string16& error_description) override;

  void DidFinishLoad(RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  void DidChangeThemeColor(SkColor color) override;

  void SetReaderMode(Eina_Bool enable);

  void OnRecognizeArticleResult(bool is_article,
                                const base::string16& page_url) override;
  void DidFailLoad(RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code,
                   const base::string16& error_description) override;

  void DidUpdateFaviconURL(const std::vector<FaviconURL>& candidates) override;

  void TitleWasSet(content::NavigationEntry* entry) override;

  void OnAuthRequired(net::URLRequest* request,
                      const std::string& realm,
                      LoginDelegateEfl* login_delegate);

  void DidDownloadFavicon(bool success,
                          const GURL& icon_url,
                          const SkBitmap& bitmap);

  EWebView* web_view() const { return web_view_; }
  WebContents& web_contents() const { return web_contents_; }

  void RequestManifestInfo(Ewk_View_Request_Manifest_Callback callback,
                           void* user_data);

  void OnRequestSelectCollectionInformationUpdateACK(int form_element_count,
                                                     int current_node_index,
                                                     bool prev_state,
                                                     bool nest_state);
  void OnDidChangeMaxScrollOffset(int max_scroll_x, int max_scroll_y);
  void OnDidChangeScrollOffset(int scroll_x, int scroll_y);
  void OnFormSubmit(const GURL& url);
  void OnDidNotAllowScript();
#if defined(TIZEN_AUTOFILL_SUPPORT)
  void UpdateAutofillIfRequired();
#endif
  void OnDidChangeFocusedNodeBounds(const gfx::RectF& focused_node_bounds);

#if defined(OS_TIZEN_TV_PRODUCT)
  bool GetMarlinEnable() override;
  const std::string& GetActiveDRM() override;
  void UpdateCurrentTime(double current_time) override;
  void UpdateTargetURL(WebContents* source, const GURL& url) override;
  void NotifyPlayingVideoRect(const gfx::RectF& rect);
  void NotifySubtitleState(int state, double time_stamp) override;
  void NotifySubtitlePlay(int active_track_id,
                          const std::string& url,
                          const std::string& lang) override;
  void NotifySubtitleData(int track_id,
                          double time_stamp,
                          const std::string& data,
                          unsigned int size) override;

  void NotifyPlaybackState(int state,
                           const std::string& url,
                           const std::string& mime_type,
                           bool* media_resource_acquired,
                           std::string* translated_url,
                           std::string* drm_info) override;
  void NotifyParentalRatingInfo(const std::string& info,
                                const std::string& url) override;
  bool IsHighBitRate() override;
  void OnLoginFields(const int type,
                     const std::string& id,
                     const std::string& username_element,
                     const std::string& password_element,
                     const std::string& action_url);
  void OnLoginFormSubmitted(const std::string& id,
                            const std::string& pw,
                            const std::string& username_element,
                            const std::string& password_element,
                            const std::string& action_url);
  void AutoLogin(const std::string& user_name, const std::string& password);
#endif

#if defined(TIZEN_ATK_FEATURE_VD)
  void OnMoveFocusToBrowser(int direction);
#endif

#if defined(TIZEN_TBM_SUPPORT)
  bool OffscreenRenderingEnabled() override;
  void SetOffscreenRendering(bool enable) override;
  void DidRenderOffscreenFrame(void* buffer) override;
#endif

 private:
  void ShowRepostFormWarningDialog(content::WebContents* source) override;
  void OnGetContentSecurityPolicy(IPC::Message* reply_msg);
  void OnWrtPluginMessage(const Ewk_Wrt_Message_Data& data);
  void OnWrtPluginSyncMessage(const Ewk_Wrt_Message_Data& data,
                              IPC::Message* reply);
  void OnHandleTapGestureWithContext(bool is_content_editable);
  void OnDidChangeNumberFieldAttributes(bool is_minimum_negative,
                                        bool is_step_integer);
  void OnDidGetManifest(Ewk_View_Request_Manifest_Callback callback,
                        void* user_data,
                        const GURL& manifest_url,
                        const Manifest& manifest);
  void OnIsVideoPlayingGet(bool is_playing, int callback_id);

#if defined(OS_TIZEN_TV_PRODUCT)
  void ShowInspectorPortInfo();
#endif

  EWebView* web_view_;
  bool is_fullscreen_;
  WebContents& web_contents_;
#if defined(OS_TIZEN_TV_PRODUCT)
  std::string last_hovered_url_;
  std::unique_ptr<_Ewk_Form_Info> form_info_;
  bool is_auto_login_;
#endif

  bool did_render_frame_;
  bool did_first_visually_non_empty_paint_;

  struct ContentSecurityPolicy {
    ContentSecurityPolicy(const std::string& p, Ewk_CSP_Header_Type type)
        : policy(p), header_type(type) {}
    std::string policy;
    Ewk_CSP_Header_Type header_type;
  };

  GURL last_visible_url_;

  std::unique_ptr<ContentSecurityPolicy> pending_content_security_policy_;
  bool document_created_;
  JavaScriptDialogManagerEfl* dialog_manager_;
  int rotation_;
  int frame_data_output_rect_width_;
  std::unique_ptr<FaviconDownloader> favicon_downloader_;
  bool reader_mode_;
#if defined(TIZEN_TBM_SUPPORT)
  bool offscreen_rendering_enabled_;
#endif
  base::WeakPtrFactory<WebContentsDelegateEfl> weak_ptr_factory_;
  std::unique_ptr<_Ewk_Certificate_Policy_Decision>
      certificate_policy_decision_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsDelegateEfl);
};
}

#endif
