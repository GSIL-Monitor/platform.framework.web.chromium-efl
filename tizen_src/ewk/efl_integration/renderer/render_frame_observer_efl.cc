// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/render_frame_observer_efl.h"

#include "content/common/content_client_export.h"
#include "content/common/frame_messages.h"
#include "content/public/common/content_client.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
// RenderFrameImpl declares itself as a friend class to
// RenderFrameObserver. In order to access private members of
// the former from a descendant class of RenderFrameObserver,
// we define redefine "private" while including the associated header.
#define private public
#include "content/renderer/render_frame_impl.h"
#undef private
#include "content/renderer/external_popup_menu.h"
#include "common/render_messages_ewk.h"
#include "renderer/content_renderer_client_efl.h"
#include "third_party/WebKit/public/platform/URLConversion.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebElementCollection.h"
#include "third_party/WebKit/public/web/WebFormElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

#include "base/logging.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "base/macros.h"
#include "common/application_type.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "components/autofill/content/renderer/password_autofill_agent.h"
#include "private/ewk_autofill_profile_private.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
using namespace autofill::form_util;
using autofill::AutofillAgent;
using autofill::FieldValueAndPropertiesMaskMap;
using autofill::FormsPredictionsMap;
using autofill::PasswordForm;
using blink::WebString;
#endif

using blink::WebDocument;
using blink::WebElement;
using blink::WebElementCollection;
using blink::WebFrame;
using blink::WebLocalFrame;
using blink::WebView;

namespace content {

namespace {

/* LCOV_EXCL_START */
WebElement GetFocusedElement(WebLocalFrame* frame) {
  WebDocument doc = frame->GetDocument();
  if (!doc.IsNull())
    return doc.FocusedElement();

  return WebElement();
}

bool hasHTMLTagNameSelect(const WebElement& element) {
  if (element.IsNull())
    return false;

  if (element.HasHTMLTagName("select"))
    return true;

  return false;
}

#if defined(OS_TIZEN_TV_PRODUCT)
const char* const features[] = {"signup", "regist", "join",
                                "new",    "change", "create"};
constexpr size_t number_of_features = arraysize(features);

void ClearAttributeValue(std::string* value) {
  value->erase(std::remove_if(value->begin(), value->end(),
                              [](char x) { return x == '-' || x == '_'; }),
               value->end());
}

bool IsSignInForm(const blink::WebElement& element) {
  for (unsigned i = 0; i < element.AttributeCount(); ++i) {
    std::string filtered_value =
        base::ToLowerASCII(element.AttributeValue(i).Utf8());
    ClearAttributeValue(&filtered_value);

    if (filtered_value.empty())
      continue;

    for (size_t j = 0; j < number_of_features; ++j) {
      if (filtered_value.find(features[j]) != std::string::npos) {
        LOG(INFO) << "SPASS find value : " << filtered_value;
        return false;
      }
    }
  }
  LOG(INFO) << "[SPASS] it looks like signIn form";
  return true;
}
#endif

} // namespace

RenderFrameObserverEfl::RenderFrameObserverEfl(RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
}

RenderFrameObserverEfl::~RenderFrameObserverEfl() {
}

void RenderFrameObserverEfl::OnDestruct() {
  delete this;
}

bool RenderFrameObserverEfl::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderFrameObserverEfl, message)
    IPC_MESSAGE_HANDLER(EwkFrameMsg_LoadNotFoundErrorPage, OnLoadNotFoundErrorPage)
    IPC_MESSAGE_HANDLER(FrameMsg_SelectPopupMenuItems, OnSelectPopupMenuItems)
    IPC_MESSAGE_HANDLER(EwkFrameMsg_MoveToNextOrPreviousSelectElement, OnMoveToNextOrPreviousSelectElement)
    IPC_MESSAGE_HANDLER(EwkFrameMsg_RequestSelectCollectionInformation, OnRequestSelectCollectionInformation);
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RenderFrameObserverEfl::OnSelectPopupMenuItems(
    bool canceled,
    const std::vector<int>& selected_indices) {
  RenderFrameImpl* render_frame_impl_ = static_cast<RenderFrameImpl*>(render_frame());
  ExternalPopupMenu* external_popup_menu_ = render_frame_impl_->external_popup_menu_.get();
  if (external_popup_menu_ == NULL)
    return;
  // It is possible to receive more than one of these calls if the user presses
  // a select faster than it takes for the show-select-popup IPC message to make
  // it to the browser UI thread. Ignore the extra-messages.
  // TODO(jcivelli): http:/b/5793321 Implement a better fix, as detailed in bug.
  canceled = canceled || !hasHTMLTagNameSelect(GetFocusedElement(render_frame()->GetWebFrame()));
  external_popup_menu_->DidSelectItems(canceled, selected_indices);
  if (canceled)
    render_frame_impl_->DidHideExternalPopupMenu();
}

