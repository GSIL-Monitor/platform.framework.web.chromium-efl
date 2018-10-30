// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <libintl.h>
#include <memory>

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "common/content_switches_efl.h"
#include "common/navigation_policy_params.h"
#include "common/render_messages_ewk.h"
#include "components/network_hints/renderer/prescient_networking_dispatcher.h"
#include "components/visitedlink/renderer/visitedlink_slave.h"
#include "content/child/request_extra_data.h"
#include "content/common/locale_efl.h"
#include "content/common/paths_efl.h"
#include "content/common/wrt/wrt_url_parse.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "content/public/common/url_loader_throttle.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "content/renderer/render_view_impl.h"
#include "net/base/net_errors.h"
#include "renderer/content_renderer_client_efl.h"
#include "renderer/content_settings_client_efl.h"
#include "renderer/editorclient_agent.h"
#include "renderer/gin_native_bridge_dispatcher.h"
#include "renderer/plugins/plugin_placeholder_efl.h"
#include "renderer/render_frame_observer_efl.h"
#include "renderer/render_view_observer_efl.h"
#include "third_party/WebKit/public/platform/URLConversion.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/platform/WebViewportStyle.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebDocumentLoader.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebSettings.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "url/gurl.h"
#include "wrt/wrtwidget.h"

#if defined(OS_TIZEN)
#include <vconf/vconf.h>
#endif

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
#include "content/common/tts_messages_efl.h"
#include "content/renderer/tts_dispatcher_efl.h"
#endif

#if BUILDFLAG(ENABLE_NACL)
#include "components/nacl/common/nacl_constants.h"
#include "components/nacl/renderer/nacl_helper.h"
#include "renderer/pepper/pepper_helper.h"
#endif

#if defined(TIZEN_AUTOFILL_SUPPORT)
#include "components/autofill/content/renderer/autofill_agent.h"
#include "components/autofill/content/renderer/password_autofill_agent.h"
#include "components/autofill/content/renderer/password_generation_agent.h"
#include "components/autofill/core/common/password_generation_util.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include "base/strings/utf_string_conversions.h"
#include "common/application_type.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "content/public/common/url_utils.h"
#include "platform/runtime_enabled_features.h"
#include "renderer/key_systems_tizen.h"
#include "renderer/plugins/plugin_placeholder_avplayer.h"
#include "url/url_util.h"
#include "wrt/hbbtv_widget.h"
using namespace autofill::form_util;
#endif

#if defined(USE_PLUGIN_PLACEHOLDER_HOLE)
#include "renderer/plugins/plugin_placeholder_hole.h"
#endif

#if defined(TIZEN_AUTOFILL_SUPPORT)
using autofill::AutofillAgent;
using autofill::PasswordAutofillAgent;
using autofill::PasswordGenerationAgent;
#endif

#if defined(TIZEN_PEPPER_EXTENSIONS)
#include "common/trusted_pepper_plugin_info_cache.h"
#include "common/trusted_pepper_plugin_util.h"
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
#include "renderer/pepper/pepper_helper.h"
#endif

// Chromium-efl deliberately uses a different value than
// default_maximum_page_scale_factor (4.0) because it has shown
// better empirical results.
static const float maximum_legible_scale = 2.0f;

// Scale limits which are defined for mobile builds in
// web_preferences.cc.
static const float minimum_page_scale_for_mobile = 0.25f;
static const float maximum_page_scale_for_mobile = 5.f;

#if defined(TIZEN_PEPPER_EXTENSIONS)
namespace {

bool CreateTrustedPepperPlugin(content::RenderFrame* render_frame,
                               const blink::WebPluginParams& params,
                               blink::WebPlugin** plugin) {
  if (pepper::AreTrustedPepperPluginsDisabled())
    return false;

  auto cache = pepper::TrustedPepperPluginInfoCache::GetInstance();
  content::PepperPluginInfo info;
  if (cache->FindPlugin(params.mime_type.Utf8(), params.url, &info)) {
    *plugin = render_frame->CreatePlugin(info.ToWebPluginInfo(), params,
                                         nullptr);

    if (*plugin)
      return true;
  }

  return false;
}

}  // namespace
#endif  // defined(TIZEN_PEPPER_EXTENSIONS)

