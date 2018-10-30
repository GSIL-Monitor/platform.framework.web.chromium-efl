// Copyright 2014 Samsung Electroncs. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "javascript_modal_dialog_efl.h"

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"

#ifdef OS_TIZEN
#include <efl_extension.h>
#endif

namespace content {

namespace {
static void PromptEntryChanged(void* data, Ecore_IMF_Context*, int value) {
  if (value == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
    Evas_Object* entry = static_cast<Evas_Object*>(data);
    if( entry)
      elm_object_focus_set(entry, EINA_FALSE);
  }
}

static void PromptEnterKeyDownCallback(void*, Evas_Object* obj, void*) {
  elm_entry_input_panel_hide(obj);
}
}

JavaScriptModalDialogEfl::JavaScriptModalDialogEfl(
    content::WebContents* web_contents,
    Type type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    JavaScriptDialogManager::DialogClosedCallback callback)
    : popup_(NULL),
      callback_(std::move(callback)),
      prompt_entry_(NULL),
      imf_context_(NULL) {
  Evas_Object *parent_view = static_cast<Evas_Object *>(
      web_contents->GetNativeView());
  popup_ = elm_popup_add(parent_view);
  elm_popup_orient_set(popup_, ELM_POPUP_ORIENT_CENTER);
  elm_popup_align_set(popup_, 0.5f, 0.5f);
  evas_object_event_callback_add(parent_view, EVAS_CALLBACK_RESIZE,
      &ParentViewResized, this);

  InitContent(type, message_text, default_prompt_text);
  InitButtons(type);

#if defined(OS_TIZEN)
  eext_object_event_callback_add(popup_, EEXT_CALLBACK_BACK,
      &OnCancelButtonPressed, this);
#endif

  evas_object_show(popup_);
}

JavaScriptModalDialogEfl::~JavaScriptModalDialogEfl() {
  Close();
}

void JavaScriptModalDialogEfl::SetLabelText(const base::string16& txt) {
  std::string txt2;
  base::ReplaceChars(base::UTF16ToUTF8(txt).c_str(), "\n", "</br>", &txt2);

  Evas_Object* label = elm_label_add(popup_);
  elm_label_line_wrap_set(label, ELM_WRAP_MIXED);
  elm_object_text_set(label, txt2.c_str());
  elm_object_part_content_set(popup_, "default", label);
  evas_object_show(label);
}

void JavaScriptModalDialogEfl::InitContent(Type type, const base::string16& message,
    const base::string16& content) {

  if (type == PROMPT) {
    DCHECK(message.length());
    elm_object_part_text_set(popup_, "title,text",
                             base::UTF16ToUTF8(message).c_str());
    prompt_entry_ = elm_entry_add(popup_);
    elm_entry_editable_set(prompt_entry_, EINA_TRUE);
    elm_entry_single_line_set(prompt_entry_, EINA_TRUE);
    elm_entry_scrollable_set(prompt_entry_, EINA_TRUE);
    evas_object_size_hint_weight_set(prompt_entry_, EVAS_HINT_EXPAND, 0.0f);
    evas_object_size_hint_align_set(prompt_entry_, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_entry_input_panel_return_key_type_set(prompt_entry_,
        ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
    elm_object_text_set(prompt_entry_, base::UTF16ToUTF8(content).c_str());
    elm_entry_cursor_end_set(prompt_entry_);
    evas_object_smart_callback_add(prompt_entry_, "activated",
        &PromptEnterKeyDownCallback, this);

    imf_context_ = static_cast<Ecore_IMF_Context*>(
        elm_entry_imf_context_get(prompt_entry_));
    // Can be NULL in case of platforms without SW keyboard (desktop).
    if (imf_context_) {
      ecore_imf_context_input_panel_event_callback_add(imf_context_,
          ECORE_IMF_INPUT_PANEL_STATE_EVENT, &PromptEntryChanged,
    prompt_entry_);
    }

    elm_object_part_content_set(popup_, "default", prompt_entry_);
  } else if (type == NAVIGATION) {
    if (message.length())
      elm_object_part_text_set(popup_, "title,text",
                               base::UTF16ToUTF8(message).c_str());
    if (content.length())
      SetLabelText(content);
  } else {
    DCHECK(message.length());
    SetLabelText(message);
  }
}

void JavaScriptModalDialogEfl::InitButtons(Type type) {
  DCHECK(popup_);
  switch (type) {
  case ALERT:
    AddButton(popup_, "OK", "button1", &OnOkButtonPressed);
  break;
  case CONFIRM:
    AddButton(popup_, "Cancel", "button1", &OnCancelButtonPressed);
    AddButton(popup_, "OK", "button2", &OnOkButtonPressed);
  break;
  case PROMPT:
    AddButton(popup_, "Cancel", "button1", &OnCancelButtonPressed);
    AddButton(popup_, "OK", "button2", &OnOkButtonPressed);
  break;
  case NAVIGATION:
    AddButton(popup_, "Leave", "button1", &OnOkButtonPressed);
    AddButton(popup_, "Stay", "button2", &OnCancelButtonPressed);
  break;
  default:
    NOTREACHED();
  }
}

Evas_Object* JavaScriptModalDialogEfl::AddButton(Evas_Object* parent,
    const char *label, const char *elm_part, Evas_Smart_Cb cb) {
  Evas_Object *btn = elm_button_add(parent);
  elm_object_style_set(btn, "popup");
  elm_object_text_set(btn, label);
  elm_object_part_content_set(parent, elm_part, btn);
  evas_object_smart_callback_add(btn, "clicked", cb, this);
  return btn;
}


void JavaScriptModalDialogEfl::OnOkButtonPressed(
    void* data, Evas_Object* obj, void* event_info) {
  JavaScriptModalDialogEfl *thiz = static_cast<JavaScriptModalDialogEfl*>(data);

  std::string prompt_data;
  if (thiz->prompt_entry_)
    prompt_data = elm_entry_entry_get(thiz->prompt_entry_);
  thiz->Close(true, prompt_data);
}

void JavaScriptModalDialogEfl::OnCancelButtonPressed(
    void* data, Evas_Object* obj, void* event_info) {
  JavaScriptModalDialogEfl *thiz = static_cast<JavaScriptModalDialogEfl*>(data);
  thiz->Close(false);
}

void JavaScriptModalDialogEfl::ParentViewResized(
    void *data, Evas *, Evas_Object *obj, void *info) {
  JavaScriptModalDialogEfl *thiz = static_cast<JavaScriptModalDialogEfl*>(data);
  int x, y, w, h;
  evas_object_geometry_get(obj, &x, &y, &w, &h);
  evas_object_geometry_set(thiz->popup_, x, y, w, h);
}

void JavaScriptModalDialogEfl::Close(bool accept, std::string reply) {
  if (popup_) {
    std::move(callback_).Run(accept, base::UTF8ToUTF16(reply));

#if defined(OS_TIZEN)
    eext_object_event_callback_del(popup_, EEXT_CALLBACK_BACK,
                                   CancelButtonCallback);
#endif

    evas_object_del(popup_);
    popup_ = NULL;
    prompt_entry_ = NULL;
    imf_context_ = NULL;
  }
}

} // namespace content
