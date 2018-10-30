// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_PASSWORD_AUTOLOGIN_AGENT_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_PASSWORD_AUTOLOGIN_AGENT_H_

#include "base/macros.h"
#include "base/timer/timer.h"

namespace blink {
class WebInputElement;
class WebFormElement;
class WebView;
class WebNode;
}

namespace content {
class RenderFrame;
}

namespace autofill {

// This class is responsible for autologin password forms.
// There is one PasswordAutologinAgent per RenderFrame.
class PasswordAutologinAgent {
 public:
  explicit PasswordAutologinAgent(content::RenderFrame* render_frame);
  ~PasswordAutologinAgent();

  // simulates autologin
  void SubmitFormForAutoLogin(blink::WebInputElement& password_input);

 private:
  // Attempts to check if the form contains any captcha
  bool DoesLoginFormContainsCaptcha(
      const blink::WebFormElement& form_element,
      const blink::WebInputElement& password_field);

  // This checks for case where Autologin should not be done
  bool ShouldAllowAutoLogin(const blink::WebFormElement& form_element,
                            const blink::WebInputElement& password_field);

  // Attempts to send enterkeydown events to blink
  bool sendEnterKeyBoardEvent();

  // attempts to simulate click on the form.
  bool simulateClickLoginForm(const blink::WebFormElement& form_element);

  void autoLoginByClick(const blink::WebFormElement& form);

  base::OneShotTimer auto_login_click_timer;

  //Pointer to render_frame
  content::RenderFrame* render_frame_;

  DISALLOW_COPY_AND_ASSIGN(PasswordAutologinAgent);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_PASSWORD_AUTOLOGIN_AGENT_H_
