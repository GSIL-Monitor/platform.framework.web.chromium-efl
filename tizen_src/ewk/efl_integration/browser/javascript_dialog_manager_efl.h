// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_DIALOG_MANAGER_EFL_H_
#define EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_DIALOG_MANAGER_EFL_H_

#include <string>
#include "base/strings/string16.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "eweb_view.h"
#include "public/ewk_view_internal.h"
#include "public/ewk_view_product.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}
class JavaScriptModalDialogEfl;

class JavaScriptDialogManagerEfl : public content::JavaScriptDialogManager {
 public:
  JavaScriptDialogManagerEfl();
  ~JavaScriptDialogManagerEfl() override;

  void RunJavaScriptDialog(content::WebContents* web_contents,
                           const GURL& alerting_frame_url,
                           content::JavaScriptDialogType dialog_type,
                           const base::string16& message_text,
                           const base::string16& default_prompt_text,
                           DialogClosedCallback callback,
                           bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(content::WebContents* web_contents,
                             bool is_reload,
                             DialogClosedCallback callback) override;
  bool HandleJavaScriptDialog(content::WebContents* web_contents,
                              bool accept,
                              const base::string16* prompt_override) override;
  void CancelDialogs(content::WebContents* web_contents,
                     bool reset_state) override;

  void SetPopupSize(int width, int height);
#if defined(OS_TIZEN_TV_PRODUCT)
  void OnDialogClosed(content::WebContents* web_contents) override;
#endif
  bool IsShowing();
  void ExecuteDialogClosedCallBack(bool result, const std::string prompt_data);
  void SetAlertCallback(Ewk_View_JavaScript_Alert_Callback callback,
                        void* user_data);
  void SetConfirmCallback(Ewk_View_JavaScript_Confirm_Callback callback,
                          void* user_data);
  void SetPromptCallback(Ewk_View_JavaScript_Prompt_Callback callback,
                         void* user_data);
  void SetBeforeUnloadConfirmPanelCallback(
      Ewk_View_Before_Unload_Confirm_Panel_Callback callback,
      void* user_data);
  void ReplyBeforeUnloadConfirmPanel(Eina_Bool result);

 private:
  template <typename CallbackType>
  struct CallbackData {
    CallbackData() : callback(nullptr), user_data(nullptr) {}

    CallbackType callback;
    void* user_data;
  };

  std::unique_ptr<JavaScriptModalDialogEfl> dialog_;
  CallbackData<Ewk_View_JavaScript_Alert_Callback> alert_callback_data_;
  CallbackData<Ewk_View_JavaScript_Confirm_Callback> confirm_callback_data_;
  CallbackData<Ewk_View_JavaScript_Prompt_Callback> prompt_callback_data_;
  CallbackData<Ewk_View_Before_Unload_Confirm_Panel_Callback>
      before_unload_confirm_panel_callback_;
  DialogClosedCallback dialog_closed_callback_;

  DISALLOW_COPY_AND_ASSIGN(JavaScriptDialogManagerEfl);
};

#endif  // EWK_EFL_INTEGRATION_BROWSER_JAVASCRIPT_DIALOG_MANAGER_EFL_H_