#if defined(OS_TIZEN_TV_PRODUCT)
namespace {

bool SendEnterKeyBoardEvent(blink::WebFrame* web_frame) {
  if (!web_frame)
    return false;

  blink::WebView* web_view = web_frame->View();
  if (!web_view)
    return false;

  LOG(INFO) << "[SPASS] " << __FUNCTION__;
  blink::WebKeyboardEvent keyboard_event;
  keyboard_event.windows_key_code = ui::VKEY_RETURN;
  keyboard_event.SetModifiers(blink::WebInputEvent::kIsKeyPad);
  keyboard_event.text[0] = ui::VKEY_RETURN;
  keyboard_event.SetType(blink::WebInputEvent::kKeyDown);
  web_view->HandleInputEvent(blink::WebCoalescedInputEvent(keyboard_event));

  keyboard_event.SetType(blink::WebInputEvent::kKeyUp);
  web_view->HandleInputEvent(blink::WebCoalescedInputEvent(keyboard_event));
  return true;
}
}  // namespace
#endif
/* LCOV_EXCL_START */
ContentRendererClientEfl::ContentRendererClientEfl()
    : javascript_can_open_windows_(true)
    , shutting_down_(false)
#if defined(OS_TIZEN_TV_PRODUCT)
    , floating_video_window_on_(false)
#endif
{
}

ContentRendererClientEfl::~ContentRendererClientEfl() {}

void ContentRendererClientEfl::RenderThreadStarted() {
  content::RenderThread* const thread = content::RenderThread::Get();

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

#if defined(OS_TIZEN_TV_PRODUCT)
  if (command_line.HasSwitch(switches::kApplicationType)) {
    std::string type =
        command_line.GetSwitchValueASCII(switches::kApplicationType);
    int app_type = 0;
    base::StringToInt(type, &app_type);
    LOG(INFO) << "Sync application type for render thread: " << app_type;
    content::SetApplicationType(
        static_cast<content::ApplicationType>(app_type));
#if defined(TIZEN_PEPPER_EXTENSIONS)
    pepper::UpdatePluginRegistry();
#endif
  }
#endif

  if (command_line.HasSwitch(switches::kInjectedBundlePath)) {
    V8Widget::Type type = V8Widget::Type::WRT;
#if defined(OS_TIZEN_TV_PRODUCT)
    if (content::IsHbbTV())
      type = V8Widget::Type::HBBTV;
#endif
    widget_.reset(V8Widget::CreateWidget(type, command_line));
    if (widget_->GetObserver())
      thread->AddObserver(widget_->GetObserver());

#if defined(OS_TIZEN_TV_PRODUCT)
    if (command_line.HasSwitch(switches::kMaxUrlCharacters)) {
      std::string switch_value =
          base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
              switches::kMaxUrlCharacters);
      size_t max_url_chars;
      if (base::StringToSizeT(switch_value, &max_url_chars))
        url::SetMaxURLCharSize(max_url_chars);
    }
#endif
  }

  render_thread_observer_.reset(new RenderThreadObserverEfl(this));
  thread->AddObserver(render_thread_observer_.get());

  visited_link_slave_.reset(new visitedlink::VisitedLinkSlave());
  auto registry = base::MakeUnique<service_manager::BinderRegistry>();
  registry->AddInterface(visited_link_slave_->GetBindCallback(),
                         base::ThreadTaskRunnerHandle::Get());
  if (content::ChildThread::Get()) {
    content::ChildThread::Get()
        ->GetServiceManagerConnection()
        ->AddConnectionFilter(base::MakeUnique<content::SimpleConnectionFilter>(
            std::move(registry)));
  }

  prescient_networking_dispatcher_.reset(
      new network_hints::PrescientNetworkingDispatcher());

  if (!command_line.HasSwitch(switches::kSingleProcess))
    LocaleEfl::Initialize();
}

void ContentRendererClientEfl::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  // Deletes itself when render_frame is destroyed.
  new content::RenderFrameObserverEfl(render_frame);
  new content::ContentSettingsClientEfl(render_frame);
  new content::GinNativeBridgeDispatcher(render_frame);

