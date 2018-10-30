// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/password_autologin_agent.h"

#include <vector>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "components/autofill/content/renderer/form_autofill_util.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/platform/WebCoalescedInputEvent.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/platform/WebKeyboardEvent.h"
#include "third_party/WebKit/public/platform/WebVector.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebElementCollection.h"
#include "third_party/WebKit/public/web/WebFormControlElement.h"
#include "third_party/WebKit/public/web/WebFormElement.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebNode.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace {

// Returns if the word 'captcha' is present in the markup of any of the nodes
// under the given node.
bool checkMarkupForCaptcha(blink::WebFormElement element) {
  std::string markup = element.createMarkup().Utf8();
  if (markup.find("captcha") != std::string::npos ||
      markup.find("Captcha") != std::string::npos)
    return true;
  return false;
}

// Returns first image node after given node.
blink::WebNode traverseToFirstImageNode(blink::WebNode node) {
  while (!(node = node.NextSibling()).IsNull()) {
    if (!node.IsElementNode() ||
        !node.ToConst<blink::WebElement>().HasHTMLTagName("img"))
      continue;
    return node;
  }
  return blink::WebNode();
}

// Returns the first textField element node after given node.
blink::WebNode traverseToFirstFocusableTextFieldNode(blink::WebNode node) {
  while (!(node = node.NextSibling()).IsNull()) {
    if (!node.IsElementNode())
      continue;

    if (node.ToConst<blink::WebElement>().HasHTMLTagName("input") &&
        node.ToConst<blink::WebInputElement>().IsTextField() &&
        node.IsFocusable())
      return node;
  }
  return blink::WebNode();
}

// FIXME: Commented the code during dev/m61_3163 rebase.
// Needs to be fixed by Author.
/*bool hasMatchedKeywords(std::string attr, std::vector<std::string> keywords) {
  if (attr.empty())
    return false;

  std::transform(attr.begin(), attr.end(), attr.begin(), ::tolower);

  for (size_t i = 0; i < keywords.size(); i++) {
    if (attr.find(keywords.at(i)) != std::string::npos) {
      return true;
    }
  }
  return false;
}*/
}  // namespace

