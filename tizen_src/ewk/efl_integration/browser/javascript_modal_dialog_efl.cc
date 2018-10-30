// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/javascript_modal_dialog_efl.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "common/web_contents_utils.h"
#include "content/common/paths_efl.h"
#include "eweb_view.h"
#include "ui/display/device_display_info_efl.h"

#ifdef OS_TIZEN
#include <efl_extension.h>
#endif

using web_contents_utils::WebViewFromWebContents;

// static
JavaScriptModalDialogEfl* JavaScriptModalDialogEfl::CreateDialogAndShow(
    content::WebContents* web_contents,
    Type type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback callback) {
  JavaScriptModalDialogEfl* dialog =
      new JavaScriptModalDialogEfl(web_contents, type, message_text,
                                   default_prompt_text, std::move(callback));
  if (!dialog->ShowJavaScriptDialog()) {
    LOG(ERROR) << "Could not create javascript dialog.";
    delete dialog;
    dialog = nullptr;
  }
  return dialog;
}

JavaScriptModalDialogEfl::JavaScriptModalDialogEfl(
    content::WebContents* web_contents,
    Type type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback callback)
    : web_view_(WebViewFromWebContents(web_contents)),
      window_(nullptr),
      conformant_(nullptr),
      popup_(nullptr),
      prompt_entry_(nullptr),
      is_callback_processed_(false),
      is_showing_(false),
      type_(type),
      message_text_(message_text),
      default_prompt_text_(default_prompt_text),
      callback_(std::move(callback)) {}

static void PromptEntryChanged(void* data, Ecore_IMF_Context* ctx, int value) {
  if (value != ECORE_IMF_INPUT_PANEL_STATE_HIDE)
    return;

  Evas_Object* entry = static_cast<Evas_Object*>(data);
  if (entry)
    elm_object_focus_set(entry, EINA_FALSE);
}

static void PromptEnterKeyDownCallback(void* data,
                                       Evas_Object* obj,
                                       void* eventInfo) {
  elm_entry_input_panel_hide(obj);
}

bool JavaScriptModalDialogEfl::ShowJavaScriptDialog() {
  Evas_Object* parent = elm_object_parent_widget_get(web_view_->evas_object());
  if (!parent)
    return false;

  Evas_Object* top_window = elm_object_top_widget_get(parent);

  if (IsMobileProfile())
    popup_ = CreatePopupOnNewWindow(top_window);
  else
    popup_ = CreatePopup(top_window);

  if (!popup_)
    return false;

  if (IsWearableProfile())
    elm_object_style_set(popup_, "circle");

  switch (type_) {
    case PROMPT:
      if (!CreatePromptLayout())
        return false;
      break;

    case NAVIGATION:
      if (!CreateNavigationLayout())
        return false;
      break;
    case ALERT:
      if (!CreateAlertLayout())
        return false;
      break;
    case CONFIRM:
      if (!CreateConfirmLayout())
        return false;
      break;
  }

#if defined(OS_TIZEN)
  eext_object_event_callback_add(popup_, EEXT_CALLBACK_BACK,
                                 CancelButtonCallback, this);
#endif
  evas_object_focus_set(popup_, true);
  evas_object_show(popup_);

  is_showing_ = true;

  return true;
}