#if defined(TIZEN_AUTOFILL_SUPPORT)
  service_manager::BinderRegistry* registry = render_frame->Registry();

  PasswordAutofillAgent* password_autofill_agent =
      new PasswordAutofillAgent(render_frame, registry);
  PasswordGenerationAgent* password_generation_agent =
      new PasswordGenerationAgent(render_frame, password_autofill_agent,
                                  registry);
  new AutofillAgent(render_frame, password_autofill_agent,
                    password_generation_agent, registry);
#endif

#if (defined(TIZEN_PEPPER_EXTENSIONS) && BUILDFLAG(ENABLE_PLUGINS))
  new PepperHelper(render_frame);
#endif
#if BUILDFLAG(ENABLE_NACL)
  new nacl::NaClHelper(render_frame);
#endif
}

void ContentRendererClientEfl::RenderViewCreated(
    content::RenderView* render_view) {
  ApplyCustomMobileSettings(render_view->GetWebView());

  // Deletes itself when render_view is destroyed.
  new RenderViewObserverEfl(render_view, this);
  new editing::EditorClientAgent(render_view);
}
/* LCOV_EXCL_STOP */

bool ContentRendererClientEfl::OverrideCreatePlugin(  // LCOV_EXCL_LINE
    content::RenderFrame* render_frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {
#if defined(TIZEN_PEPPER_EXTENSIONS)
  if (CreateTrustedPepperPlugin(render_frame, params, plugin))
    return true;
#endif  // TIZEN_PEPPER_EXTENSIONS
#if BUILDFLAG(ENABLE_NACL)
  std::string orig_mime_type = params.mime_type.Utf8();
  if (orig_mime_type == nacl::kNaClPluginMimeType)
    return false;  // LCOV_EXCL_LINE
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
  plugins::PluginPlaceholderBase* placeholder = nullptr;

#if defined(OS_TIZEN_TV_PRODUCT)
#if defined(USE_PLUGIN_PLACEHOLDER_HOLE)
  if (widget_ && widget_->GetType() == V8Widget::Type::HBBTV &&
      PluginPlaceholderHole::SupportsMimeType(render_frame->GetWebFrame(),
                                              params.mime_type.Utf8())) {
    placeholder = PluginPlaceholderHole::Create(
        render_frame, render_frame->GetWebFrame(), params);
  }
#endif

  if (widget_ && widget_->GetType() == V8Widget::Type::WRT &&
      PluginPlaceholderAvplayer::SupportsMimeType(params.mime_type.Utf8())) {
    placeholder = PluginPlaceholderAvplayer::Create(
        render_frame, render_frame->GetWebFrame(), params);
  }
#endif

  if (!placeholder) {
    placeholder = PluginPlaceholderEfl::CreateMissingPlugin(
        render_frame, render_frame->GetWebFrame(), params);
  }

  if (!placeholder)
    return false;
  *plugin = placeholder->plugin();
  return true;
#else
  return false;  // LCOV_EXCL_LINE
#endif
}

#if defined(OS_TIZEN_TV_PRODUCT)
void ContentRendererClientEfl::ScrollbarThumbPartFocusChanged(
    int view_id,
    blink::WebScrollbar::Orientation orientation,
    bool focused) {
  content::RenderThread::Get()->Send(
      new EwkViewHostMsg_ScrollbarThumbPartFocusChanged(view_id, orientation,
                                                        focused));
}
#endif

bool ContentRendererClientEfl::WillSendRequest(  // LCOV_EXCL_LINE
    blink::WebLocalFrame* frame,
    ui::PageTransition transition_type,
    const blink::WebURL& url,
    std::vector<std::unique_ptr<content::URLLoaderThrottle>>* throttles,
    GURL* new_url
#if defined(OS_TIZEN_TV_PRODUCT)
    ,
    bool& is_encrypted_file) {
  if (widget_)
    return widget_->ParseUrl(url, *new_url, &is_encrypted_file);
#else
    ) {
#endif
  return false;  // LCOV_EXCL_LINE
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool ContentRendererClientEfl::GetFileDecryptedDataBuffer(
    const GURL& url,
    std::vector<char>* data) {
  if (widget_)
    return widget_->GetFileDecryptedDataBuffer(url, data);

  return false;
}
#endif

bool ContentRendererClientEfl::GetWrtParsedUrl(const GURL& url,
                                               GURL& parsed_url) {
  bool is_encrypted_file = false;
  if (widget_)
    return widget_->ParseUrl(url, parsed_url, &is_encrypted_file);

  return false;
}

/* LCOV_EXCL_START */
void ContentRendererClientEfl::DidCreateScriptContext(
    blink::WebFrame* frame,
    v8::Handle<v8::Context> context,
    int world_id) {
  const content::RenderView* render_view =
      content::RenderView::FromWebView(frame->View());
  if (!widget_)
    return;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (widget_->GetType() == V8Widget::Type::HBBTV) {
    blink::RuntimeEnabledFeatures::SetCSS3NavigationEnabled(true);
    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();
    if (command_line.HasSwitch(switches::kJSPluginMimeTypes)) {
      std::string mime_types =
          command_line.GetSwitchValueASCII(switches::kJSPluginMimeTypes);
      static_cast<HbbtvWidget*>(widget_.get())
          ->RegisterJSPluginMimeTypes(mime_types);
    }
  }
#endif

  widget_->StartSession(context, render_view->GetRoutingID(),
                        frame->ToWebLocalFrame()
                            ->GetDocument()
                            .BaseURL()
                            .GetString()
                            .Utf8()
                            .c_str());
}

void ContentRendererClientEfl::WillReleaseScriptContext(
    blink::WebFrame* frame,
    v8::Handle<v8::Context> context,
    int world_id) {
  if (widget_)
    widget_->StopSession(context);
}
/* LCOV_EXCL_STOP */

bool ContentRendererClientEfl::HandleNavigation(  // LCOV_EXCL_LINE
    content::RenderFrame* render_frame,
    content::DocumentState* document_state,
    bool render_view_was_created_by_renderer,
    blink::WebFrame* frame,
    const blink::WebURLRequest& request,
    blink::WebNavigationType type,
    blink::WebNavigationPolicy default_policy,
    bool is_redirect) {
  // During renderer is shutting down, sync message should not be sent.
  if (shutting_down_)  // LCOV_EXCL_LINE
    return false;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (content::IsTIZENWRT() && GURL(request.Url()).SchemeIsFileSystem())
    return false;
#endif

  /* LCOV_EXCL_START */
  const content::RenderView* render_view =
      content::RenderView::FromWebView(frame->View());
  bool result = false;
  GURL referrer_url(blink::WebStringToGURL(
      request.HttpHeaderField(blink::WebString::FromUTF8("Referer"))));
  blink::WebReferrerPolicy referrer_policy =
      request.IsNull()
          ? frame->ToWebLocalFrame()->GetDocument().GetReferrerPolicy()
          : request.GetReferrerPolicy();
  int render_view_id = render_view->GetRoutingID();

  NavigationPolicyParams params;
  params.render_view_id = render_view_id;
  params.url = request.Url();
  params.httpMethod = request.HttpMethod().Utf8();
  params.referrer = content::Referrer(referrer_url, referrer_policy);
  params.auth =
      request.HttpHeaderField(blink::WebString::FromUTF8("Authorization"));
  params.policy = default_policy;
  params.is_main_frame = (frame->View()->MainFrame() == frame);
  params.type = type;
  params.is_redirect = is_redirect;
  params.cookie =
      request.HttpHeaderField(blink::WebString::FromUTF8("Cookie")).Utf8();

  blink::WebDocumentLoader* docLoader =
      frame->ToWebLocalFrame()->GetProvisionalDocumentLoader();
  params.should_replace_current_entry =
      (docLoader ? docLoader->ReplacesCurrentHistoryItem() : false);

  // Sync message, renderer is blocked here.
  content::RenderThread::Get()->Send(
      new EwkHostMsg_DecideNavigationPolicy(params, &result));

  return result;
}

unsigned long long ContentRendererClientEfl::VisitedLinkHash(
    const char* canonical_url,
    size_t length) {
  return visited_link_slave_->ComputeURLFingerprint(canonical_url, length);
}

bool ContentRendererClientEfl::IsLinkVisited(unsigned long long link_hash) {
  return visited_link_slave_->IsVisited(link_hash);
}

blink::WebPrescientNetworking*
ContentRendererClientEfl::GetPrescientNetworking() {
  return prescient_networking_dispatcher_.get();
}

const char* GetErrorTag(int error) {
  // The list of error reason mappings can be found in
  // net/base/net_error_list.h
  if ((error <= net::ERR_DNS_MALFORMED_RESPONSE &&
       error >= net::ERR_DNS_SORT_ERROR) ||
      error == net::ERR_NAME_RESOLUTION_FAILED ||
      error == net::ERR_NAME_NOT_RESOLVED) {
    return "IDS_WEBVIEW_BODY_THE_SERVER_AT_PS_CANT_BE_FOUND_BECAUSE_THE_DNS_"
           "LOOK_UP_FAILED_MSG";
  }

  if (error <= net::ERR_INTERNET_DISCONNECTED &&
      error >= net::ERR_SSL_HANDSHAKE_NOT_COMPLETED) {
    return "IDS_WEBVIEW_BODY_UNABLE_TO_LOAD_THE_PAGE_PS_TOOK_TOO_LONG_TO_"
           "RESPOND_THE_WEBSITE_MAY_BE_DOWN_OR_THERE_MAY_HAVE_BEEN_A_NETWORK_"
           "CONNECTION_ERROR";
  }

  if (error <= net::ERR_ICANN_NAME_COLLISION && error >= net::ERR_CERT_END) {
    // net::ERR_SSL_INAPPROPRIATE_FALLBACK was removed by [1] in upstream.
    // So, we change from to net::ERR_ICANN_NAME_COLLISION.
    // [1] https://codereview.chromium.org/2382983002
    return "IDS_WEBVIEW_BODY_UNABLE_TO_LOAD_THE_PAGE_A_SECURE_CONNECTION_"
           "COULD_NOT_BE_MADE_TO_PS_THE_MOST_LIKELY_CAUSE_IS_THE_DEVICES_"
           "CLOCK_CHECK_THAT_THE_TIME_ON_YOUR_DEVICE_IS_CORRECT_AND_REFRESH_"
           "THE_PAGE";
  }
  return "IDS_WEBVIEW_BODY_UNABLE_TO_LOAD_THE_PAGE_PS_MAY_BE_TEMPORARILY_"
         "DOWN_OR_HAVE_MOVED_TO_A_NEW_URL";
}

#if BUILDFLAG(ENABLE_NACL)
bool ContentRendererClientEfl::IsExternalPepperPlugin(
    const std::string& module_name) {
  // TODO(bbudge) remove this when the trusted NaCl plugin has been removed.
  // We must defer certain plugin events for NaCl instances since we switch
  // from the in-process to the out-of-process proxy after instantiating them.
  return module_name == "Native Client";
}
#endif

void ContentRendererClientEfl::GetNavigationErrorStrings(
    content::RenderFrame* render_frame,
    const blink::WebURLRequest& failed_request,
    const blink::WebURLError& error,
    std::string* error_html,
    base::string16* error_description) {
  if (error_html) {
    std::string errorHead(
        dgettext("WebKit", "IDS_WEBVIEW_HEADER_THIS_WEBPAGE_IS_NOT_AVAILABLE"));
    std::string errorMessage(dgettext("WebKit", GetErrorTag(error.reason)));
    errorMessage = base::StringPrintf(
        errorMessage.c_str(), error.unreachable_url.GetString().Utf8().c_str());

    *error_html =
        "<html>"
        "<head>"
        "<meta name='viewport' content='width=device-width,"
        "initial-scale=1.0, user-scalable=no'>"
        "<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'>"
        "<title>";
    *error_html += error.unreachable_url.GetString().Utf8();
    *error_html +=
        "</title>"
        "<style type=text/css>"
        "#body"
        "{"
        " background-color: #fff;"
        " margin: 0;"
        " padding: 0;"
        "}"
        "#Box"
        "{"
        " background: #fff;"
        " width: 80%%;"
        " min-width: 150px;"
        " max-width: 750px;"
        " margin: auto;"
        " padding: 5px;"
        " border: 1px solid #BFA3A3;"
        " border-radius: 1px;"
        " word-wrap:break-word"
        "}"
        "</style>"
        "</head>"
        "<body bgcolor=\"#CFCFCF\">"
        "<div id=Box>"
        "<h2 align=\"center\">";
    *error_html += errorHead;
    *error_html += "</h2></br>";
    *error_html += errorMessage;
    *error_html +=
        "</div>"
        "</body>"
        "</html>";
  }
}

void ContentRendererClientEfl::AddSupportedKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems) {
#if defined(OS_TIZEN_TV_PRODUCT)
  efl_integration::TizenAddKeySystems(key_systems);
#endif
}

std::unique_ptr<blink::WebSpeechSynthesizer>
ContentRendererClientEfl::OverrideSpeechSynthesizer(
    blink::WebSpeechSynthesizerClient* client) {
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  return std::unique_ptr<content::TtsDispatcherEfl>(
      new content::TtsDispatcherEfl(client));
#else
  return nullptr;
#endif
}

#if defined(TIZEN_PEPPER_EXTENSIONS)
bool ContentRendererClientEfl::IsPluginAllowedToUseCompositorAPI(
    const GURL& url) {
  return true;
}
#endif

void ContentRendererClientEfl::ApplyCustomMobileSettings(
    blink::WebView* webview) {
  // blink::WebViewImpl prevents auto zoom after tap if maximum legible scale
  // is too small. Blink by default sets it to 1 and needs to be enlarged to
  // make auto zoom work.
  webview->SetMaximumLegibleScale(maximum_legible_scale);

  base::CommandLine* cmdline = base::CommandLine::ForCurrentProcess();

  if (cmdline->HasSwitch(switches::kEwkEnableMobileFeaturesForDesktop)) {
    blink::WebSettings* settings = webview->GetSettings();
    settings->SetViewportStyle(blink::WebViewportStyle::kMobile);
    settings->SetViewportMetaEnabled(true);
    settings->SetAutoZoomFocusedNodeToLegibleScale(true);
    webview->SetDefaultPageScaleLimits(minimum_page_scale_for_mobile,
                                       maximum_page_scale_for_mobile);
  }
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
void ContentRendererClientEfl::RunArrowScroll(int view_id) {
  content::RenderThread::Get()->Send(new EwkHostMsg_RunArrowScroll(view_id));
}

void ContentRendererClientEfl::AutoLogin(blink::WebFrame* frame,
                                         const std::string& user_name,
                                         const std::string& password) {
  if (!frame) {
    LOG(WARNING) << "[SPASS] frame is invalid, do nothing.";
    return;
  }

  for (blink::WebFrame* fm = frame; fm != NULL; fm = fm->TraverseNext()) {
    blink::WebDocument doc = fm->ToWebLocalFrame()->GetDocument();
    blink::WebVector<blink::WebFormElement> forms;
    blink::WebString id = blink::WebString::FromUTF8(user_name);
    blink::WebString pw = blink::WebString::FromUTF8(password);
    auto autofillAgent =
        static_cast<AutofillAgent*>(fm->ToWebLocalFrame()->AutofillClient());
    bool is_username_fill = false;
    bool is_password_fill = false;
    doc.Forms(forms);
    for (size_t i = 0; i < forms.size(); ++i) {
      blink::WebFormElement fe = forms[i];
      std::vector<blink::WebFormControlElement> control_elements =
          autofill::form_util::ExtractAutofillableElementsInForm(fe);
      for (blink::WebFormControlElement& control_element : control_elements) {
        if (!control_element.HasHTMLTagName("input"))
          continue;

        blink::WebInputElement* input_element =
            ToWebInputElement(&control_element);
        if (input_element->IsPasswordField()) {
          is_password_fill = true;
          input_element->SetEditingValue(pw);
          input_element->Focus();
        } else if (autofillAgent->IsUsernameOrPasswordForm(*input_element)) {
          is_username_fill = true;
          input_element->SetEditingValue(id);
        } else
          LOG(INFO) << "[SPASS] no user name or password field";
      }
    }
    if (is_password_fill && is_username_fill) {
      LOG(INFO) << "[SPASS] fill user name & password successful";
      if (!SendEnterKeyBoardEvent(fm))
        LOG(WARNING) << "[SPASS] send enter key failed";

      return;  // already find the form, no need to check more.
    }
  }
}
#endif
