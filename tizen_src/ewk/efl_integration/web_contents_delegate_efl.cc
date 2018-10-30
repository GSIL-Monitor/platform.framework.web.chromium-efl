// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web_contents_delegate_efl.h"

#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/ttrace.h"
#include "browser/favicon/favicon_database.h"
#include "browser/input_picker/color_chooser_efl.h"
#include "browser/javascript_dialog_manager_efl.h"
#include "browser/policy_response_delegate_efl.h"
#include "common/render_messages_ewk.h"
#include "components/favicon_base/favicon_types.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/browser/web_contents/web_contents_impl_efl.h"
#include "content/common/content_switches_internal.h"
#include "content/common/date_time_suggestion.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/favicon_url.h"
#include "eweb_view.h"
#include "eweb_view_callbacks.h"
#include "net/base/load_states.h"
#include "net/cert/x509_certificate.h"
#include "net/http/http_response_headers.h"
#include "printing/pdf_metafile_skia.h"
#include "private/ewk_certificate_private.h"
#include "private/ewk_console_message_private.h"
#include "private/ewk_error_private.h"
#include "private/ewk_form_repost_decision_private.h"
#include "private/ewk_policy_decision_private.h"
#include "private/ewk_user_media_private.h"
#include "private/webview_delegate_ewk.h"
#include "public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "url/gurl.h"

#if defined(OS_TIZEN)
#include <app_control.h>
#include <app_manager.h>
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include "browser/mixed_content_observer.h"
#include "browser/scroll_client_observer.h"
#include "content/browser/renderer_host/web_event_factory_efl.h"
#include "devtools_port_manager.h"
#include "private/ewk_autofill_profile_private.h"
#include "private/ewk_context_private.h"
#endif

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
#include "content/public/browser/media_capture_devices.h"
#include "media/capture/video/tizen/video_capture_device_tizen.h"
#endif

#if defined(TIZEN_AUTOFILL_SUPPORT)
#include "browser/autofill/autofill_client_efl.h"
#include "browser/password_manager/password_manager_client_efl.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"

using autofill::AutofillManager;
using autofill::AutofillClientEfl;
using autofill::ContentAutofillDriverFactory;
using password_manager::PasswordManagerClientEfl;
#endif

using base::string16;
using namespace ui;