namespace autofill {

PasswordAutologinAgent::PasswordAutologinAgent(
    content::RenderFrame* render_frame)
    : render_frame_(render_frame) {
}

PasswordAutologinAgent::~PasswordAutologinAgent() {
}

bool PasswordAutologinAgent::DoesLoginFormContainsCaptcha(
    const blink::WebFormElement& form_element,
    const blink::WebInputElement& password_field) {
  // This method is heuristic approach. Not all site work this. But improves the
  // situation for Fingerprint screen.
  // Assuming Captcha is a combination of Image and a Text Field immediately
  // after
  // password field.
  // Traverse after the password field to find the img element and text field.
  // TODO: Improve this logic

  // Check if any image element is present after password node.
  blink::WebNode imgNode = traverseToFirstImageNode(password_field);
  if (!imgNode.IsNull()) {
    // Checks for the first focuasble textField element after image node.
    blink::WebNode textFieldNode =
        traverseToFirstFocusableTextFieldNode(imgNode);

    if (!textFieldNode.IsNull())
      return true;

  }
  blink::WebVector<blink::WebFormControlElement> control_elements;
  form_element.GetFormControlElements(control_elements);
  bool password_element_found = false;
  for (size_t i = 0; i < control_elements.size(); ++i) {
    blink::WebFormControlElement control_element = control_elements[i];
    blink::WebInputElement* input_element = ToWebInputElement(&control_element);
    if (!input_element)
      continue;
    if (!password_element_found && input_element->IsPasswordField()) {
      password_element_found = true;
      continue;
    }
    if (password_element_found ==false)
      continue;
    if (input_element->IsTextField() && input_element->IsFocusable()) {
      // Check if any element has the word 'captcha' in its markup.
      if (checkMarkupForCaptcha(form_element))
        return true;
    }
  }
  return false;
}

bool PasswordAutologinAgent::ShouldAllowAutoLogin(
    const blink::WebFormElement& form_element,
    const blink::WebInputElement& password_field) {
  bool iscaptcha = DoesLoginFormContainsCaptcha(form_element, password_field);
  // Do not allow AutoLogin when form is not visible or has captcha.
  return !(iscaptcha || !password_field.IsFocusable());
}

bool PasswordAutologinAgent::sendEnterKeyBoardEvent() {
  if (!render_frame_ || !render_frame_->GetRenderView())
    return false;
  blink::WebView* web_view_ = render_frame_->GetRenderView()->GetWebView();
  if (!web_view_)
    return false;

  blink::WebKeyboardEvent keyboard_event;
  keyboard_event.windows_key_code = ui::VKEY_RETURN;
  keyboard_event.SetModifiers(blink::WebInputEvent::kIsKeyPad);
  keyboard_event.text[0] = ui::VKEY_RETURN;
  keyboard_event.SetType(blink::WebInputEvent::kKeyDown);
  web_view_->HandleInputEvent(blink::WebCoalescedInputEvent(keyboard_event));

  keyboard_event.SetType(blink::WebInputEvent::kKeyUp);
#if defined (S_TERRACE_SUPPORT)
  return web_view_->HandleInputEvent(blink::WebCoalescedInputEvent(
             keyboard_event)) != blink::WebInputEventResult::kNotHandled;
#else
  return false;
#endif
}

bool PasswordAutologinAgent::simulateClickLoginForm(
    const blink::WebFormElement& form_element) {
// FIXME: Commented the code during dev/m61_3163 rebase.
// Needs to be fixed by Author.
#if 0
  if (form_element.WasSubmitted())
    return true;

  // Some times we need to click input element to submit the form
  // Try to click on link which has either id or onclick
  // attribute contains heuristic login terminology.
  std::vector<std::string> assumption_login_button = {
      "login", "signin", "sign-in", "confirm"};

  blink::WebVector<blink::WebFormControlElement> control_elements;
  form_element.GetFormControlElements(control_elements);
  bool password_element_found = false;
  for (size_t i = 0; i < control_elements.size(); i++) {
    blink::WebFormControlElement element = control_elements[i];
    if (element.IsNull())
      break;
    if (!form_util::IsWebElementVisible(element))
      continue;
    if (element.HasHTMLTagName("input") &&
        element.ToConst<blink::WebInputElement>().IsPasswordField() &&
        element.Value().length() > 0) {
      password_element_found = true;
      continue;
    }

    if (password_element_found) {
      bool is_login_element = false;
      if (element.HasHTMLTagName("input")) {
        if (element.ToConst<blink::WebInputElement>().IsSubmit() ||
            (element.ToConst<blink::WebInputElement>().IsButton() &&
             hasMatchedKeywords(element.GetAttribute("class").Utf8(),
                                assumption_login_button))) {
          is_login_element = true;
        }
      } else if (element.HasHTMLTagName("button") &&
                 (element.ToConst<blink::WebInputElement>().IsSubmit() ||
                  hasMatchedKeywords(element.GetAttribute("id").Utf8(),
                                     assumption_login_button) ||
                  hasMatchedKeywords(element.GetAttribute("onclick").Utf8(),
                                     assumption_login_button))) {
        is_login_element = true;
      }

      if (is_login_element) {
        if (element.IsFocusable()) {
          element.Focus();
          element.SimulateClick();
        } else {
          LOG(ERROR) << "simulateClick: found unfocusable element";
        }
        return true;
      }
    }
  }

  if (!password_element_found)
    return false;

  // Some times we need to click anchor or img to submit the form
  // Try to click on link which has either class or id or href or src
  // attribute contains heuristic login terminology.
  std::vector<std::string> assumption_login = {
      "login", "submit", "signin", "sign-in", "button"};
  std::vector<std::string> assumption_signup = {
      "signup", "sign-up", "join", "register"};

  // Previous order was 1. <a> tag and then 2. <img> tag
  // this created problem for sportsseoul.com site.
  // So we are changing the order of search
  // 1. <img> tag and then 2. <a> tag
  blink::WebElementCollection collection =
      form_element.GetElementsByHTMLTagName("img");
  for (blink::WebElement item = collection.FirstItem(); !item.IsNull();
       item = collection.NextItem()) {
    if (hasMatchedKeywords(item.GetAttribute("src").Utf8(), assumption_login)) {
      if (!form_util::IsWebElementVisible(item))
        continue;
      item.SimulateClick();
      return true;
    }
  }

  collection = form_element.GetElementsByHTMLTagName("a");
  for (blink::WebElement item = collection.FirstItem(); !item.IsNull();
       item = collection.NextItem()) {
    if (!form_util::IsWebElementVisible(item) || !item.IsFocusable())
      continue;
    if (!hasMatchedKeywords(
            item.GetAttribute("id").Utf8(), assumption_signup) &&
        (hasMatchedKeywords(item.GetAttribute("id").Utf8(), assumption_login) ||
         hasMatchedKeywords(
             item.GetAttribute("href").Utf8(), assumption_login))) {
      item.Focus();
      item.SimulateClick();
      return true;
    }
  }
#endif
  return false;
}

void PasswordAutologinAgent::SubmitFormForAutoLogin(
    blink::WebInputElement& password_input) {
  blink::WebFormElement form_element = password_input.Form();
  if (form_element.IsNull()) {
    // For support <input> elements not in a <form> element.
    sendEnterKeyBoardEvent();
    return;
  }

  if (!ShouldAllowAutoLogin(form_element, password_input))
    return;

  sendEnterKeyBoardEvent();

  if (password_input.Form().wasSubmitted())
    return;

  // KeyEvents didn't succeed to submit the form.
  // In such cases try another approach to submit the form.
  // Wait for some time as wasWebLoginSubmitted is not always correct.
  if (auto_login_click_timer.IsRunning()) {
    auto_login_click_timer.Reset();
  } else {
    auto_login_click_timer.Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(600),
        base::Bind(&PasswordAutologinAgent::autoLoginByClick,
                   base::Unretained(this), form_element));
  }
}

void PasswordAutologinAgent::autoLoginByClick(
    const blink::WebFormElement& form_element) {
  simulateClickLoginForm(form_element);
}

}  // namespace autofill
