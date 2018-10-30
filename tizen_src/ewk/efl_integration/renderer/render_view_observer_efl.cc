// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/render_view_observer_efl.h"

#include <string>
#include <limits.h>

#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "common/hit_test_params.h"
#include "common/render_messages_ewk.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/render_view.h"
#include "content/renderer/render_view_impl.h"
#include "public/web/WebFrameContentDumper.h"
#include "third_party/WebKit/public/platform/WebData.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/platform/WebPoint.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFormElement.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebFrameContentDumper.h"
#include "third_party/WebKit/public/web/WebFrameWidget.h"
#include "third_party/WebKit/public/web/WebHitTestResult.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebSettings.h"
#include "third_party/WebKit/public/web/WebView.h"

// XXX: config.h needs to be included before internal blink headers.
//      It'd be great if we did not include not internal blibk headers.
#include "third_party/WebKit/Source/platform/fonts/FontCache.h"

using blink::WebDocumentLoader;
using blink::WebURLRequest;

namespace {

typedef blink::WebContentSecurityPolicyType SecurityPolicyType;

static_assert(
    static_cast<int>(SecurityPolicyType::kWebContentSecurityPolicyTypeReport) ==
        static_cast<int>(EWK_REPORT_ONLY),
    "mismatching enums : EWK_REPORT_ONLY");
static_assert(
    static_cast<int>(SecurityPolicyType::kWebContentSecurityPolicyTypeEnforce) ==
        static_cast<int>(EWK_ENFORCE_POLICY),
    "mismatching enums : EWK_ENFORCE_POLICY");

SecurityPolicyType ToSecurityPolicyType(Ewk_CSP_Header_Type type) {
  return static_cast<SecurityPolicyType>(type);
}

/* LCOV_EXCL_START */
void PopulateEwkHitTestData(const blink::WebHitTestResult& web_hit_test, Hit_Test_Params* params)
{
  DCHECK(params);
  params->imageURI = web_hit_test.AbsoluteImageURL().GetString().Utf8();
  params->linkURI = web_hit_test.AbsoluteLinkURL().GetString().Utf8();
  params->linkLabel = web_hit_test.textContent().Utf8();
  params->linkTitle = web_hit_test.titleDisplayString().Utf8();
  params->isEditable = web_hit_test.IsContentEditable();

  int context = EWK_HIT_TEST_RESULT_CONTEXT_DOCUMENT;
  if (!web_hit_test.AbsoluteLinkURL().IsEmpty())
    context |= EWK_HIT_TEST_RESULT_CONTEXT_LINK;
  if (!web_hit_test.AbsoluteImageURL().IsEmpty())
    context |= EWK_HIT_TEST_RESULT_CONTEXT_IMAGE;
  if (!web_hit_test.absoluteMediaURL().IsEmpty())
    context |= EWK_HIT_TEST_RESULT_CONTEXT_MEDIA;
  if (web_hit_test.isSelected())
    context |= EWK_HIT_TEST_RESULT_CONTEXT_SELECTION;
  if (web_hit_test.IsContentEditable())
    context |= EWK_HIT_TEST_RESULT_CONTEXT_EDITABLE;
  if (web_hit_test.GetNode().IsTextNode())
    context |= EWK_HIT_TEST_RESULT_CONTEXT_TEXT;

  params->context = static_cast<Ewk_Hit_Test_Result_Context>(context);

  if (params->mode & EWK_HIT_TEST_MODE_NODE_DATA) {
    params->nodeData.nodeValue = web_hit_test.GetNode().NodeValue().Utf8();
  }

  if ((params->mode & EWK_HIT_TEST_MODE_IMAGE_DATA) &&
      (params->context & EWK_HIT_TEST_RESULT_CONTEXT_IMAGE)) {
    blink::WebElement hit_element = web_hit_test.GetNode().ToConst<blink::WebElement>();

    params->imageData.imageBitmap = hit_element.ImageContents().GetSkBitmap();
    params->imageData.fileNameExtension = hit_element.imageFilenameExtension().Utf8();
  }
}

void PopulateNodeAttributesMapFromHitTest(const blink::WebHitTestResult& web_hit_test,
                                          Hit_Test_Params* params)
{
  DCHECK(params);

  if (!web_hit_test.GetNode().IsElementNode())
    return;

  blink::WebElement hit_element = web_hit_test.GetNode().ToConst<blink::WebElement>();
  for (unsigned int i = 0; i < hit_element.AttributeCount(); i++) {
    params->nodeData.attributes.insert(std::pair<std::string, std::string>(
        hit_element.AttributeLocalName(i).Utf8(), hit_element.AttributeValue(i).Utf8()));
  }
}

}  //namespace