namespace content {

/* LCOV_EXCL_START */
void WritePdfDataToFile(printing::PdfMetafileSkia* metafile,
                        const base::FilePath& filename) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  DCHECK(metafile);
  base::File file(filename,
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  metafile->SaveTo(&file);
  file.Close();
  delete metafile;
}
/* LCOV_EXCL_STOP */

static bool IsMainFrame(RenderFrameHost* render_frame_host) {
  return !render_frame_host->GetParent();
}

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
static const content::MediaStreamDevice* GetRequestedAudioDevice(
    const std::string& device_id) {
  const content::MediaStreamDevices& audio_devices =
      MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices();
  if (audio_devices.empty())
    return NULL;
  if (device_id.length() == 0)
    return &(*audio_devices.begin());

  for (content::MediaStreamDevices::const_iterator i = audio_devices.begin();
       i != audio_devices.end(); i++) {
    if (i->id.compare(device_id) == 0)
      return &(*i);
  }

  NOTREACHED();
  return NULL;
}

static const content::MediaStreamDevice* GetRequestedVideoDevice(
    const std::string& device_id) {
  const content::MediaStreamDevices& video_devices =
      MediaCaptureDevices::GetInstance()
          ->GetVideoCaptureDevices();  // LCOV_EXCL_LINE
  if (video_devices.empty())           // LCOV_EXCL_LINE
    return NULL;
  if (device_id.length() == 0)  // LCOV_EXCL_LINE
    return &(*video_devices.begin());

  /* LCOV_EXCL_START */
  for (content::MediaStreamDevices::const_iterator i = video_devices.begin();
       i != video_devices.end(); i++) {
    if (i->id.compare(device_id) == 0)
      return &(*i);
    /* LCOV_EXCL_STOP */
  }

  NOTREACHED();
  return NULL;
}
#endif

WebContentsDelegateEfl::WebContentsDelegateEfl(EWebView* view)
    : WebContentsObserver(&view->web_contents()),
      web_view_(view),
      is_fullscreen_(false),
      web_contents_(view->web_contents()),
#if defined(OS_TIZEN_TV_PRODUCT)
      is_auto_login_(false),
#endif
      did_render_frame_(false),
      did_first_visually_non_empty_paint_(false),
      document_created_(false),
      dialog_manager_(NULL),
      rotation_(-1),
      frame_data_output_rect_width_(0),
      reader_mode_(false),
#if defined(TIZEN_TBM_SUPPORT)
      offscreen_rendering_enabled_(false),
#endif
      weak_ptr_factory_(this) {
#if defined(OS_TIZEN_TV_PRODUCT)
  MixedContentObserver::CreateForWebContents(&web_contents_);
  ScrollClientObserver::CreateForWebContents(&web_contents_);
#endif
  static_cast<WebContentsImplEfl*>(&web_contents_)
      ->set_ewk_view(web_view_->evas_object());

#if defined(TIZEN_AUTOFILL_SUPPORT)
  AutofillClientEfl::CreateForWebContents(&web_contents_);
  AutofillClientEfl* autofill_manager =
      AutofillClientEfl::FromWebContents(&web_contents_);
  autofill_manager->SetEWebView(view);
  PasswordManagerClientEfl::CreateForWebContentsWithAutofillClient(
      &web_contents_, autofill_manager);
  ContentAutofillDriverFactory::CreateForWebContentsAndDelegate(
      &web_contents_, autofill_manager, EWebView::GetPlatformLocale(),
      AutofillManager::DISABLE_AUTOFILL_DOWNLOAD_MANAGER);
#endif
}

WebContentsDelegateEfl::~WebContentsDelegateEfl() {
  // It's important to delete web_contents_ before dialog_manager_
  // destructor of web contents uses dialog_manager_

  delete dialog_manager_;
}

void WebContentsDelegateEfl::AddNewContents(
    WebContents* source, /* LCOV_EXCL_START */
    WebContents* new_contents,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture,
    bool* was_blocked) {
  CHECK(new_contents); /* LCOV_EXCL_START */
  // Initialize the delegate for the new contents here using source's delegate
  // as it will be needed to create a new window with the pre-created
  // new contents in case the opener is suppressed. Otherwise, the delegate
  // had already been initialized in EWebView::InitializeContent()
  /* LCOV_EXCL_START */
  if (!new_contents->GetDelegate())
    new_contents->SetDelegate(this);
}

WebContents* WebContentsDelegateEfl::OpenURLFromTab(
    WebContents* source,
    const content::OpenURLParams& params) {
  const GURL& url = params.url;
  WindowOpenDisposition disposition = params.disposition;

  if (!source || (disposition != WindowOpenDisposition::CURRENT_TAB &&
                  disposition != WindowOpenDisposition::NEW_FOREGROUND_TAB &&
                  disposition != WindowOpenDisposition::NEW_BACKGROUND_TAB &&
                  disposition != WindowOpenDisposition::OFF_THE_RECORD)) {
    NOTIMPLEMENTED();
    return NULL;
  }

  if (disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB ||
      disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB ||
      disposition == WindowOpenDisposition::OFF_THE_RECORD) {
    Evas_Object* new_object = NULL;
    if (IsMobileProfile() &&
        disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB) {
      web_view_->SmartCallback<EWebViewCallbacks::CreateNewBackgroundWindow>()
          .call(&new_object);
    } else {
      web_view_->SmartCallback<EWebViewCallbacks::CreateNewWindow>().call(
          &new_object);
    }

    // TODO(bj1987.kim): Mobile browser needs another callback signal with url.
    // It will be used at PWA scenario.
    web_view_->SmartCallback<EWebViewCallbacks::CreateNewWindowUrl>().call(
        url.possibly_invalid_spec().c_str());

    if (!new_object)
      return NULL;

    if (disposition == WindowOpenDisposition::NEW_FOREGROUND_TAB)
      ActivateContents(source);

    EWebView* wv =
        WebViewDelegateEwk::GetInstance().GetWebViewFromEvasObject(new_object);
    DCHECK(wv);
    wv->SetURL(url);
    return NULL;
  }

  ui::PageTransition transition(ui::PageTransitionFromInt(params.transition));
  source->GetController().LoadURL(url, params.referrer, transition,
                                  std::string());
  return source;
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::NavigationStateChanged(
    WebContents* source,
    InvalidateTypes changed_flags) {
  // We always notfiy clients about title invalidation, even if its text
  // didn't actually change. This is to maintain EWK API consistency.
  if (changed_flags & content::INVALIDATE_TYPE_TITLE) {
    web_view_->SmartCallback<EWebViewCallbacks::TitleChange>().
        call(base::UTF16ToUTF8(source->GetTitle()).c_str());
  }
  // We only notify clients if visible url actually changed, because on some
  // pages we would get notifications with flag INVALIDATE_TYPE_URL even when
  // visible url did not change.
#if defined(OS_TIZEN_TV_PRODUCT)
  if ((changed_flags & content::INVALIDATE_TYPE_URL) &&
      source->GetUserInputURLNeedNotify() &&
      last_visible_url_ == source->GetVisibleURL()) {
    const char* url = last_visible_url_.spec().c_str();
    web_view_->SmartCallback<EWebViewCallbacks::URLChanged>().call(url);
    web_view_->SmartCallback<EWebViewCallbacks::URIChanged>().call(url);
  }
  if (changed_flags & content::INVALIDATE_TYPE_URL)
    source->SetUserInputURLNeedNotify(false);
#endif

  if ((changed_flags & content::INVALIDATE_TYPE_URL) &&
      last_visible_url_ != source->GetVisibleURL()) {
    last_visible_url_ = source->GetVisibleURL();
    const char* url = last_visible_url_.spec().c_str();

    web_view_->SmartCallback<EWebViewCallbacks::URLChanged>().call(url);
    web_view_->SmartCallback<EWebViewCallbacks::URIChanged>().call(url);
    web_view_->ResetContextMenuController();
    auto rwhv = static_cast<RenderWidgetHostViewEfl*>(
        web_contents_.GetRenderWidgetHostView());
    if (rwhv) {
      auto selection_controller = rwhv->GetSelectionController();
      if (selection_controller)
        selection_controller->ClearSelection();
    }
  }
}

void WebContentsDelegateEfl::LoadProgressChanged(WebContents* source,
                                                 double progress) {
  web_view_->SetProgressValue(progress);
  web_view_->SmartCallback<EWebViewCallbacks::LoadProgress>().call(&progress);
}

/* LCOV_EXCL_START */
bool WebContentsDelegateEfl::ShouldCreateWebContents(
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
    content::SessionStorageNamespace* session_storage_namespace) {
  // We implement the asynchronous version of the function, this
  // one should never be invoked.
  NOTREACHED();
  return false;
}

void WebContentsDelegateEfl::CloseContents(WebContents* source) {
  web_view_->SmartCallback<EWebViewCallbacks::WindowClosed>().call();
}

void WebContentsDelegateEfl::NotifyUrlForPlayingVideo(const GURL& url) {
  web_view_->SmartCallback<EWebViewCallbacks::VideoPlayingURL>().call(
      url.spec().c_str());
}

#if defined(OS_TIZEN_TV_PRODUCT)
void WebContentsDelegateEfl::UpdateTargetURL(WebContents* /*source*/,
                                             const GURL& url) {
  std::string absolute_link_url(url.spec());
  if (absolute_link_url == last_hovered_url_)
    return;

  if (absolute_link_url.empty()) {
    // If length is 0, it is not link. send "hover,out,link" callback with the
    // original hovered URL.
    web_view_->SmartCallback<EWebViewCallbacks::HoverOutLink>().call(
        last_hovered_url_.c_str());
  } else {
    web_view_->SmartCallback<EWebViewCallbacks::HoverOverLink>().call(
        absolute_link_url.c_str());
  }

  // Update latest hovered url.
  last_hovered_url_ = absolute_link_url;
}

void WebContentsDelegateEfl::NotifyPlayingVideoRect(const gfx::RectF& rect) {
  Eina_Rectangle videoRect;
  videoRect.x = rect.x();
  videoRect.y = rect.y();
  videoRect.w = rect.width();
  videoRect.h = rect.height();

  web_view_->SmartCallback<EWebViewCallbacks::VideoResized>().call(
        static_cast<void*>(&videoRect));
}
#endif

void WebContentsDelegateEfl::EnterFullscreenModeForTab(
    content::WebContents* web_contents,
    const GURL& origin) {
  is_fullscreen_ = true;
  web_view_->SmartCallback<EWebViewCallbacks::EnterFullscreen>().call();
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::ExitFullscreenModeForTab(
    content::WebContents* web_contents) {
  is_fullscreen_ = false;
  web_view_->SmartCallback<EWebViewCallbacks::ExitFullscreen>().call();
}

bool WebContentsDelegateEfl::IsFullscreenForTabOrPending(
    const WebContents* web_contents) const {
  return is_fullscreen_;
}

/* LCOV_EXCL_START */
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
void WebContentsDelegateEfl::RequestMediaAccessAllow(
    const MediaStreamRequest& request,
    const MediaResponseCallback& callback) {
  MediaStreamDevices devices;

  if (request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE) {
    const content::MediaStreamDevice* audio_device =
        GetRequestedAudioDevice(request.requested_audio_device_id);
    if (audio_device) {
      devices.push_back(*audio_device);
    } else {
      callback.Run(MediaStreamDevices(), MEDIA_DEVICE_NOT_SUPPORTED,
                   std::unique_ptr<MediaStreamUI>());
    }
  }

  if (request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE) {
    const content::MediaStreamDevice* video_device =
        GetRequestedVideoDevice(request.requested_video_device_id);
    if (video_device) {
      devices.push_back(*video_device);
    } else {
      callback.Run(MediaStreamDevices(), MEDIA_DEVICE_NOT_SUPPORTED,
                   std::unique_ptr<MediaStreamUI>());
    }
  }

  callback.Run(devices, MEDIA_DEVICE_OK, std::unique_ptr<MediaStreamUI>());
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::RequestMediaAccessDeny(  // LCOV_EXCL_LINE
    const MediaStreamRequest& request,
    const MediaResponseCallback& callback) {
  /* LCOV_EXCL_START */
  LOG(ERROR) << __FUNCTION__ << " Decline request with empty list";
  callback.Run(MediaStreamDevices(), MEDIA_DEVICE_NOT_SUPPORTED,
               std::unique_ptr<MediaStreamUI>());
}
/* LCOV_EXCL_STOP */

bool WebContentsDelegateEfl::CheckMediaAccessPermission(  // LCOV_EXCL_LINE
    WebContents* web_contents,
    const GURL& security_origin,
    MediaStreamType type) {
  return true;  // LCOV_EXCL_LINE
}

void WebContentsDelegateEfl::RequestMediaAccessPermission(  // LCOV_EXCL_LINE
    WebContents* web_contents,
    const MediaStreamRequest& request,
    const MediaResponseCallback& callback) {
  std::unique_ptr<_Ewk_User_Media_Permission_Request> media_permission_request(
      new _Ewk_User_Media_Permission_Request(this, request,
                                             callback));  // LCOV_EXCL_LINE

  /* LCOV_EXCL_START */
  Eina_Bool callback_result = EINA_FALSE;
  if (!web_view_->InvokeViewUserMediaPermissionCallback(
          media_permission_request.get(), &callback_result)) {
    web_view_->SmartCallback<EWebViewCallbacks::UserMediaPermission>().call(
        media_permission_request.get());
    /* LCOV_EXCL_STOP */
  }

  // if policy is suspended, the API takes over the policy object lifetime
  // and policy will be deleted after decision is made
  /* LCOV_EXCL_START */
  if (media_permission_request->IsSuspended())
    ignore_result(media_permission_request.release());
  else if (!media_permission_request->IsDecided()) {
    callback.Run(MediaStreamDevices(), MEDIA_DEVICE_NOT_SUPPORTED,
                 std::unique_ptr<MediaStreamUI>());
    /* LCOV_EXCL_STOP */
  }
}  // LCOV_EXCL_LINE
#endif

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::OnAuthRequired(net::URLRequest* request,
                                            const std::string& realm,
                                            LoginDelegateEfl* login_delegate) {
  web_view_->InvokeAuthCallback(login_delegate, request->url(), realm);
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::DidStartNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame())
    return;

#if defined(TIZEN_ATK_SUPPORT)
  if (web_view_->CheckLazyInitializeAtk())
    web_view_->InitAtk();
#endif
  web_view_->SmartCallback<EWebViewCallbacks::ProvisionalLoadStarted>().call();
}

void WebContentsDelegateEfl::DidRedirectNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame())
    return;

  web_view_->SmartCallback<EWebViewCallbacks::ProvisionalLoadRedirect>().call();
}

void WebContentsDelegateEfl::DidFinishNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted())
    return;

  if (navigation_handle->IsInMainFrame()) {
#if defined(OS_TIZEN_TV_PRODUCT)
    if (!navigation_handle->IsSameDocument()) {
      auto rwhv = static_cast<RenderWidgetHostViewEfl*>(
          web_contents_.GetRenderWidgetHostView());
      if (rwhv)
        rwhv->DidFinishNavigation();
    }
#endif

    web_view_->SmartCallback<EWebViewCallbacks::LoadCommitted>().call();

    if (!navigation_handle->IsSameDocument() &&
        !navigation_handle->IsErrorPage())
      web_view_->SmartCallback<EWebViewCallbacks::LoadStarted>().call();
  } else if (!navigation_handle
                  ->HasSubframeNavigationEntryCommitted()) {  // LCOV_EXCL_LINE
    return;
  }

  if (navigation_handle->ShouldUpdateHistory())
    static_cast<BrowserContextEfl*>(web_contents_.GetBrowserContext())
        ->AddVisitedURLs(navigation_handle->GetRedirectChain());
}