void RenderFrameObserverEfl::OnLoadNotFoundErrorPage(std::string errorUrl) {
  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  blink::WebURLError error;
  error.unreachable_url = GURL(errorUrl);
  blink::WebURLRequest failed_request(error.unreachable_url);
  bool replace = true;

  std::string error_html;
  GetContentClientExport()->renderer()->GetNavigationErrorStrings(
      render_frame(), failed_request, error, &error_html, NULL);

  frame->LoadHTMLString(error_html,
                       GURL(kUnreachableWebDataURL),
                       error.unreachable_url,
                       replace);
}

void RenderFrameObserverEfl::OnMoveToNextOrPreviousSelectElement(bool direction) {
  content::RenderView* render_view_ = render_frame()->GetRenderView();
  if (direction)
    render_view_->GetWebView()->moveFocusToNext(WebView::TraverseFocusThrough::SelectElement);
  else
    render_view_->GetWebView()->moveFocusToPrevious(WebView::TraverseFocusThrough::SelectElement);
}

void RenderFrameObserverEfl::OnRequestSelectCollectionInformation() {
  WebElement focused_element = GetFocusedElement(render_frame()->GetWebFrame());
  if (focused_element.IsNull())
    return;

  content::RenderView* render_view_ = render_frame()->GetRenderView();
  WebLocalFrame* focused_frame = render_view_->GetWebView()->FocusedFrame();
  if (!focused_frame)
    return;

  WebDocument document = focused_frame->GetDocument();
  if (document.IsNull())
      return;

  WebElementCollection select_elements = document.GetElementsByHTMLTagName("select");
  int count = 0;
  int index = 0;
  for (WebElement e = select_elements.FirstItem();
      !e.IsNull(); e = select_elements.NextItem()) {
    // take only visible elements into account
    if (e.HasNonEmptyLayoutSize()) {
      if (e == focused_element)
        index = count;
      ++count;
    }
  }

  if (count) {
    bool prev_status = index > 0;
    bool next_status = index < count - 1;

    Send(new EwkHostMsg_RequestSelectCollectionInformationUpdateACK(
        render_frame()->GetRoutingID(), count, index, prev_status, next_status));
  }
}

#if defined(OS_TIZEN_TV_PRODUCT)
void RenderFrameObserverEfl::DidFinishLoad() {
  if (!IsWebBrowser())
    return;

  blink::WebDocument doc = render_frame()->GetWebFrame()->GetDocument();
  blink::WebVector<blink::WebFormElement> forms;
  bool is_username_form;
  bool is_password_form;
  std::string user_name;
  Ewk_Form_Type type;
  auto autofillAgent = static_cast<AutofillAgent*>(
      render_frame()->GetWebFrame()->AutofillClient());
  doc.Forms(forms);
  for (size_t i = 0; i < forms.size(); ++i) {
    is_username_form = false;
    is_password_form = false;
    type = EWK_FORM_NONE;
    blink::WebFormElement fe = forms[i];
    // FIXME : only check form attributes is not enough, some websites do not
    // indicate in form element, have to check more in input element.
    if (!IsSignInForm(fe))
      continue;

    FieldValueAndPropertiesMaskMap field_value_and_properties_map;
    FormsPredictionsMap form_predictions;

    std::unique_ptr<PasswordForm> password_form;
    password_form = CreatePasswordFormFromWebForm(
        fe, &field_value_and_properties_map, &form_predictions);
    if (!password_form) {
      LOG(INFO) << "[SPASS] CreatePasswordFormFromWebForm is invalid";
      continue;
    }
    GURL action_url = fe.IsNull()
                          ? GURL()
                          : autofill::form_util::GetCanonicalActionForForm(fe);

    std::vector<blink::WebFormControlElement> control_elements =
        autofill::form_util::ExtractAutofillableElementsInForm(fe);

    for (const blink::WebFormControlElement& control_element :
         control_elements) {
      if (!control_element.HasHTMLTagName("input"))
        continue;
      bool is_element_visible =
          autofill::form_util::IsWebElementVisible(control_element);
      const blink::WebInputElement* input_element =
          ToWebInputElement(&control_element);
      LOG(INFO) << "[SPASS] check input element";
      if (!IsSignInForm(control_element)) {
        is_username_form = false;
        is_password_form = false;
        break;
      }
      if (input_element->IsPasswordField() && is_element_visible) {
        is_password_form = true;
      } else if (autofillAgent->IsUsernameOrPasswordForm(*input_element)) {
        user_name = input_element->Value().Utf8();
        if (is_element_visible)
          is_username_form = true;
      } else
        LOG(INFO) << "[SPASS] no user or password field";
    }

    if (is_username_form) {
      if (is_password_form)
        type = EWK_FORM_BOTH;
      else
        type = EWK_FORM_USERNAME;
    } else {
      if (is_password_form)
        type = EWK_FORM_PASSWORD;
      else
        type = EWK_FORM_NONE;
    }
    if (type != EWK_FORM_NONE) {
      Send(new EwkHostMsg_LoginFieldsIdentified(
          render_frame()->GetRoutingID(), static_cast<int>(type), user_name,
          base::UTF16ToUTF8(password_form->username_element),
          base::UTF16ToUTF8(password_form->password_element),
          action_url.spec()));
    }
  }
}