RenderViewObserverEfl::RenderViewObserverEfl(
    content::RenderView* render_view,
    ContentRendererClientEfl* render_client)
    : content::RenderViewObserver(render_view),
      renderer_client_(render_client) {}

RenderViewObserverEfl::~RenderViewObserverEfl() {}

bool RenderViewObserverEfl::OnMessageReceived(const IPC::Message& message)
{
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderViewObserverEfl, message)
    IPC_MESSAGE_HANDLER(EwkViewMsg_DoHitTest, OnDoHitTest)
    IPC_MESSAGE_HANDLER(EwkViewMsg_SetCSP, OnSetContentSecurityPolicy)
    IPC_MESSAGE_HANDLER(EwkViewMsg_SetScroll, OnSetScroll)
#if defined(OS_TIZEN_TV_PRODUCT)
    IPC_MESSAGE_HANDLER(EwkViewMsg_TranslatedUrlSet, OnSetTranslatedURL);
    IPC_MESSAGE_HANDLER(EwkViewMsg_SetPreferTextLang, OnSetPreferTextLang);
    IPC_MESSAGE_HANDLER(EwkViewMsg_ParentalRatingResultSet,
                        OnSetParentalRatingResult);
    IPC_MESSAGE_HANDLER(EwkViewMsg_EdgeScrollBy, OnEdgeScrollBy)
    IPC_MESSAGE_HANDLER(EwkViewMsg_RequestHitTestForMouseLongPress,
                        OnRequestHitTestForMouseLongPress)
    IPC_MESSAGE_HANDLER(EwkViewMsg_SetFloatVideoWindowState,
                        OnSetFloatVideoWindowState)
    IPC_MESSAGE_HANDLER(EwkViewMsg_SuspendNetworkLoading,
                        OnSuspendNetworkLoading);
    IPC_MESSAGE_HANDLER(EwkViewMsg_ResumeNetworkLoading,
                        OnResumeNetworkLoading);
    IPC_MESSAGE_HANDLER(EwkViewMsg_AutoLogin, OnAutoLogin)
#endif
    IPC_MESSAGE_HANDLER(EwkViewMsg_UseSettingsFont, OnUseSettingsFont)
    IPC_MESSAGE_HANDLER(EwkViewMsg_PlainTextGet, OnPlainTextGet)
    IPC_MESSAGE_HANDLER(EwkViewMsg_DoHitTestAsync, OnDoHitTestAsync)
    IPC_MESSAGE_HANDLER(EwkViewMsg_PrintToPdf, OnPrintToPdf)
    IPC_MESSAGE_HANDLER(EwkViewMsg_GetMHTMLData, OnGetMHTMLData);
    IPC_MESSAGE_HANDLER(EwkViewMsg_GetBackgroundColor, OnGetBackgroundColor);
    IPC_MESSAGE_HANDLER(EwkViewMsg_WebAppIconUrlGet, OnWebAppIconUrlGet);
    IPC_MESSAGE_HANDLER(EwkViewMsg_WebAppIconUrlsGet, OnWebAppIconUrlsGet);
    IPC_MESSAGE_HANDLER(EwkViewMsg_WebAppCapableGet, OnWebAppCapableGet);
    IPC_MESSAGE_HANDLER(EwkViewMsg_SetBackgroundColor, OnSetBackgroundColor);
    IPC_MESSAGE_HANDLER(EwkViewMsg_SetBrowserFont, OnSetBrowserFont);
    IPC_MESSAGE_HANDLER(EwkViewMsg_SuspendScheduledTask, OnSuspendScheduledTasks);
    IPC_MESSAGE_HANDLER(EwkViewMsg_ResumeScheduledTasks, OnResumeScheduledTasks);