Evas_Object* JavaScriptModalDialogEfl::CreatePopup(Evas_Object* window) {
  if (!window)
    return nullptr;

  conformant_ = elm_conformant_add(window);
  if (!conformant_)
    return nullptr;

  evas_object_size_hint_weight_set(conformant_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  elm_win_resize_object_add(window, conformant_);
  evas_object_show(conformant_);
  Evas_Object* layout = elm_layout_add(conformant_);
  if (!layout)
    return nullptr;

  elm_layout_theme_set(layout, "layout", "application", "default");
  evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_show(layout);

  elm_object_content_set(conformant_, layout);

  return elm_popup_add(layout);
}

Evas_Object* JavaScriptModalDialogEfl::CreatePopupOnNewWindow(
    Evas_Object* top_window) {
  if (!top_window)
    return nullptr;

  window_ = elm_win_add(top_window, "JavaScriptModalDialogEfl", ELM_WIN_BASIC);
  if (!window_)
    return nullptr;

  elm_win_alpha_set(window_, EINA_TRUE);

  if (elm_win_indicator_mode_get(top_window) == ELM_WIN_INDICATOR_SHOW)
    elm_win_indicator_mode_set(window_, ELM_WIN_INDICATOR_SHOW);

  if (elm_win_indicator_opacity_get(top_window) ==
      ELM_WIN_INDICATOR_TRANSPARENT)
    elm_win_indicator_opacity_set(window_, ELM_WIN_INDICATOR_TRANSPARENT);

  if (elm_win_wm_rotation_supported_get(top_window)) {
    int rots[] = {0, 90, 180, 270};
    elm_win_wm_rotation_available_rotations_set(window_, rots, 4);
  }

  display::DeviceDisplayInfoEfl display_info;
  evas_object_resize(window_, display_info.GetDisplayWidth(),
                     display_info.GetDisplayHeight());

  Evas_Object* popup = CreatePopup(window_);
  elm_win_conformant_set(window_, EINA_TRUE);
  evas_object_show(window_);

  return popup;
}

bool JavaScriptModalDialogEfl::CreatePromptLayout() {
  Evas_Object* layout = elm_layout_add(popup_);
  if (!layout)
    return false;
  if (IsWearableProfile()) {
    elm_layout_theme_set(layout, "layout", "popup", "content/circle/buttons2");
    if (message_text_.c_str()) {
      elm_object_part_text_set(layout, "elm.text.title",
                               base::UTF16ToUTF8(message_text_).c_str());
    }
  } else {
    if (message_text_.c_str()) {
      elm_object_part_text_set(popup_, "title,text",
                               base::UTF16ToUTF8(message_text_).c_str());
    }

    elm_layout_file_set(layout, GetEdjPath().c_str(), "prompt");
    evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
  }

  prompt_entry_ = elm_entry_add(popup_);
  if (!prompt_entry_)
    return false;

  Ecore_IMF_Context* imf_context =
      static_cast<Ecore_IMF_Context*>(elm_entry_imf_context_get(prompt_entry_));
  ecore_imf_context_input_panel_event_callback_add(
      imf_context, ECORE_IMF_INPUT_PANEL_STATE_EVENT, PromptEntryChanged,
      prompt_entry_);
  elm_entry_single_line_set(prompt_entry_, EINA_TRUE);
  elm_entry_input_panel_return_key_type_set(
      prompt_entry_, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
  evas_object_smart_callback_add(prompt_entry_, "activated",
                                 PromptEnterKeyDownCallback, this);
  elm_object_text_set(prompt_entry_,
                      base::UTF16ToUTF8(default_prompt_text_).c_str());
  elm_entry_scrollable_set(prompt_entry_, EINA_TRUE);
  elm_entry_cursor_end_set(prompt_entry_);

  if (IsWearableProfile())
    elm_object_part_content_set(layout, "elm.swallow.content", prompt_entry_);
  else
    elm_object_part_content_set(layout, "prompt_container", prompt_entry_);

  elm_object_content_set(popup_, layout);
  Evas_Object* cancel_btn = AddButton("IDS_WEBVIEW_BUTTON_CANCEL_ABB4",
                                      "button1", CancelButtonCallback);
  Evas_Object* ok_btn =
      AddButton("IDS_WEBVIEW_BUTTON_OK_ABB4", "button2", OkButtonCallback);
  if (!cancel_btn || !ok_btn)
    return false;

  evas_object_focus_set(ok_btn, true);
  return true;
}

bool JavaScriptModalDialogEfl::CreateNavigationLayout() {
  if (message_text_.c_str()) {
    elm_object_part_text_set(popup_, "title,text",
                             base::UTF16ToUTF8(message_text_).c_str());
  }

  std::string question(dgettext("WebKit", "IDS_WEBVIEW_POP_LEAVE_THIS_PAGE_Q"));
  std::string message;
  if (default_prompt_text_.c_str()) {
    message = std::string(base::UTF16ToUTF8(default_prompt_text_).c_str()) +
              ("\n") + question;
  } else {
    message = question;
  }
  if (IsWearableProfile() &&
      !AddCircleLayout(message, "content/circle/buttons2")) {
    return false;
  } else if (!message_text_.empty()) {
    // Use GetPopupMessage() to modify the message_text_
    elm_object_text_set(
        popup_, GetPopupMessage(base::UTF16ToUTF8(message_text_)).c_str());
  }
  Evas_Object* cancel_btn =
      AddButton("IDS_WEBVIEW_BUTTON_STAY", "button1", CancelButtonCallback);
  Evas_Object* ok_btn =
      AddButton("IDS_WEBVIEW_BUTTON_LEAVE", "button2", OkButtonCallback);
  if (!cancel_btn || !ok_btn)
    return false;

  evas_object_focus_set(ok_btn, true);

  return true;
}

bool JavaScriptModalDialogEfl::CreateAlertLayout() {
  elm_object_part_text_set(popup_, "title,text", GetTitle().c_str());
  if (IsWearableProfile() && !AddCircleLayout(base::UTF16ToUTF8(message_text_),
                                              "content/circle/buttons1")) {
    return false;
  } else if (!message_text_.empty()) {
    // Use GetPopupMessage() to modify the message_text_
    elm_object_text_set(
        popup_, GetPopupMessage(base::UTF16ToUTF8(message_text_)).c_str());
  }

  Evas_Object* ok_btn =
      AddButton("IDS_WEBVIEW_BUTTON_OK_ABB4", "button1", OkButtonCallback);
  if (!ok_btn)
    return false;

  evas_object_focus_set(ok_btn, true);
  return true;
}
bool JavaScriptModalDialogEfl::CreateConfirmLayout() {
  elm_object_part_text_set(popup_, "title,text", GetTitle().c_str());
  if (IsWearableProfile() && !AddCircleLayout(base::UTF16ToUTF8(message_text_),
                                              "content/circle/buttons2")) {
    return false;
  } else if (!message_text_.empty()) {
    // Use GetPopupMessage() to modify the message_text_
    elm_object_text_set(
        popup_, GetPopupMessage(base::UTF16ToUTF8(message_text_)).c_str());
  }
  Evas_Object* cancel_btn = AddButton("IDS_WEBVIEW_BUTTON_CANCEL_ABB4",
                                      "button1", CancelButtonCallback);
  Evas_Object* ok_btn =
      AddButton("IDS_WEBVIEW_BUTTON_OK_ABB4", "button2", OkButtonCallback);
  if (!cancel_btn || !ok_btn)
    return false;

  evas_object_focus_set(ok_btn, true);

  return true;
}

std::string JavaScriptModalDialogEfl::GetTitle() {
  const GURL& url = web_view_->GetURL();
  std::string text;
  if (url.SchemeIsFile() || url.is_empty())
    text = std::string(web_view_->GetTitle());
  else
    text = url.possibly_invalid_spec();

  std::string title =
      dgettext("WebKit", "IDS_WEBVIEW_HEADER_MESSAGE_FROM_PS_M_WEBSITE");

  const std::string replaceStr("%s");
  size_t pos = title.find(replaceStr);
  if (pos != std::string::npos)
    title.replace(pos, replaceStr.length(), text);

  return title;
}

std::string JavaScriptModalDialogEfl::GetPopupMessage(const std::string& str) {
  if (str.empty())
    return str;
  // Replace new line with break in the string
  std::string message = std::string(elm_entry_utf8_to_markup(str.c_str()));
  base::ReplaceChars(message, "\n", "</br>", &message);

  if (!IsWearableProfile())
    message = "<color='#000000'>" + message + "</color>";

  return message;
}

std::string JavaScriptModalDialogEfl::GetEdjPath() {
  base::FilePath edj_dir;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  return edj_dir.Append(FILE_PATH_LITERAL("JavaScriptPopup.edj"))
      .AsUTF8Unsafe();
}

void JavaScriptModalDialogEfl::CancelButtonCallback(void* data,
                                                    Evas_Object* obj,
                                                    void* event_info) {
  JavaScriptModalDialogEfl* dialog =
      static_cast<JavaScriptModalDialogEfl*>(data);

  std::move(dialog->callback_).Run(false, base::string16());
  evas_object_del(dialog->popup_);
  dialog->is_callback_processed_ = true;
  dialog->Close();
  dialog->web_view_->SmartCallback<EWebViewCallbacks::PopupReplyWaitFinish>()
      .call(nullptr);
}

void JavaScriptModalDialogEfl::OkButtonCallback(void* data,
                                                Evas_Object* obj,
                                                void* event_info) {
  JavaScriptModalDialogEfl* dialog =
      static_cast<JavaScriptModalDialogEfl*>(data);
  if (dialog->type_ == PROMPT && dialog->prompt_entry_) {
    std::move(dialog->callback_)
        .Run(true,
             base::UTF8ToUTF16(elm_entry_entry_get(dialog->prompt_entry_)));
  } else {
    std::move(dialog->callback_).Run(true, base::string16());
  }
  dialog->is_callback_processed_ = true;
  dialog->Close();
  dialog->web_view_->SmartCallback<EWebViewCallbacks::PopupReplyWaitFinish>()
      .call(nullptr);
}

void JavaScriptModalDialogEfl::Close() {
  if (!is_callback_processed_)
    std::move(callback_).Run(false, base::string16());

  if (IsMobileProfile() && window_) {
    evas_object_del(window_);
    window_ = nullptr;
    conformant_ = nullptr;
  } else if (conformant_) {
    evas_object_del(conformant_);
    conformant_ = nullptr;
  }

  popup_ = nullptr;
  prompt_entry_ = nullptr;

  is_showing_ = false;
}

void JavaScriptModalDialogEfl::SetPopupSize(int width, int height) {
  if (!popup_)
    return;

  evas_object_resize(popup_, width, height);
  evas_object_move(popup_, 0, 0);
}

JavaScriptModalDialogEfl::~JavaScriptModalDialogEfl() {
  Close();
}

Evas_Object* JavaScriptModalDialogEfl::AddButton(const std::string& text,
                                                 const std::string& part,
                                                 Evas_Smart_Cb callback) {
  Evas_Object* btn = elm_button_add(popup_);
  if (!btn)
    return nullptr;
  if (IsWearableProfile()) {
    Evas_Object* img;
    if (type_ == ALERT) {
      elm_object_style_set(btn, "bottom");
      img = AddButtonIcon(btn, "popup_btn_ok.png");
    } else if (part.compare("button1") == 0) {
      elm_object_style_set(btn, "popup/circle/left");
      img = AddButtonIcon(btn, "popup_btn_cancel.png");
    } else {
      elm_object_style_set(btn, "popup/circle/right");
      img = AddButtonIcon(btn, "popup_btn_ok.png");
    }

    if (!img)
      return nullptr;
  } else {
    elm_object_style_set(btn, "popup");
    elm_object_domain_translatable_part_text_set(btn, NULL, "WebKit",
                                                 text.c_str());
  }
  elm_object_part_content_set(popup_, part.c_str(), btn);
  evas_object_smart_callback_add(btn, "clicked", callback, this);
  return btn;
}

Evas_Object* JavaScriptModalDialogEfl::AddButtonIcon(Evas_Object* btn,
                                                     const std::string& img) {
  if (!IsWearableProfile())
    return nullptr;

  Evas_Object* icon = elm_image_add(btn);
  if (!icon)
    return nullptr;

  base::FilePath img_dir;
  PathService::Get(PathsEfl::IMAGE_RESOURCE_DIR, &img_dir);
  std::string file_path = img_dir.Append(FILE_PATH_LITERAL(img)).AsUTF8Unsafe();
  if (!elm_image_file_set(icon, file_path.c_str(), nullptr))
    return nullptr;

  evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  elm_object_part_content_set(btn, "elm.swallow.content", icon);
  evas_object_show(icon);
  return icon;
}

bool JavaScriptModalDialogEfl::AddCircleLayout(const std::string& title,
                                               const std::string& theme) {
  if (!IsWearableProfile())
    return false;

  Evas_Object* layout = elm_layout_add(popup_);
  if (!layout)
    return false;

  elm_layout_theme_set(layout, "layout", "popup", theme.c_str());

  if (!title.empty()) {
    elm_object_part_text_set(layout, "elm.text",
                             GetPopupMessage(title).c_str());
  }

  elm_object_content_set(popup_, layout);

  return true;
}