void RenderFrameObserverEfl::WillSendSubmitEvent(
    const blink::WebFormElement& form) {
  GURL url(blink::WebStringToGURL(form.Action()));

  if (IsWebBrowser()) {
    LOG(INFO) << "[SPASS] " << __FUNCTION__;
    std::unique_ptr<PasswordForm> password_form;
    password_form =
        autofill::CreatePasswordFormFromWebForm(form, nullptr, nullptr);
    GURL action_url =
        form.IsNull() ? GURL()
                      : autofill::form_util::GetCanonicalActionForForm(form);
    if (password_form && !password_form->username_value.empty() &&
        !password_form->password_value.empty()) {
      LOG(INFO) << "[SPASS] Send Login Submit";
      Send(new EwkHostMsg_LoginFormSubmitted(
          render_frame()->GetRoutingID(),
          base::UTF16ToUTF8(password_form->username_value),
          base::UTF16ToUTF8(password_form->password_value),
          base::UTF16ToUTF8(password_form->username_element),
          base::UTF16ToUTF8(password_form->password_element),
          action_url.spec()));
    }
  }
}
#endif

void RenderFrameObserverEfl::DidChangeScrollOffset() {
  if (render_frame()->GetRenderView()->GetMainRenderFrame() != render_frame())
    return;

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();

  blink::WebSize contentsSize = frame->ContentsSize();
  blink::WebRect visibleContentRect = frame->VisibleContentRect();
  blink::WebSize maximumScrollOffset(
      contentsSize.width - visibleContentRect.width,
      contentsSize.height - visibleContentRect.height);

  if (max_scroll_offset_ != maximumScrollOffset) {
    max_scroll_offset_ = maximumScrollOffset;
    Send(new EwkHostMsg_DidChangeMaxScrollOffset(render_frame()->GetRoutingID(),
                                                 maximumScrollOffset.width,
                                                 maximumScrollOffset.height));
  }

  if (last_scroll_offset_ != frame->GetScrollOffset()) {
    last_scroll_offset_ = frame->GetScrollOffset();
    Send(new EwkHostMsg_DidChangeScrollOffset(render_frame()->GetRoutingID(),
                                              frame->GetScrollOffset().width,
                                              frame->GetScrollOffset().height));
  }
}

void RenderFrameObserverEfl::WillSubmitForm(
    const blink::WebFormElement& form) {
  GURL url(blink::WebStringToGURL(form.Action()));
  Send(new EwkHostMsg_FormSubmit(render_frame()->GetRoutingID(), url));
}

void RenderFrameObserverEfl::DidCreateScriptContext(
    v8::Local<v8::Context> context, int world_id) {
  ContentRendererClientEfl* client = static_cast<ContentRendererClientEfl*>(
      GetContentClientExport()->renderer());

  client->DidCreateScriptContext(
      render_frame()->GetWebFrame(), context, world_id);
}

void RenderFrameObserverEfl::WillReleaseScriptContext(
    v8::Handle<v8::Context> context, int world_id) {
  ContentRendererClientEfl* client = static_cast<ContentRendererClientEfl*>(
      GetContentClientExport()->renderer());

  client->WillReleaseScriptContext(
      render_frame()->GetWebFrame(), context, world_id);
}
/* LCOV_EXCL_STOP */

} // namespace content
