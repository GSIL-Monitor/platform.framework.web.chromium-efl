// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/js_dialogs_efl/javascript_dialog_manager_efl.h"

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/strings/utf_string_conversions.h"
#include "components/js_dialogs_efl/javascript_modal_dialog_efl.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace content {

JavaScriptDialogManagerEfl::JavaScriptDialogManagerEfl() {
}

JavaScriptDialogManagerEfl::~JavaScriptDialogManagerEfl() {
}
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

  JavaScriptModalDialogEfl::Type efl_type;

  switch (dialog_type) {
    case JAVASCRIPT_DIALOG_TYPE_ALERT:
      efl_type = JavaScriptModalDialogEfl::ALERT;
      break;
    case JAVASCRIPT_DIALOG_TYPE_CONFIRM:
      efl_type = JavaScriptModalDialogEfl::CONFIRM;
      break;
    case JAVASCRIPT_DIALOG_TYPE_PROMPT:
      efl_type = JavaScriptModalDialogEfl::PROMPT;
      break;
    default:
      NOTREACHED();
  }
  CancelDialogs(web_contents, false);
  open_dialogs_[web_contents] =
      new JavaScriptModalDialogEfl(web_contents, efl_type, message_text,
                                   default_prompt_text, std::move(callback));
  *did_suppress_message = false;
}

void JavaScriptDialogManagerEfl::RunBeforeUnloadDialog(
    content::WebContents* web_contents,
    bool is_reload,
    DialogClosedCallback callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  CancelDialogs(web_contents, false);
  open_dialogs_[web_contents] = new JavaScriptModalDialogEfl(
      web_contents, JavaScriptModalDialogEfl::NAVIGATION,
      base::UTF8ToUTF16(
          std::string(dgettext("WebKit", "IDS_WEBVIEW_POP_LEAVE_THIS_PAGE_Q"))),
      base::string16(), std::move(callback));
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
    bool reset_state) {
  if (open_dialogs_.find(web_contents) != open_dialogs_.end()) {
    open_dialogs_[web_contents]->Close();
    delete open_dialogs_[web_contents];
  }
}

} // namespace content