#if defined(TIZEN_VIDEO_HOLE)
    IPC_MESSAGE_HANDLER(EwkViewMsg_SetVideoHole, OnSetVideoHole);
#endif
    IPC_MESSAGE_HANDLER(EwkViewMsg_IsVideoPlaying, OnIsVideoPlaying);
    IPC_MESSAGE_HANDLER(EwkViewMsg_SetMainFrameScrollbarVisible,
                        OnSetMainFrameScrollbarVisible)
    IPC_MESSAGE_HANDLER(EwkViewMsg_GetMainFrameScrollbarVisible,
                        OnGetMainFrameScrollbarVisible)
    IPC_MESSAGE_HANDLER(EwkSettingsMsg_UpdateWebKitPreferencesEfl, OnUpdateWebKitPreferencesEfl);
    IPC_MESSAGE_HANDLER(EwkViewMsg_ScrollFocusedNodeIntoView, OnScrollFocusedNodeIntoView);
    IPC_MESSAGE_HANDLER(ViewMsg_SetViewMode, OnSetViewMode);
    IPC_MESSAGE_HANDLER(ViewMsg_SetTextZoomFactor, OnSetTextZoomFactor)
    IPC_MESSAGE_HANDLER(EwkViewMsg_UpdateFocusedNodeBounds,
                        OnUpdateFocusedNodeBounds)
    IPC_MESSAGE_HANDLER(EwkViewMsg_SetLongPollingGlobalTimeout,
                        OnSetLongPollingGlobalTimeout)
    IPC_MESSAGE_HANDLER(EwkViewMsg_QueryNumberFieldAttributes,
                        OnQueryNumberFieldAttributes)
    IPC_MESSAGE_HANDLER(EwkViewMsg_QueryInputType, OnQueryInputType)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RenderViewObserverEfl::OnDestruct() {
  delete this;
}

void RenderViewObserverEfl::DidCreateDocumentElement(
    blink::WebLocalFrame* frame) {
  std::string policy;
  Ewk_CSP_Header_Type type = EWK_DEFAULT_POLICY;
  Send(new EwkHostMsg_GetContentSecurityPolicy(render_view()->GetRoutingID(),
                                               &policy, &type));

  // Since, Webkit supports some more types and we cast ewk type to Webkit type.
  // We allow only ewk types.
  if (type == EWK_REPORT_ONLY || type == EWK_ENFORCE_POLICY) {
    frame->GetDocument().setContentSecurityPolicyUsingHeader(
        blink::WebString::FromUTF8(policy), ToSecurityPolicyType(type));
  }
}

void RenderViewObserverEfl::OnSetContentSecurityPolicy(const std::string& policy, Ewk_CSP_Header_Type header_type)
{
  blink::WebView* view = render_view()->GetWebView();
  DCHECK(view);
  blink::WebDocument document = view->FocusedFrame()->GetDocument();

  document.setContentSecurityPolicyUsingHeader(blink::WebString::FromUTF8(policy),
      ToSecurityPolicyType(header_type));
}