void WebContentsDelegateEfl::DidFinishLoad(RenderFrameHost* render_frame_host,
                                           const GURL& validated_url) {
  if (!IsMainFrame(render_frame_host))
    return;

  TTRACE_WEB("WebContentsDelegateEfl::DidFinishLoad");
  web_view_->SmartCallback<EWebViewCallbacks::LoadFinished>().call();
#if defined(OS_TIZEN_TV_PRODUCT)
  // After load finish show inspector port info
  if (web_view_->context() && web_view_->context()->GetImpl() &&
      web_view_->context()->GetImpl()->ShowInspectorPortInfoState()) {
    if (!web_view_->RWIInfoShowed())
      ShowInspectorPortInfo();
  }
#endif

  if (IsMobileProfile())
    web_contents_.Focus();

  if (reader_mode_) {
    RenderViewHost* rvh = web_contents_.GetRenderViewHost();  // LCOV_EXCL_LINE
    DCHECK(rvh);
    /* LCOV_EXCL_START */
    Send(new ViewMsg_RecognizeArticle(
        rvh->GetRoutingID(), blink::WebFrame::SimpleNativeRecognitionMode));
    /* LCOV_EXCL_STOP */
  }
}

void WebContentsDelegateEfl::DidUpdateFaviconURL(
    const std::vector<FaviconURL>& candidates) {
  // select and set proper favicon
  for (unsigned int i = 0; i < candidates.size(); ++i) {
    FaviconURL favicon = candidates[i];
    if (favicon.icon_type == FaviconURL::IconType::kFavicon &&
        !favicon.icon_url.is_empty()) {
      NavigationEntry* entry = web_contents_.GetController().GetVisibleEntry();
      if (!entry)
        return;

      entry->GetFavicon().url = favicon.icon_url;
      entry->GetFavicon().valid = true;

      // check/update the url and favicon url in favicon database
      FaviconDatabase::Instance()->SetFaviconURLForPageURL(favicon.icon_url,
                                                           entry->GetURL());

      // download favicon if there is no such in database
      if (!FaviconDatabase::Instance()->ExistsForFaviconURL(favicon.icon_url)) {
        LOG(INFO) << "No favicon in database for URL: "
                  << favicon.icon_url.spec();
        favicon_downloader_.reset(new FaviconDownloader(
            &web_contents_, favicon.icon_url,
            base::Bind(&WebContentsDelegateEfl::DidDownloadFavicon,
                       weak_ptr_factory_.GetWeakPtr())));
        favicon_downloader_->Start();
      } else {
        web_view_->SmartCallback<EWebViewCallbacks::IconReceived>()
            .call();  // LCOV_EXCL_LINE
      }
      return;
    }
  }
}

