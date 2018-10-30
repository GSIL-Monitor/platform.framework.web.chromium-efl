// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JAVA_SCRIPT_MODAL_DIALOG_EFL_H_
#define JAVA_SCRIPT_MODAL_DIALOG_EFL_H_

#include <string>
#include <Elementary.h>

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "url/gurl.h"

namespace content {

class JavaScriptModalDialogEfl {
 public:
  enum Type {
    ALERT,
    CONFIRM,
    NAVIGATION,
    PROMPT
  };

  JavaScriptModalDialogEfl(content::WebContents* web_contents,
                           Type type,
                           const base::string16& message_text,
                           const base::string16& default_prompt_text,
                           JavaScriptDialogManager::DialogClosedCallback);
  ~JavaScriptModalDialogEfl();

  void Close(bool accept = false, std::string reply = std::string());

 private:
  void InitContent(Type, const base::string16& message,
                   const base::string16& content);
  void InitButtons(Type);

  void SetLabelText(const base::string16& txt);
  Evas_Object* AddButton(Evas_Object* parent, const char *label,
      const char *elm_part, Evas_Smart_Cb);

  static void OnOkButtonPressed(void*, Evas_Object*, void*);
  static void OnCancelButtonPressed(void*, Evas_Object*, void*);

  static void ParentViewResized(void *, Evas *, Evas_Object *, void *);

  Evas_Object* popup_;
  JavaScriptDialogManager::DialogClosedCallback callback_;
  Evas_Object* prompt_entry_;
  Ecore_IMF_Context* imf_context_;
};

} // namespace content

#endif /* JAVA_SCRIPT_MODAL_DIALOG_EFL_H_ */