void RenderViewObserverEfl::OnSetScroll(int x, int y)
{
  blink::WebLocalFrame* frame = render_view()->GetWebView()->FocusedFrame();
  if (!frame)
    return;
  frame->SetScrollOffset(blink::WebSize(x, y));
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
void RenderViewObserverEfl::OnSetTranslatedURL(const std::string url,
                                               bool* result) {
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->SetTranslatedURL(blink::WebString::FromUTF8(url), *result);
}

void RenderViewObserverEfl::OnSetPreferTextLang(const std::string& lang) {
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->SetPreferTextLang(blink::WebString::FromUTF8(lang));
}

void RenderViewObserverEfl::OnSetParentalRatingResult(const std::string& url,
                                                      bool is_pass) {
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->SetParentalRatingResult(blink::WebString::FromUTF8(url), is_pass);
}

void RenderViewObserverEfl::OnEdgeScrollBy(gfx::Point offset,
                                           gfx::Point mouse_position) {
  bool handled = false;
  blink::WebView* web_view = render_view()->GetWebView();
  if (web_view) {
    handled = web_view->EdgeScrollBy(
        blink::ScrollOffset(offset.x(), offset.y()),
        blink::WebPoint(mouse_position.x(), mouse_position.y()));
  }
  Send(new EwkHostMsg_DidEdgeScrollBy(render_view()->GetRoutingID(), offset,
                                      handled));
}

void RenderViewObserverEfl::OnRequestHitTestForMouseLongPress(int render_id,
                                                              int view_x,
                                                              int view_y) {
  const blink::WebHitTestResult web_hit_test_result =
      render_view()->GetWebView()->HitTestResultAt(
          blink::WebPoint(view_x, view_y));

  if (!web_hit_test_result.AbsoluteLinkURL().GetString().IsEmpty()) {
    Send(new EwkViewHostMsg_RequestHitTestForMouseLongPressACK(
        routing_id(), web_hit_test_result.AbsoluteLinkURL(), render_id, view_x,
        view_y));
  }
}

void RenderViewObserverEfl::OnSetFloatVideoWindowState(bool enable) {
  if (renderer_client_)
    static_cast<ContentRendererClientEfl*>(renderer_client_)
        ->SetFloatVideoWindowState(enable);

  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->SetFloatVideoWindowState(enable);
}
#endif

#if defined(TIZEN_ATK_FEATURE_VD)
void RenderViewObserverEfl::MoveFocusToBrowser(int direction) {
  Send(new EwkHostMsg_MoveFocusToBrowser(render_view()->GetRoutingID(),
                                         direction));
}
#endif

/* LCOV_EXCL_START */
void RenderViewObserverEfl::OnUseSettingsFont()
{
#if !defined(EWK_BRINGUP)
  blink::FontCache::fontCache()->invalidate();

  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->sendResizeEventAndForceLayout();
#endif
}

void RenderViewObserverEfl::OnPlainTextGet(int plain_text_get_callback_id)
{
  // WebFrameContentDumper should only be used for testing purposes. See http://crbug.com/585164.
  blink::WebString content = blink::WebFrameContentDumper::DumpWebViewAsText(
      render_view()->GetWebView(), INT_MAX);
  Send(new EwkHostMsg_PlainTextGetContents(render_view()->GetRoutingID(), content.Utf8(), plain_text_get_callback_id));
}

void RenderViewObserverEfl::OnDoHitTest(int view_x,
                                        int view_y,
                                        Ewk_Hit_Test_Mode mode) {
  Hit_Test_Params params;

  DoHitTest(view_x, view_y, mode, &params);
  Send(new EwkViewHostMsg_HitTestReply(routing_id(), params));
}

void RenderViewObserverEfl::OnDoHitTestAsync(int view_x, int view_y, Ewk_Hit_Test_Mode mode, int64_t request_id)
{
  Hit_Test_Params params;

  if (DoHitTest(view_x, view_y, mode, &params)) {
    Send(new EwkViewHostMsg_HitTestAsyncReply(routing_id(), params, request_id));
  }
}

bool RenderViewObserverEfl::DoHitTest(int view_x, int view_y, Ewk_Hit_Test_Mode mode, Hit_Test_Params* params)
{
  DCHECK(params);

  if (!render_view() || !render_view()->GetWebView())
    return false;

  const blink::WebHitTestResult web_hit_test_result =
      render_view()->GetWebView()->HitTestResultAt(
          blink::WebPoint(view_x, view_y));

  if (web_hit_test_result.GetNode().IsNull())
    return false;

  params->mode = mode;

  PopulateEwkHitTestData(web_hit_test_result, params);
  if (params->mode & EWK_HIT_TEST_MODE_NODE_DATA)
    PopulateNodeAttributesMapFromHitTest(web_hit_test_result, params);

  return true;
}

void RenderViewObserverEfl::OnPrintToPdf(int width, int height, const base::FilePath& filename)
{
  blink::WebView* web_view = render_view()->GetWebView();
  DCHECK(web_view);
  PrintWebViewHelperEfl print_helper(render_view(), filename);
  print_helper.PrintToPdf(width, height);
}

void RenderViewObserverEfl::OnGetMHTMLData(int callback_id)
{
  blink::WebView* view = render_view()->GetWebView();
  if (!view)
    return;

  std::string content_string;
#if !defined(EWK_BRINGUP)
#pragma message "[M49] WebPageSerializer has been removed from blink. Check for alternative."
  blink::WebCString content =  blink::WebPageSerializer::serializeToMHTML(view);
  if (!content.isEmpty())
    content_string = content.data();
#endif

  Send(new EwkHostMsg_ReadMHTMLData(render_view()->GetRoutingID(), content_string, callback_id));
}

void RenderViewObserverEfl::OnGetBackgroundColor(int callback_id) {
  blink::WebView* view = render_view()->GetWebView();
  if (!view)
    return;

  blink::WebColor bg_color = view->BackgroundColor();

  Send(new EwkHostMsg_GetBackgroundColor(
      render_view()->GetRoutingID(), SkColorGetR(bg_color),
      SkColorGetG(bg_color), SkColorGetB(bg_color), SkColorGetA(bg_color),
      callback_id));
}

void RenderViewObserverEfl::DidUpdateLayout()
{
  // Check if the timer is already running
  if (check_contents_size_timer_.IsRunning())
    return;

  check_contents_size_timer_.Start(FROM_HERE,
                                   base::TimeDelta::FromMilliseconds(0), this,
                                   &RenderViewObserverEfl::CheckContentsSize);
}

void RenderViewObserverEfl::CheckContentsSize()
{
  blink::WebView* view = render_view()->GetWebView();
  if (!view || !view->MainFrame() || !view->MainFrame()->IsWebLocalFrame())
    return;

  gfx::Size contents_size = view->MainFrame()->ToWebLocalFrame()->ContentsSize();

  // Fall back to contentsPreferredMinimumSize if the mainFrame is reporting a
  // 0x0 size (this happens during initial load).
  if (contents_size.IsEmpty()) {
    contents_size = render_view()->GetWebView()->ContentsPreferredMinimumSize();
  }

  if (contents_size == last_sent_contents_size_)
    return;

  last_sent_contents_size_ = contents_size;
  const blink::WebSize size = static_cast<blink::WebSize>(contents_size);
  Send(new EwkHostMsg_DidChangeContentsSize(render_view()->GetRoutingID(),
                                            size.width,
                                            size.height));
}

void RenderViewObserverEfl::OnSetBackgroundColor(int red,
                                                 int green,
                                                 int blue,
                                                 int alpha) {
  blink::WebFrameWidget* widget = render_view()->GetWebFrameWidget();
  if (!widget)
    return;

  blink::WebColor background_color =
      static_cast<blink::WebColor>(SkColorSetARGB(alpha, red, green, blue));
  widget->SetBaseBackgroundColor(background_color);
}

void RenderViewObserverEfl::OnWebAppIconUrlGet(int callback_id)
{
  blink::WebFrame *frame = render_view()->GetWebView()->MainFrame();
  if (!frame) {
    return;
  }

  blink::WebDocument document = frame->ToWebLocalFrame()->GetDocument();
  blink::WebElement head = document.Head();
  if (head.IsNull()) {
    return;
  }

  std::string iconUrl;
  std::string appleIconUrl;
  // We're looking for Apple style rel ("apple-touch-*")
  // and Google style rel ("icon"), but we prefer the Apple one
  // when both appear, as WebKit-efl was looking only for Apple style rels.
  for (blink::WebNode node = head.FirstChild(); !node.IsNull();
       node = node.NextSibling()) {
    if (!node.IsElementNode()) {
      continue;
    }
    blink::WebElement elem = node.To<blink::WebElement>();
    if (!elem.HasHTMLTagName("link")) {
      continue;
    }
    std::string rel = elem.GetAttribute("rel").Utf8();
    if (base::LowerCaseEqualsASCII(rel, "apple-touch-icon") ||              // Apple's way
        base::LowerCaseEqualsASCII(rel, "apple-touch-icon-precomposed")) {
      appleIconUrl = document.CompleteURL(elem.GetAttribute("href")).GetString().Utf8();
      break;
    } else if (base::LowerCaseEqualsASCII(rel, "icon")) {                   // Google's way
      iconUrl = document.CompleteURL(elem.GetAttribute("href")).GetString().Utf8();
    }
  }
  Send(new EwkHostMsg_WebAppIconUrlGet(render_view()->GetRoutingID(), appleIconUrl.empty() ? iconUrl : appleIconUrl, callback_id));
}

void RenderViewObserverEfl::OnWebAppIconUrlsGet(int callback_id) {
  blink::WebFrame *frame = render_view()->GetWebView()->MainFrame();
  if (!frame) {
    return;
  }

  blink::WebDocument document = frame->ToWebLocalFrame()->GetDocument();
  blink::WebElement head = document.Head();
  if (head.IsNull()) {
    return;
  }

  std::map<std::string, std::string> iconUrls;
  for (blink::WebNode node = head.FirstChild(); !node.IsNull();
       node = node.NextSibling()) {
    if (!node.IsElementNode()) {
      continue;
    }
    blink::WebElement elem = node.To<blink::WebElement>();
    if (!elem.HasHTMLTagName("link")) {
      continue;
    }
    std::string rel = elem.GetAttribute("rel").Utf8();
    if (base::LowerCaseEqualsASCII(rel, "apple-touch-icon") ||              // Apple's way
        base::LowerCaseEqualsASCII(rel, "apple-touch-icon-precomposed") ||  // same here
        base::LowerCaseEqualsASCII(rel, "icon")) {                          // Google's way
      std::string iconSize = elem.GetAttribute("sizes").Utf8();
      std::string iconUrl = document.CompleteURL(elem.GetAttribute("href")).GetString().Utf8();

      iconUrls[iconSize] = iconUrl;
    }
  }
  Send(new EwkHostMsg_WebAppIconUrlsGet(render_view()->GetRoutingID(), iconUrls, callback_id));
}

void RenderViewObserverEfl::OnWebAppCapableGet(int callback_id) {
  blink::WebFrame *frame = render_view()->GetWebView()->MainFrame();
  if (!frame)
    return;

  blink::WebDocument document = frame->ToWebLocalFrame()->GetDocument();
  blink::WebElement head = document.Head();
  if (head.IsNull())
    return;

  bool capable = false;
  for (blink::WebNode node = head.FirstChild(); !node.IsNull();
       node = node.NextSibling()) {
    if (!node.IsElementNode())
      continue;

    blink::WebElement elem = node.To<blink::WebElement>();
    if (!elem.HasHTMLTagName("meta"))
      continue;

    std::string name = elem.GetAttribute("name").Utf8();
    if (base::LowerCaseEqualsASCII(name, "apple-mobile-web-app-capable") ||   // Apple's way
        base::LowerCaseEqualsASCII(name, "mobile-web-app-capable")) {         // Google's way
      std::string content = elem.GetAttribute("content").Utf8();
      if (base::LowerCaseEqualsASCII(content, "yes")) {
        capable = true;
      }
      break;
    }
  }
  Send(new EwkHostMsg_WebAppCapableGet(render_view()->GetRoutingID(), capable, callback_id));
}

void RenderViewObserverEfl::DidHandleGestureEvent(const blink::WebGestureEvent& event)
{
  if (event.GetType() == blink::WebInputEvent::kGestureTap)
    HandleTap(event);
}

void RenderViewObserverEfl::OnSetBrowserFont()
{
  if (IsMobileProfile() || IsWearableProfile())
    blink::FontCache::GetFontCache()->SetFontFamilyTizenBrowser();
}

void RenderViewObserverEfl::OnSuspendScheduledTasks()
{
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->suspendScheduledTasks();
}

void RenderViewObserverEfl::OnResumeScheduledTasks()
{
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->resumeScheduledTasks();
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
void RenderViewObserverEfl::OnSuspendNetworkLoading() {
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->SuspendNetworkLoading();
}

void RenderViewObserverEfl::OnResumeNetworkLoading() {
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->ResumeNetworkLoading();
}
void RenderViewObserverEfl::OnAutoLogin(const std::string& user_name,
                                        const std::string& password) {
  blink::WebFrame* frame = render_view()->GetWebView()->MainFrame();
  if (renderer_client_ && frame) {
    static_cast<ContentRendererClientEfl*>(renderer_client_)
        ->AutoLogin(frame, user_name, password);
  }
}
#endif

/* LCOV_EXCL_START */
void RenderViewObserverEfl::OnIsVideoPlaying(int callback_id) {
  bool is_playing = false;
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    is_playing = view->IsVideoPlaying();
  Send(new EwkHostMsg_IsVideoPlaying(render_view()->GetRoutingID(), is_playing,
                                     callback_id));
}

void RenderViewObserverEfl::OnSetMainFrameScrollbarVisible(bool visible) {
  blink::WebView* view = render_view()->GetWebView();
  if (!view || !view->MainFrame())
    return;

  view->MainFrame()->ToWebLocalFrame()->ChangeScrollbarVisibility(visible);
}

void RenderViewObserverEfl::OnGetMainFrameScrollbarVisible(int callback_id) {
  blink::WebView* view = render_view()->GetWebView();
  if (!view)
    return;

  blink::WebLocalFrame* local_frame = view->MainFrame()->ToWebLocalFrame();
  if (!local_frame)
    return;

  bool visible = local_frame->HasHorizontalScrollbar() ||
                 local_frame->HasVerticalScrollbar();

  Send(new EwkHostMsg_GetMainFrameScrollbarVisible(
      render_view()->GetRoutingID(), visible, callback_id));
}

void RenderViewObserverEfl::OnSetViewMode(blink::WebViewMode view_mode) {
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->SetViewMode(view_mode);
}

#if defined(TIZEN_VIDEO_HOLE)
void RenderViewObserverEfl::OnSetVideoHole(bool enable) {
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->SetVideoHoleForRender(enable);
}
#endif

void RenderViewObserverEfl::OnUpdateWebKitPreferencesEfl(const WebPreferencesEfl& web_preferences_efl)
{
  blink::WebView* view = render_view()->GetWebView();
  if (view && view->GetSettings()) {
    blink::WebSettings* settings = view->GetSettings();
    settings->SetShrinksViewportContentToFit(web_preferences_efl.shrinks_viewport_content_to_fit);
    // Allows resetting the scale factor when "auto fitting" gets disabled.
    settings->SetLoadWithOverviewMode(web_preferences_efl.shrinks_viewport_content_to_fit);
    // and more if they exist in web_preferences_efl.
#if defined(OS_TIZEN)
    settings->SetHwKeyboardConnected(web_preferences_efl.hw_keyboard_connected);
#endif
  }

  DCHECK(renderer_client_);
  static_cast<ContentRendererClientEfl*>(renderer_client_)->SetAllowPopup(
      web_preferences_efl.javascript_can_open_windows_automatically_ewk);
}

void RenderViewObserverEfl::HandleTap(const blink::WebGestureEvent& event)
{
  // In order to closely match our touch adjustment logic, we
  // perform a hit test "for tap" using the same "padding" as the
  // original tap event. That way, touch adjustment picks up a link
  // for clicking, we will emit a click sound.
  blink::WebView* view = render_view()->GetWebView();
  if (!view)
    return;

  blink::WebSize size(event.data.tap.width, event.data.tap.height);
  const blink::WebHitTestResult web_hit_test_result = view->HitTestResultForTap(
      blink::WebPoint(event.x, event.y), size);
  bool hit_content_editable = web_hit_test_result.IsContentEditable();
  Send(new EwkHostMsg_HandleTapGestureWithContext(render_view()->GetRoutingID(),
      hit_content_editable));
}

void RenderViewObserverEfl::OnSetTextZoomFactor(float zoom_factor) {
  blink::WebView* view = render_view()->GetWebView();
  if (!view)
    return;

  // Hide selection and autofill popups.
  view->HidePopups();
  view->SetTextZoomFactor(zoom_factor);
}

void RenderViewObserverEfl::OnSetLongPollingGlobalTimeout(
    unsigned long timeout) {
  blink::WebView* view = render_view()->GetWebView();
  if (view)
    view->SetLongPollingGlobalTimeout(timeout);
}

void RenderViewObserverEfl::OnScrollFocusedNodeIntoView() {
  blink::WebView* view = render_view()->GetWebView();
  if (!view)
    return;

  view->scrollFocusedNodeIntoView();
}

void RenderViewObserverEfl::OnUpdateFocusedNodeBounds() {
  if (!render_view() || !render_view()->GetWebView())
    return;

  blink::WebLocalFrame* focused_frame =
      render_view()->GetWebView()->FocusedFrame();

  if (!focused_frame)
    return;

  blink::WebDocument document = focused_frame->GetDocument();

  if (document.IsNull())
    return;

  blink::WebElement element = document.FocusedElement();

  if (!element.IsNull() && element.IsElementNode()) {
    gfx::RectF focused_node_bounds_scaled =
        gfx::ScaleRect(gfx::RectF(element.BoundsInViewport()),
                       render_view()->GetWebView()->PageScaleFactor());

    Send(new EwkHostMsg_DidChangeFocusedNodeBounds(
        render_view()->GetRoutingID(), focused_node_bounds_scaled));
  }
}

void RenderViewObserverEfl::OnQueryNumberFieldAttributes() {
  if (!render_view() || !render_view()->GetWebView())
    return;

  blink::WebFrame* focused_frame = render_view()->GetWebView()->FocusedFrame();

  if (!focused_frame)
    return;

  blink::WebDocument document = focused_frame->ToWebLocalFrame()->GetDocument();

  if (document.IsNull())
    return;

  blink::WebElement element = document.FocusedElement();

  if (!element.IsNull() && element.IsElementNode()) {
    std::string type =
        element.GetAttribute(blink::WebString::FromUTF8("type")).Utf8();
    if (type.compare("number") == 0) {
      int min;
      base::StringToInt(
          element.GetAttribute(blink::WebString::FromUTF8("min")).Utf8(), &min);

      double step;
      base::StringToDouble(
          element.GetAttribute(blink::WebString::FromUTF8("step")).Utf8(),
          &step);

      double integer_part;
      bool is_step_integer = modf(step, &integer_part) == 0.0;
      Send(new EwkHostMsg_DidChangeNumberFieldAttributes(
          render_view()->GetRoutingID(), min < 0, is_step_integer));
    }
  }
}

blink::WebElement RenderViewObserverEfl::GetFocusedElement() {
  if (!render_view() || !render_view()->GetWebView())
    return blink::WebElement();

  blink::WebFrame* focused_frame = render_view()->GetWebView()->FocusedFrame();
  if (focused_frame) {
    blink::WebDocument doc = focused_frame->ToWebLocalFrame()->GetDocument();
    if (!doc.IsNull())
      return doc.FocusedElement();
  }

  return blink::WebElement();
}

void RenderViewObserverEfl::OnQueryInputType() {
  blink::WebElement element = GetFocusedElement();
  bool is_password_field = false;

  if (!element.IsNull()) {
    auto input_element = blink::ToWebInputElement(&element);
    if (input_element) {
      is_password_field = input_element->IsPasswordField();
    }
  }

  Send(new EwkHostMsg_DidChangeInputType(render_view()->GetRoutingID(),
                                         is_password_field));
}

/* LCOV_EXCL_STOP */