void WebContentsDelegateEfl::DidDownloadFavicon(bool success,  // LCOV_EXCL_LINE
                                                const GURL& icon_url,
                                                const SkBitmap& bitmap) {
  /* LCOV_EXCL_START */
  favicon_downloader_.reset();
  if (success) {
    FaviconDatabase::Instance()->SetBitmapForFaviconURL(bitmap, icon_url);
    // emit "icon,received"
    web_view_->SmartCallback<EWebViewCallbacks::IconReceived>().call();
    /* LCOV_EXCL_STOP */
  }
}  // LCOV_EXCL_LINE

#if defined(OS_TIZEN_TV_PRODUCT)
void WebContentsDelegateEfl::ShowInspectorPortInfo() {
  std::string mappingInfo;
  devtools_http_handler::DevToolsPortManager::GetInstance()->GetMappingInfo(
      mappingInfo);
  if (web_view_->UseEarlyRWI()) {
    mappingInfo.append(
        "\r\nFast RWI is used, [about:blank] is loaded fist instead of \r\n[");
    mappingInfo.append(web_view_->RWIURL().spec());
    mappingInfo.append(
        "]\r\nClick OK button will start the real loading.\r\nNotes:\r\nPlease "
        "connect to RWI in PC before click OK button.\r\nThen you can get "
        "network log from the initial loading.\r\nPlease click Record button "
        "in Timeline panel in PC before click OK button,\r\nThen you can get "
        "profile log from the initial loading.");
  }

  LOG(INFO) << "[RWI] WebContentsDelegateEfl::ShowPortInfo mappingInfo = "
            << mappingInfo.c_str();
  if (!mappingInfo.empty()) {
    const GURL kGURL("");
    WebContentsImpl* wci = static_cast<WebContentsImpl*>(&web_contents_);
    IPC::Message* dummy_message = new IPC::Message;
    wci->RunJavaScriptDialog(wci->GetMainFrame(),
                             base::ASCIIToUTF16(mappingInfo),
                             base::ASCIIToUTF16("OK"), kGURL,
                             JAVASCRIPT_DIALOG_TYPE_ALERT, dummy_message);
  }
}
#endif

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::RequestCertificateConfirm(
    WebContents* /*web_contents*/,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& url,
    ResourceType resource_type,
    bool /*strict_enforcement*/,
    const base::Callback<void(CertificateRequestResultType)>& callback) {
  std::string pem_certificate;
  if (!net::X509Certificate::GetPEMEncoded(ssl_info.cert->os_cert_handle(), &pem_certificate)) {
    LOG(INFO) << "Certificate for URL: " << url.spec()
        << " could not be opened";
    callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_CANCEL);
    return;
  }
  certificate_policy_decision_.reset(new _Ewk_Certificate_Policy_Decision(
      url, pem_certificate, resource_type == RESOURCE_TYPE_MAIN_FRAME,
      cert_error, callback));

  web_view_->SmartCallback<EWebViewCallbacks::RequestCertificateConfirm>().call(
      certificate_policy_decision_.get());
  LOG(INFO) << "Certificate policy decision called for URL: " << url.spec()
      << " with cert_error: " << cert_error;

  // if policy is suspended, the API takes over the policy object lifetime
  // and policy will be deleted after decision is made
  if (certificate_policy_decision_->IsSuspended()) {
    ignore_result(certificate_policy_decision_.release());
  } else if (!certificate_policy_decision_->IsDecided()) {
    // When the embeder neither suspended certificate decision nor handled it
    // inside the smart callback by calling
    // ewk_certificate_policy_decision_allowed_set
    // this means the embeder let chromium decide what to do.
    if (resource_type != RESOURCE_TYPE_MAIN_FRAME) {
      // A sub-resource has a certificate error. The user doesn't really
      // have a context for making the right decision, so block the
      // request hard.
      callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
    } else {
#if defined(OS_TIZEN_TV_PRODUCT)
      // For TV, not allows page to be opened with certificate compromise,
      // needs to request certificate confirm again if user doesn't make a
      // decision.
      certificate_policy_decision_->SetDecision(false);
#else
      // By default chromium-efl allows page to be opened with certificate
      // compromise.
      certificate_policy_decision_->SetDecision(true);
      LOG(WARNING) << "Certificate for " << url.spec() << " was compromised";
#endif
    }
  }
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::VisibleSecurityStateChanged(WebContents* source) {
  std::unique_ptr<_Ewk_Certificate_Info> certificate_info;
  std::string pem_certificate;
  SSLStatus ssl_status = source->GetController().GetVisibleEntry()->GetSSL();
  scoped_refptr<net::X509Certificate> cert = ssl_status.certificate;

  if (!cert) {
    certificate_info.reset(new _Ewk_Certificate_Info(nullptr, false));
  } else {
    if (!net::X509Certificate::GetPEMEncoded(
        cert->os_cert_handle(), &pem_certificate)) {
      LOG(ERROR) << "Unable to get encoded PEM!";  // LCOV_EXCL_LINE
      return;
    }
    // Note: The implementation below is a copy of the one in
    // ChromeAutofillClient::IsContextSecure, and should be kept in sync
    // until crbug.com/505388 gets implemented.
    bool is_context_secure =
        ssl_status.certificate &&
         (!net::IsCertStatusError(ssl_status.cert_status) ||
          net::IsCertStatusMinorError(ssl_status.cert_status)) &&
         !(ssl_status.content_status & content::SSLStatus::RAN_INSECURE_CONTENT);

    certificate_info.reset(
        new _Ewk_Certificate_Info(pem_certificate.c_str(), is_context_secure));
  }

  web_view_->SmartCallback<EWebViewCallbacks::SSLCertificateChanged>().call(
      certificate_info.get());
}

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::ActivateContents(WebContents* contents) {
#if defined(OS_TIZEN)
  app_control_h app_control = nullptr;
  int ret = app_control_create(&app_control);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << "app_control_create is failed with err " << ret;
    return;
  }

  std::unique_ptr<std::remove_pointer<app_control_h>::type,
                  decltype(app_control_destroy)*>
      auto_release{app_control, app_control_destroy};

  char* app_id = nullptr;
  int app_ret = app_manager_get_app_id(getpid(), &app_id);
  if (app_ret != APP_MANAGER_ERROR_NONE) {
    LOG(ERROR) << "app_manager_get_app_id is failed with err " << app_ret;
    return;
  }

  ret = app_control_set_app_id(app_control, app_id);
  if (app_id)
    free(app_id);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << "app_control_set_app_id is failed with err " << ret;
    return;
  }

  ret = app_control_send_launch_request(app_control, nullptr, nullptr);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << "app_control_send_launch_request is failed with err " << ret;
    return;
  }
