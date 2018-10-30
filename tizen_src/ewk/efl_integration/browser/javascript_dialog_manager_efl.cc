// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/javascript_dialog_manager_efl.h"
#include "browser/javascript_modal_dialog_efl.h"
#include "common/web_contents_utils.h"
#include "content/public/browser/browser_thread.h"
#include "eweb_view.h"

using content::BrowserThread;
using web_contents_utils::WebViewFromWebContents;

JavaScriptDialogManagerEfl::JavaScriptDialogManagerEfl()
    : dialog_(nullptr) {}

JavaScriptDialogManagerEfl::~JavaScriptDialogManagerEfl() {}

void JavaScriptDialogManagerEfl::RunJavaScriptDialog(
    content::WebContents* web_contents,
    const GURL& alerting_frame_url,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    DialogClosedCallback callback,
    bool* did_suppress_message) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(web_contents);
  dialog_closed_callback_ = std::move(callback);
  EWebView* wv = WebViewFromWebContents(web_contents);
  wv->SmartCallback<EWebViewCallbacks::PopupReplyWaitStart>().call(0);
  JavaScriptModalDialogEfl::Type type = JavaScriptModalDialogEfl::ALERT;
  switch (dialog_type) {
    case content::JAVASCRIPT_DIALOG_TYPE_ALERT:
      type = JavaScriptModalDialogEfl::ALERT;
      if (alert_callback_data_.callback) {
        if (!alert_callback_data_.callback(
                wv->evas_object(), base::UTF16ToUTF8(message_text).c_str(),
                alert_callback_data_.user_data)) {
          ExecuteDialogClosedCallBack(false, std::string());
        }
        return;
      }
      break;
    case content::JAVASCRIPT_DIALOG_TYPE_CONFIRM:
      type = JavaScriptModalDialogEfl::CONFIRM;
      if (confirm_callback_data_.callback) {
        if (!confirm_callback_data_.callback(
                wv->evas_object(), base::UTF16ToUTF8(message_text).c_str(),
                confirm_callback_data_.user_data)) {
          ExecuteDialogClosedCallBack(false, std::string());
        }
        return;
      }
      break;
    case content::JAVASCRIPT_DIALOG_TYPE_PROMPT:
      type = JavaScriptModalDialogEfl::PROMPT;
      if (prompt_callback_data_.callback) {
        if (!prompt_callback_data_.callback(
                wv->evas_object(), base::UTF16ToUTF8(message_text).c_str(),
                base::UTF16ToUTF8(default_prompt_text).c_str(),
                prompt_callback_data_.user_data)) {
          ExecuteDialogClosedCallBack(false, std::string());
        }
        return;
      }
      break;
    default:
      break;
  }
  // If "*_callback_data_" doen't exist, create default popup
  dialog_.reset(JavaScriptModalDialogEfl::CreateDialogAndShow(
      web_contents, type, message_text, default_prompt_text,
      std::move(dialog_closed_callback_)));
}

void JavaScriptDialogManagerEfl::SetAlertCallback(
    Ewk_View_JavaScript_Alert_Callback callback,
    void* user_data) {
  alert_callback_data_.callback = callback;
  alert_callback_data_.user_data = user_data;
}

void JavaScriptDialogManagerEfl::SetConfirmCallback(
    Ewk_View_JavaScript_Confirm_Callback callback,
    void* user_data) {
  confirm_callback_data_.callback = callback;
  confirm_callback_data_.user_data = user_data;
}

void JavaScriptDialogManagerEfl::SetPromptCallback(
    Ewk_View_JavaScript_Prompt_Callback callback,
    void* user_data) {
  prompt_callback_data_.callback = callback;
  prompt_callback_data_.user_data = user_data;
}

void JavaScriptDialogManagerEfl::ExecuteDialogClosedCallBack(
    bool result,
    const std::string prompt_data) {
  // If default dialog was created, callback is handled in dtor of dialog.
  if (dialog_)
    dialog_.reset(nullptr);
  else
    std::move(dialog_closed_callback_)
        .Run(result, base::UTF8ToUTF16(prompt_data));
}

void JavaScriptDialogManagerEfl::SetPopupSize(int width, int height) {
  if (dialog_)
    dialog_->SetPopupSize(width, height);
}

bool JavaScriptDialogManagerEfl::IsShowing() {
  return dialog_ && dialog_->IsShowing();
}

void JavaScriptDialogManagerEfl::RunBeforeUnloadDialog(
    content::WebContents* web_contents,
    bool is_reload,
    DialogClosedCallback callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(web_contents);
  dialog_closed_callback_ = std::move(callback);
  EWebView* wv = WebViewFromWebContents(web_contents);
  wv->SmartCallback<EWebViewCallbacks::PopupReplyWaitStart>().call(0);

  // TODO(karan.kumar): remove message argument from the callback
  // http://suprem.sec.samsung.net/jira/browse/RWASP-204
  if (before_unload_confirm_panel_callback_.callback &&
      !before_unload_confirm_panel_callback_.callback(
          wv->evas_object(), nullptr,
          before_unload_confirm_panel_callback_.user_data)) {
    std::move(dialog_closed_callback_).Run(false, base::string16());
    return;
  }

  dialog_.reset(JavaScriptModalDialogEfl::CreateDialogAndShow(
      web_contents, JavaScriptModalDialogEfl::NAVIGATION,
      base::UTF8ToUTF16(
          std::string(dgettext("WebKit", "IDS_WEBVIEW_POP_LEAVE_THIS_PAGE_Q"))),
      base::string16(), std::move(dialog_closed_callback_)));
}

bool JavaScriptDialogManagerEfl::HandleJavaScriptDialog(
    content::WebContents* web_contents,
    bool accept,
    const base::string16* prompt_override) {
  NOTIMPLEMENTED();
  return false;
}

void JavaScriptDialogManagerEfl::CancelDialogs(
    content::WebContents* web_contents,
    bool reset_state) {}

#if defined(OS_TIZEN_TV_PRODUCT)
void JavaScriptDialogManagerEfl::OnDialogClosed(
    content::WebContents* web_contents) {
  EWebView* wv = WebViewFromWebContents(web_contents);
  if (wv)
    wv->OnDialogClosed();
}
#endif

void JavaScriptDialogManagerEfl::SetBeforeUnloadConfirmPanelCallback(
    Ewk_View_Before_Unload_Confirm_Panel_Callback callback,
    void* user_data) {
  before_unload_confirm_panel_callback_.callback = callback;
  before_unload_confirm_panel_callback_.user_data = user_data;
}

void JavaScriptDialogManagerEfl::ReplyBeforeUnloadConfirmPanel(
    Eina_Bool result) {
  std::move(dialog_closed_callback_).Run(result, base::string16());
}
