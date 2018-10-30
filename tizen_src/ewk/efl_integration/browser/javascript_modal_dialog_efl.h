// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_MODAL_DIALOG_EFL_H_
#define EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_MODAL_DIALOG_EFL_H_

#include <Elementary.h>
#include <Evas.h>
#include <string>

#include "base/callback.h"
#include "base/strings/string16.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "ecore_x_wayland_wrapper.h"
#include "url/gurl.h"

class EWebView;

namespace content {
class WebContentsDelegateEfl;
}
class JavaScriptModalDialogEfl {
 public:
  enum Type {
    ALERT,
    CONFIRM,
    NAVIGATION,
    PROMPT
  };
  static JavaScriptModalDialogEfl* CreateDialogAndShow(
      content::WebContents* web_contents,
      Type type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      content::JavaScriptDialogManager::DialogClosedCallback callback);

  virtual ~JavaScriptModalDialogEfl();
  void SetPopupSize(int width, int height);
  bool IsShowing() { return is_showing_; }

 private:
  JavaScriptModalDialogEfl(
      content::WebContents* web_contents,
      Type type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      content::JavaScriptDialogManager::DialogClosedCallback callback);

  void Close();

  bool ShowJavaScriptDialog();
  Evas_Object* CreatePopup(Evas_Object* window);
  Evas_Object* CreatePopupOnNewWindow(Evas_Object* top_window);

  bool CreatePromptLayout();
  bool CreateAlertLayout();
  bool CreateConfirmLayout();
  bool CreateNavigationLayout();

  Evas_Object* AddButton(const std::string& text,
                         const std::string& part,
                         Evas_Smart_Cb callback);
  Evas_Object* AddButtonIcon(Evas_Object* btn, const std::string& img);
  bool AddCircleLayout(const std::string& title, const std::string& theme);
  std::string GetEdjPath();
  std::string GetPopupMessage(const std::string&);
  std::string GetTitle();

  static void CancelButtonCallback(void* data,
                                   Evas_Object* obj,
                                   void* event_info);
  static void OkButtonCallback(void* data, Evas_Object* obj, void* event_info);

  EWebView* web_view_;
  Evas_Object* window_;
  Evas_Object* conformant_;
  Evas_Object* popup_;
  Evas_Object* prompt_entry_;

  bool is_callback_processed_;
  bool is_showing_;

  Type type_;
  base::string16 message_text_;
  base::string16 default_prompt_text_;
  std::string popup_message_;
  content::JavaScriptDialogManager::DialogClosedCallback callback_;
};

#endif  // EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_MODAL_DIALOG_EFL_H_