#endif
}

void WebContentsDelegateEfl::SetContentSecurityPolicy(
    const std::string& policy,
    Ewk_CSP_Header_Type header_type) {
  if (document_created_) {
    RenderViewHost* rvh = web_contents_.GetRenderViewHost();
    rvh->Send(new EwkViewMsg_SetCSP(rvh->GetRoutingID(), policy, header_type));
  } else {
    DCHECK(!pending_content_security_policy_.get());
    pending_content_security_policy_.reset(
        new ContentSecurityPolicy(policy, header_type));
  }
}
/* LCOV_EXCL_STOP */

#if defined(TIZEN_AUTOFILL_SUPPORT)
void WebContentsDelegateEfl::UpdateAutofillIfRequired() {
  AutofillClientEfl::FromWebContents(&web_contents_)
      ->UpdateAutofillIfRequired();
}
#endif

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::OnDidChangeFocusedNodeBounds(
    const gfx::RectF& focused_node_bounds) {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  AutofillClientEfl::FromWebContents(&web_contents_)
      ->DidChangeFocusedNodeBounds(focused_node_bounds);
#endif
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::FindReply(
    WebContents* web_contents,  // LCOV_EXCL_LINE
    int request_id,
    int number_of_matches,
    const gfx::Rect& selection_rect,
    int active_match_ordinal,
    bool final_update) {
  if (final_update &&
      request_id == web_view_->current_find_request_id()) {  // LCOV_EXCL_LINE
    /* LCOV_EXCL_START */
    unsigned int uint_number_of_matches =
        static_cast<unsigned int>(number_of_matches);
    web_view_->SmartCallback<EWebViewCallbacks::TextFound>().call(
        &uint_number_of_matches);
  }
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::DidRenderFrame() {
  // Call FrameRendered callback when loading and first layout is finished.
  if (!did_render_frame_ && did_first_visually_non_empty_paint_ &&
      (web_view_->GetProgressValue() > 0.1)) {
    did_first_visually_non_empty_paint_ = false;
    did_render_frame_ = true;
    TTRACE_WEB("WebContentsDelegateEfl::DidRenderFrame");
    LOG(INFO) << "WebContentsDelegateEfl::DidRenderFrame";

    // "frame,rendered" message is triggered as soon as rendering of a frame
    // is completed.
    web_view_->SmartCallback<EWebViewCallbacks::FrameRendered>().call(0);
  }
}

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::DidRenderRotatedFrame(
    int new_rotation,
    int frame_data_output_rect_width) {
  if (!IsMobileProfile())
    return;

  bool is_manual_rotation_not_initialized =
      (rotation_ == -1) && (frame_data_output_rect_width_ == 0);
  bool is_changed_landscape_to_landscape =
      (rotation_ == 90 && new_rotation == 270) ||
      (rotation_ == 270 && new_rotation == 90);
  bool is_frame_data_width_changed =
      frame_data_output_rect_width_ != frame_data_output_rect_width;
  // While javascript dialog(alert, etc.) is showing, renderer is blocked.
  // As a result, rotated frame is not rendered although device is rotated
  // when javascript dialog is showing.
  // Need to invoke 'rotate,prepared' callback in that case.
  bool is_showing_javascript_dialog = dialog_manager_ &&
                                      dialog_manager_->IsShowing() &&
                                      rotation_ != new_rotation;
  if (is_manual_rotation_not_initialized || is_changed_landscape_to_landscape ||
      is_frame_data_width_changed || is_showing_javascript_dialog) {
    rotation_ = new_rotation;
    frame_data_output_rect_width_ = frame_data_output_rect_width;
    web_view_->SmartCallback<EWebViewCallbacks::RotatePrepared>().call(0);
  }
}

void WebContentsDelegateEfl::QueryNumberFieldAttributes() {
  RenderViewHost* rvh = web_contents_.GetRenderViewHost();
  if (rvh)
    rvh->Send(new EwkViewMsg_QueryNumberFieldAttributes(rvh->GetRoutingID()));
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::WillImePanelShow() {  // LCOV_EXCL_LINE
#if defined(OS_TIZEN_MOBILE_PRODUCT)
  web_view_->HidePopupMenu();
#endif
}  // LCOV_EXCL_LINE

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::SetTooltipText(const base::string16& text) {
  std::string tooltip = base::UTF16ToUTF8(text);
  if (tooltip.empty()) {
    web_view_->SmartCallback<EWebViewCallbacks::TooltipTextUnset>().call();
  } else {
    web_view_->SmartCallback<EWebViewCallbacks::TooltipTextSet>().call(
        tooltip.c_str());
  }
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::DidTopControlsMove(int offset) {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  AutofillClientEfl::FromWebContents(&web_contents_)->HideAutofillPopup();
#endif

  web_view_->SmartCallback<EWebViewCallbacks::TopControlMoved>().call(&offset);
}

JavaScriptDialogManager* WebContentsDelegateEfl::GetJavaScriptDialogManager(
    WebContents* source) {
  if (!dialog_manager_)
    dialog_manager_ = new JavaScriptDialogManagerEfl();
  return dialog_manager_;
}

void WebContentsDelegateEfl::OnUpdateSettings(const Ewk_Settings* settings) {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  PasswordManagerClientEfl* client =
      PasswordManagerClientEfl::FromWebContents(&web_contents_);
  if (client) {
    PrefService* prefs = client->GetPrefs();
    prefs->SetBoolean(password_manager::prefs::kCredentialsEnableService,
                      settings->autofillPasswordForm());
  }
#endif
}

bool WebContentsDelegateEfl::OnMessageReceived(
    const IPC::Message& message, RenderFrameHost* render_frame_host) {
  bool handled = true;
  /* LCOV_EXCL_START */
  IPC_BEGIN_MESSAGE_MAP(WebContentsDelegateEfl, message)
    IPC_MESSAGE_HANDLER(EwkHostMsg_RequestSelectCollectionInformationUpdateACK,
                        OnRequestSelectCollectionInformationUpdateACK)
    IPC_MESSAGE_HANDLER(EwkHostMsg_DidChangeMaxScrollOffset,
                        OnDidChangeMaxScrollOffset)
    IPC_MESSAGE_HANDLER(EwkHostMsg_DidChangeScrollOffset,
                        OnDidChangeScrollOffset)
    IPC_MESSAGE_HANDLER(EwkHostMsg_FormSubmit, OnFormSubmit)
#if defined(OS_TIZEN_TV_PRODUCT)
    IPC_MESSAGE_HANDLER(EwkHostMsg_LoginFormSubmitted, OnLoginFormSubmitted)
    IPC_MESSAGE_HANDLER(EwkHostMsg_LoginFieldsIdentified, OnLoginFields)
#endif
    IPC_MESSAGE_HANDLER(EwkHostMsg_DidNotAllowScript, OnDidNotAllowScript)
    /* LCOV_EXCL_STOP */
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

bool WebContentsDelegateEfl::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  /* LCOV_EXCL_START */
  IPC_BEGIN_MESSAGE_MAP(WebContentsDelegateEfl, message)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(EwkHostMsg_GetContentSecurityPolicy,
                                    OnGetContentSecurityPolicy)
    IPC_MESSAGE_HANDLER(EwkHostMsg_DidPrintPagesToPdf, OnPrintedMetafileReceived)
    IPC_MESSAGE_HANDLER(EwkHostMsg_WrtMessage, OnWrtPluginMessage)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(EwkHostMsg_WrtSyncMessage,
                                    OnWrtPluginSyncMessage)
    IPC_MESSAGE_HANDLER(EwkHostMsg_HandleTapGestureWithContext,
                        OnHandleTapGestureWithContext)
    IPC_MESSAGE_HANDLER(EwkHostMsg_DidChangeNumberFieldAttributes,
                        OnDidChangeNumberFieldAttributes)
    IPC_MESSAGE_HANDLER(EwkHostMsg_DidChangeFocusedNodeBounds,
                        OnDidChangeFocusedNodeBounds)
    IPC_MESSAGE_HANDLER(EwkHostMsg_DidChangeInputType, OnDidChangeInputType)
#if defined(TIZEN_ATK_FEATURE_VD)
    IPC_MESSAGE_HANDLER(EwkHostMsg_MoveFocusToBrowser, OnMoveFocusToBrowser)
#endif
    IPC_MESSAGE_HANDLER(EwkHostMsg_IsVideoPlaying, OnIsVideoPlayingGet);
    IPC_MESSAGE_HANDLER(ViewHostMsg_OnRecognizeArticleResult,
                        OnRecognizeArticleResult)
    /* LCOV_EXCL_STOP */
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void WebContentsDelegateEfl::DidFailProvisionalLoad(  // LCOV_EXCL_LINE
    RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    int error_code,
    const base::string16& error_description) {
  DidFailLoad(render_frame_host, validated_url, error_code,
              error_description);  // LCOV_EXCL_LINE
}  // LCOV_EXCL_LINE

void WebContentsDelegateEfl::DidFailLoad(  // LCOV_EXCL_LINE
    RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    int error_code,
    const base::string16& error_description) {
  if (!IsMainFrame(render_frame_host))  // LCOV_EXCL_LINE
    return;

  /* LCOV_EXCL_START */
  web_view_->InvokeLoadError(validated_url, error_code,
                             error_code == net::ERR_ABORTED);
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::OnWrtPluginMessage(
    const Ewk_Wrt_Message_Data& data) {
  std::unique_ptr<Ewk_Wrt_Message_Data> p(new Ewk_Wrt_Message_Data);
  p->type = data.type;
  p->value = data.value;
  p->id = data.id;
  p->reference_id = data.reference_id;

  web_view_->SmartCallback<EWebViewCallbacks::WrtPluginsMessage>().call(
      p.get());
}

void WebContentsDelegateEfl::OnWrtPluginSyncMessage(
    const Ewk_Wrt_Message_Data& data,
    IPC::Message* reply) {
  Ewk_Wrt_Message_Data tmp = data;
  web_view_->SmartCallback<EWebViewCallbacks::WrtPluginsMessage>().call(&tmp);
  EwkHostMsg_WrtSyncMessage::WriteReplyParams(reply, tmp.value);
  Send(reply);
}

void WebContentsDelegateEfl::OnHandleTapGestureWithContext(
    bool is_content_editable) {
  web_view_->HandleTapGestureForSelection(is_content_editable);
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::DidFirstVisuallyNonEmptyPaint() {
  did_first_visually_non_empty_paint_ = true;
  web_view_->SmartCallback<EWebViewCallbacks::LoadNonEmptyLayoutFinished>()
      .call();
}

void WebContentsDelegateEfl::DidStartLoading() {
  did_render_frame_ = false;
#if defined(TIZEN_VD_DISABLE_GPU)
  web_view_->SmartCallback<EWebViewCallbacks::FrameRendered>().call(0);
#endif
}

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::OnRequestSelectCollectionInformationUpdateACK(
    int form_element_count,
    int current_node_index,
    bool prev_state,
    bool next_state) {
  if (web_view_->GetSelectPicker())
    web_view_->GetSelectPicker()->UpdateFormNavigation(form_element_count,
                                                       current_node_index);
}

void WebContentsDelegateEfl::OnDidChangeMaxScrollOffset(int max_scroll_x,
                                                        int max_scroll_y) {
  web_view_->GetScrollDetector()->SetMaxScroll(max_scroll_x, max_scroll_y);
}

void WebContentsDelegateEfl::OnDidChangeScrollOffset(int scroll_x,
                                                     int scroll_y) {
  web_view_->GetScrollDetector()->OnChangeScrollOffset(
      gfx::Vector2d(scroll_x, scroll_y));
}

void WebContentsDelegateEfl::OnFormSubmit(const GURL& url) {
  web_view_->SmartCallback<EWebViewCallbacks::FormSubmit>().call(
      url.GetContent().c_str());
}

void WebContentsDelegateEfl::OnDidNotAllowScript() {
  web_view_->SmartCallback<EWebViewCallbacks::DidNotAllowScript>().call();
}

void WebContentsDelegateEfl::OnGetContentSecurityPolicy(
    IPC::Message* reply_msg) {
  document_created_ = true;
  if (!pending_content_security_policy_.get()) {
    EwkHostMsg_GetContentSecurityPolicy::WriteReplyParams(
        reply_msg, std::string(), EWK_DEFAULT_POLICY);
  } else {
    EwkHostMsg_GetContentSecurityPolicy::WriteReplyParams(
        reply_msg, pending_content_security_policy_->policy,
        pending_content_security_policy_->header_type);
    pending_content_security_policy_.reset();
  }
  Send(reply_msg);
}

void WebContentsDelegateEfl::OnPrintedMetafileReceived(
    const DidPrintPagesParams& params) {
  base::SharedMemory shared_buf(params.metafile_data_handle, true);
  if (!shared_buf.Map(params.data_size)) {
    NOTREACHED() << "couldn't map";
    return;
  }

  std::unique_ptr<printing::PdfMetafileSkia> metafile(
      new printing::PdfMetafileSkia(printing::SkiaDocumentType::PDF));
  if (!metafile->InitFromData(shared_buf.memory(), params.data_size)) {
    NOTREACHED() << "Invalid metafile header";
    return;
  }
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&WritePdfDataToFile, metafile.release(), params.filename));
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::NavigationEntryCommitted(
    const LoadCommittedDetails& load_details) {
  web_view_->InvokeBackForwardListChangedCallback();
}

void WebContentsDelegateEfl::RenderViewCreated(
    RenderViewHost* render_view_host) {
  web_view_->RenderViewCreated(render_view_host);
}

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::RenderProcessGone(base::TerminationStatus status) {
  // See RenderWidgetHostViewEfl::RenderProcessGone.
  if (status == base::TERMINATION_STATUS_ABNORMAL_TERMINATION ||
      status == base::TERMINATION_STATUS_PROCESS_WAS_KILLED ||
      status == base::TERMINATION_STATUS_PROCESS_CRASHED) {
    web_view_->HandleRendererProcessCrash();
  }
}
bool WebContentsDelegateEfl::DidAddMessageToConsole(WebContents* source,
                                                    int32_t level,
                                                    const string16& message,
                                                    int32_t line_no,
                                                    const string16& source_id) {
  std::unique_ptr<_Ewk_Console_Message> console_message(
      new _Ewk_Console_Message(level, base::UTF16ToUTF8(message).c_str(),
                               line_no, base::UTF16ToUTF8(source_id).c_str()));
  web_view_->SmartCallback<EWebViewCallbacks::ConsoleMessage>().call(
      console_message.get());
  return true;
}

void WebContentsDelegateEfl::RunFileChooser(RenderFrameHost* render_frame_host,
                                            const FileChooserParams& params) {
  web_view_->ShowFileChooser(render_frame_host, params);
}

content::ColorChooser* WebContentsDelegateEfl::OpenColorChooser(
    WebContents* web_contents,
    SkColor color,
    const std::vector<ColorSuggestion>& suggestions) {
  ColorChooserEfl* color_chooser_efl = new ColorChooserEfl(*web_contents);
  web_view_->RequestColorPicker(SkColorGetR(color), SkColorGetG(color),
                                SkColorGetB(color), SkColorGetA(color));

  return color_chooser_efl;
}

void WebContentsDelegateEfl::OpenDateTimeDialog(
    ui::TextInputType dialog_type,
    double dialog_value,
    double min,
    double max,
    double step,
    const std::vector<DateTimeSuggestion>& suggestions) {
  web_view_->InputPickerShow(dialog_type, dialog_value);
}

bool WebContentsDelegateEfl::PreHandleGestureEvent(
    WebContents* source,
    const blink::WebGestureEvent& event) {
  blink::WebInputEvent::Type event_type = event.GetType();
  switch (event_type) {
    case blink::WebInputEvent::kGestureDoubleTap:
      if (is_fullscreen_)
        return true;
      break;
    case blink::WebInputEvent::kGesturePinchBegin:
    case blink::WebInputEvent::kGesturePinchUpdate:
    case blink::WebInputEvent::kGesturePinchEnd:
      if (!IsPinchToZoomEnabled() ||
          IsFullscreenForTabOrPending(&web_contents()))
        return true;
      break;
    default:
      break;
  }
  return false;
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::TitleWasSet(NavigationEntry* entry) {
  if (entry)
    web_view_->GetBackForwardList()->UpdateItemWithEntry(entry);
}

void WebContentsDelegateEfl::DidChangeThemeColor(SkColor color) {
  web_view_->DidChangeThemeColor(color);
}

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::SetReaderMode(Eina_Bool enable) {
  reader_mode_ = enable;
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::OnRecognizeArticleResult(  // LCOV_EXCL_LINE
    bool is_article,
    const base::string16& page_url) {
  /* LCOV_EXCL_START */
  if (!page_url.empty())
    web_view_->SmartCallback<EWebViewCallbacks::ReaderMode>().call(&is_article);
}
/* LCOV_EXCL_STOP */

void WebContentsDelegateEfl::RequestManifestInfo(
    Ewk_View_Request_Manifest_Callback callback,
    void* user_data) {
  web_contents_.GetManifest(
      base::Bind(&WebContentsDelegateEfl::OnDidGetManifest,
                 base::Unretained(this), callback, user_data));
}

void WebContentsDelegateEfl::OnDidGetManifest(
    Ewk_View_Request_Manifest_Callback callback,
    void* user_data,
    const GURL& manifest_url,
    const Manifest& manifest) {
  if (manifest.IsEmpty()) {
    web_view_->DidRespondRequestManifest(nullptr, callback,
                                         user_data);  // LCOV_EXCL_LINE
  } else {
    _Ewk_View_Request_Manifest ewk_manifest(manifest);
    web_view_->DidRespondRequestManifest(&ewk_manifest, callback, user_data);
  }
}

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::OnIsVideoPlayingGet(bool is_playing,
                                                 int callback_id) {
  if (web_view_)
    web_view_->InvokeIsVideoPlayingCallback(is_playing, callback_id);
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
bool WebContentsDelegateEfl::GetMarlinEnable() {
  return web_view_->GetMarlinEnable();
}

const std::string& WebContentsDelegateEfl::GetActiveDRM() {
  return web_view_->GetActiveDRM();
}

void WebContentsDelegateEfl::UpdateCurrentTime(double current_time) {
  web_view_->UpdateCurrentTime(current_time);
}

void WebContentsDelegateEfl::NotifySubtitleState(int state, double time_stamp) {
  web_view_->NotifySubtitleState(state, time_stamp);
}

void WebContentsDelegateEfl::NotifySubtitlePlay(int active_track_id,
                                                const std::string& url,
                                                const std::string& lang) {
  web_view_->NotifySubtitlePlay(active_track_id, url.c_str(), lang.c_str());
}

void WebContentsDelegateEfl::NotifySubtitleData(int track_id,
                                                double time_stamp,
                                                const std::string& data,
                                                unsigned int size) {
  web_view_->NotifySubtitleData(track_id, time_stamp, data, size);
}

void WebContentsDelegateEfl::NotifyPlaybackState(int state,
                                                 const std::string& url,
                                                 const std::string& mime_type,
                                                 bool* media_resource_acquired,
                                                 std::string* translated_url,
                                                 std::string* drm_info) {
  if (!web_view_)
    return;
  std::vector<std::string> data = web_view_->NotifyPlaybackState(
      state, url.empty() ? "" : url.c_str(),
      mime_type.empty() ? "" : mime_type.c_str());
  if (data.size()) {
    if (media_resource_acquired)
      *media_resource_acquired = !data.at(0).empty() ? true : false;
    if (translated_url)
      *translated_url = data.at(1);
    if (drm_info)
      *drm_info = data.at(2);
  }
}

void WebContentsDelegateEfl::NotifyParentalRatingInfo(const std::string& info,
                                                      const std::string& url) {
  web_view_->NotifyParentalRatingInfo(info.c_str(), url.c_str());
}

bool WebContentsDelegateEfl::IsHighBitRate() {
  if (web_view_)
    return web_view_->IsHighBitRate();
  return false;
}

void WebContentsDelegateEfl::OnLoginFields(const int type,
                                           const std::string& id,
                                           const std::string& username_element,
                                           const std::string& password_element,
                                           const std::string& action_url) {
  LOG(INFO) << "[SPASS LoginFields], type:" << type << ", id:" << id;
  form_info_.reset(
      new _Ewk_Form_Info(static_cast<Ewk_Form_Type>(type), id, std::string()));
  form_info_.get()->username_element_ = username_element;
  form_info_.get()->password_element_ = password_element;
  form_info_.get()->action_url_ = action_url;
  web_view_->SmartCallback<EWebViewCallbacks::LoginFields>().call(
      form_info_.get());
}

void WebContentsDelegateEfl::OnLoginFormSubmitted(
    const std::string& id,
    const std::string& pw,
    const std::string& username_element,
    const std::string& password_element,
    const std::string& action_url) {
  if (is_auto_login_) {
    is_auto_login_ = false;
    LOG(INFO) << "[SPASS] auto login, donot raise submit notification";
    return;
  }
  form_info_.reset(new _Ewk_Form_Info(EWK_FORM_BOTH, id, pw));
  form_info_.get()->username_element_ = username_element;
  form_info_.get()->password_element_ = password_element;
  form_info_.get()->action_url_ = action_url;
  web_view_->SmartCallback<EWebViewCallbacks::LoginFormSubmitted>().call(
      form_info_.get());
}

void WebContentsDelegateEfl::AutoLogin(const std::string& user_name,
                                       const std::string& password) {
  is_auto_login_ = true;
  AutofillClientEfl::FromWebContents(&web_contents_)
      ->AutoLogin(user_name, password);
}
#endif

/* LCOV_EXCL_START */
void WebContentsDelegateEfl::OnDidChangeInputType(bool is_password_input) {
  web_view_->UpdateContextMenu(is_password_input);
}

void WebContentsDelegateEfl::OnDidChangeNumberFieldAttributes(
    bool is_minimum_negative,
    bool is_step_integer) {
  auto rwhv = static_cast<RenderWidgetHostViewEfl*>(
      web_contents_.GetRenderWidgetHostView());

  rwhv->UpdateIMELayoutVariation(is_minimum_negative, is_step_integer);
}

void WebContentsDelegateEfl::ShowRepostFormWarningDialog(WebContents* source) {
  _Ewk_Form_Repost_Decision_Request::Create(source);
}

#if defined(TIZEN_ATK_FEATURE_VD)
void WebContentsDelegateEfl::OnMoveFocusToBrowser(int direction) {
  if (web_view_)
    web_view_->MoveFocusToBrowser(direction);
}
#endif

#if defined(TIZEN_TBM_SUPPORT)
bool WebContentsDelegateEfl::OffscreenRenderingEnabled() {
  return offscreen_rendering_enabled_;
}

void WebContentsDelegateEfl::SetOffscreenRendering(bool enable) {
  offscreen_rendering_enabled_ = enable;
}

void WebContentsDelegateEfl::DidRenderOffscreenFrame(void* buffer) {
  if (offscreen_rendering_enabled_) {
    web_view_->SmartCallback<EWebViewCallbacks::OffscreenFrameRendered>().call(
        buffer);
  }
}
#endif
/* LCOV_EXCL_STOP */

}  // namespace content
