// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JAVASCRIPT_DIALOG_MANAGER_EFL_H_
#define JAVASCRIPT_DIALOG_MANAGER_EFL_H_

#include <string>
#include <map>
#include <Evas.h>

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/strings/string16.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/common/javascript_dialog_type.h"
#include "url/gurl.h"

namespace content{

class WebContents;
class JavaScriptModalDialogEfl;

class JavaScriptDialogManagerEfl: public JavaScriptDialogManager {
 public:
  JavaScriptDialogManagerEfl();
  ~JavaScriptDialogManagerEfl() override;

  virtual void RunJavaScriptDialog(WebContents* web_contents,
                                   const GURL& alerting_frame_url,
                                   JavaScriptDialogType dialog_type,
                                   const base::string16& message_text,
                                   const base::string16& default_prompt_text,
                                   DialogClosedCallback callback,
                                   bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(content::WebContents* web_contents,
                             bool is_reload,
                             DialogClosedCallback callback) override;
  bool HandleJavaScriptDialog(
      content::WebContents* web_contents,
      bool accept,
      const base::string16* prompt_override) override;
  void CancelDialogs(content::WebContents* web_contents,
                     bool reset_state) override;

 private:
  std::map<WebContents*, JavaScriptModalDialogEfl*> open_dialogs_;

  DISALLOW_COPY_AND_ASSIGN(JavaScriptDialogManagerEfl);
};

} // namespace content

#endif /* JAVASCRIPT_DIALOG_MANAGER_EFL_H_ */
