// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/im_context_efl.h"

#include <Ecore_Evas.h>
#include <Ecore_IMF_Evas.h>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "build/tizen_version.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/common/input_messages.h"
#include "content/public/browser/web_contents_delegate.h"
#include "third_party/WebKit/public/web/WebImeTextSpan.h"
#include "third_party/icu/source/common/unicode/uchar.h"
#include "tizen/system_info.h"
#include "ui/base/clipboard/clipboard_helper_efl.h"

// TODO: Accessing from chromium_impl to ewk is prohibited, so we have to
// access ewk via WebContentDelegateEfl.
#if defined(OS_TIZEN_MOBILE_PRODUCT)
#include "common/render_messages_ewk.h"
#include "common/web_contents_utils.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "eweb_view.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include "tizen_src/ewk/efl_integration/common/application_type.h"
#endif

// FIXME: we do not handle text compositing correctly yet.
// Limit the functinality of this class to the handling of virtual keyboard for
// now.
#define USE_IM_COMPOSITING 1

#if defined(OS_TIZEN_MOBILE_PRODUCT)
using web_contents_utils::WebViewFromWebContents;
#endif

namespace {

Ecore_IMF_Context* CreateIMFContext(Evas* evas) {
  const char* default_context_id = ecore_imf_context_default_id_get();
  if (!default_context_id) {
    LOG(ERROR) << "no default context id";
    return NULL;
  }
  Ecore_IMF_Context* context = ecore_imf_context_add(default_context_id);
  if (!context) {
    LOG(ERROR) << "cannot create context";
    return NULL;
  }

  Ecore_Window window = ecore_evas_window_get(ecore_evas_ecore_evas_get(evas));
  ecore_imf_context_client_window_set(context, reinterpret_cast<void*>(window));
  ecore_imf_context_client_canvas_set(context, evas);

  return context;
}

}  // namespace

namespace content {

// static
IMContextEfl* IMContextEfl::Create(RenderWidgetHostViewEfl* view) {
  Ecore_IMF_Context* context = CreateIMFContext(view->evas());
  if (!context)
    return NULL;

  return new IMContextEfl(view, context);
}

IMContextEfl::IMContextEfl(RenderWidgetHostViewEfl* view,
                           Ecore_IMF_Context* context)
    : view_(view),
      context_(context),
      is_focused_(false),
      is_in_form_tag_(false),
      is_showing_(false),
      current_mode_(ui::TEXT_INPUT_MODE_DEFAULT),
      current_type_(ui::TEXT_INPUT_TYPE_NONE),
      can_compose_inline_(false),
      is_ime_ctx_reset_(false),
      should_show_on_resume_(false),
      is_get_lookup_table_from_app_(false),
      caret_position_(0),
#if defined(OS_TIZEN_TV_PRODUCT)
      password_input_minlength_(-1),
      surrounding_text_length_(0),
#endif
      is_surrounding_text_change_in_progress_(false),
      is_keyevent_processing_(false),
      is_transaction_finished_(true) {
  InitializeIMFContext(context_);
}

void IMContextEfl::InitializeIMFContext(Ecore_IMF_Context* context) {
  ecore_imf_context_input_panel_enabled_set(context, false);
  ecore_imf_context_use_preedit_set(context, false);

  ecore_imf_context_event_callback_add(context, ECORE_IMF_CALLBACK_COMMIT,
                                       &IMFCommitCallback, this);
  ecore_imf_context_event_callback_add(context,
                                       ECORE_IMF_CALLBACK_DELETE_SURROUNDING,
                                       &IMFDeleteSurroundingCallback, this);
  ecore_imf_context_event_callback_add(context,
                                       ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
                                       &IMFPreeditChangedCallback, this);
  ecore_imf_context_event_callback_add(
      context, ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
      &IMFTransactionPrivateCommandSendCallback, this);
#if defined(OS_TIZEN_TV_PRODUCT)
  ecore_imf_context_event_callback_add(context,
                                       ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
                                       &IMFEventPrivateCommandCallback, this);
#endif
  ecore_imf_context_input_panel_event_callback_add(
      context, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
      &IMFInputPanelStateChangedCallback, this);
  ecore_imf_context_input_panel_event_callback_add(
      context, ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT,
      &IMFInputPanelGeometryChangedCallback, this);
  ecore_imf_context_input_panel_event_callback_add(
      context, ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT,
      &IMFCandidatePanelStateChangedCallback, this);
  ecore_imf_context_input_panel_event_callback_add(
      context, ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT,
      &IMFCandidatePanelGeometryChangedCallback, this);
  ecore_imf_context_input_panel_event_callback_add(
      context, ECORE_IMF_INPUT_PANEL_LANGUAGE_EVENT,
      &IMFCandidatePanelLanguageChangedCallback, this);
  ecore_imf_context_retrieve_surrounding_callback_set(
      context, &IMFRetrieveSurroundingCallback, this);
}

IMContextEfl::~IMContextEfl() {
  ecore_imf_context_event_callback_del(context_, ECORE_IMF_CALLBACK_COMMIT,
                                       &IMFCommitCallback);
  ecore_imf_context_event_callback_del(context_,
                                       ECORE_IMF_CALLBACK_DELETE_SURROUNDING,
                                       &IMFDeleteSurroundingCallback);
  ecore_imf_context_event_callback_del(
      context_, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, &IMFPreeditChangedCallback);
  ecore_imf_context_event_callback_del(
      context_, ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
      &IMFTransactionPrivateCommandSendCallback);
#if defined(OS_TIZEN_TV_PRODUCT)
  ecore_imf_context_event_callback_del(context_,
                                       ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
                                       &IMFEventPrivateCommandCallback);
#endif

  ecore_imf_context_input_panel_event_callback_del(
      context_, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
      &IMFInputPanelStateChangedCallback);
  ecore_imf_context_input_panel_event_callback_del(
      context_, ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT,
      &IMFInputPanelGeometryChangedCallback);
  ecore_imf_context_input_panel_event_callback_del(
      context_, ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT,
      &IMFCandidatePanelStateChangedCallback);
  ecore_imf_context_input_panel_event_callback_del(
      context_, ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT,
      &IMFCandidatePanelGeometryChangedCallback);
  ecore_imf_context_input_panel_event_callback_del(
      context_, ECORE_IMF_INPUT_PANEL_LANGUAGE_EVENT,
      &IMFCandidatePanelLanguageChangedCallback);
  ecore_imf_context_del(context_);
}

void IMContextEfl::HandleKeyDownEvent(const Evas_Event_Key_Down* event,
                                      bool* was_filtered) {
  Ecore_IMF_Event im_event;
  ecore_imf_evas_event_key_down_wrap(const_cast<Evas_Event_Key_Down*>(event),
                                     &im_event.key_down);
  *was_filtered = ecore_imf_context_filter_event(
      context_, ECORE_IMF_EVENT_KEY_DOWN, &im_event);
  is_keyevent_processing_ = true;
}

void IMContextEfl::HandleKeyUpEvent(const Evas_Event_Key_Up* event,
                                    bool* was_filtered) {
  Ecore_IMF_Event im_event;
  ecore_imf_evas_event_key_up_wrap(const_cast<Evas_Event_Key_Up*>(event),
                                   &im_event.key_up);
  *was_filtered = ecore_imf_context_filter_event(
      context_, ECORE_IMF_EVENT_KEY_UP, &im_event);
}

void IMContextEfl::UpdateLayoutVariation(bool is_minimum_negative,
                                         bool is_step_integer) {
  // Dot should be available only when step attribute is not integer
  // Minus should be available only when min attribute is negative
  if (is_minimum_negative && !is_step_integer) {
    ecore_imf_context_input_panel_layout_variation_set(
        context_,
        ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED_AND_DECIMAL);
  } else if (is_minimum_negative) {
    ecore_imf_context_input_panel_layout_variation_set(
        context_, ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED);
  } else if (!is_step_integer) {
    ecore_imf_context_input_panel_layout_variation_set(
        context_, ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_DECIMAL);
  } else {
    ecore_imf_context_input_panel_layout_variation_set(
        context_, ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_NORMAL);
  }

  // Keyboard layout doesn't update after change
  HidePanel();
  ShowPanel();
}

void IMContextEfl::UpdateInputMethodType(ui::TextInputType type,
                                         ui::TextInputMode mode,
                                         bool can_compose_inline
#if defined(OS_TIZEN_TV_PRODUCT)
                                         ,
                                         int password_input_minlength
#endif
                                         ) {
  if (current_type_ == type && current_mode_ == mode &&
      can_compose_inline_ == can_compose_inline
#if defined(OS_TIZEN_TV_PRODUCT)
      && password_input_minlength_ == password_input_minlength
#endif
      ) {
    return;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  password_input_minlength_ = password_input_minlength;
#endif

  Ecore_IMF_Input_Panel_Layout layout;
  Ecore_IMF_Input_Panel_Return_Key_Type return_key_type =
      ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
  Ecore_IMF_Autocapital_Type cap_type = ECORE_IMF_AUTOCAPITAL_TYPE_NONE;
  bool allow_prediction = true;
  bool is_multi_line __attribute__((unused)) = false;
  bool disable_done_key = false;

  if (is_in_form_tag_)
    return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_GO;

  switch (type) {
    case ui::TEXT_INPUT_TYPE_TEXT:
      layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
      if (!is_in_form_tag_)
        return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE;
      break;
    case ui::TEXT_INPUT_TYPE_PASSWORD:
      layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD;
      if (!is_in_form_tag_)
        return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE;
      allow_prediction = false;
      break;
    case ui::TEXT_INPUT_TYPE_SEARCH:
      layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
      return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH;
      break;
    case ui::TEXT_INPUT_TYPE_EMAIL:
      layout = ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL;
      if (!is_in_form_tag_)
        return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE;
      break;
    case ui::TEXT_INPUT_TYPE_NUMBER:
      if (!is_in_form_tag_)
        return_key_type = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE;
      if (IsMobileProfile() || IsWearableProfile()) {
        layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY;
        view_->web_contents()->GetDelegate()->QueryNumberFieldAttributes();
      } else {
        layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBER;
      }
      break;
    case ui::TEXT_INPUT_TYPE_TELEPHONE:
      layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER;
      break;
    case ui::TEXT_INPUT_TYPE_URL:
      layout = ECORE_IMF_INPUT_PANEL_LAYOUT_URL;
      break;
    case ui::TEXT_INPUT_TYPE_MONTH:
      layout = ECORE_IMF_INPUT_PANEL_LAYOUT_MONTH;
      break;
    case ui::TEXT_INPUT_TYPE_TEXT_AREA:
    case ui::TEXT_INPUT_TYPE_CONTENT_EDITABLE:
      layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
      cap_type = ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE;
      is_multi_line = true;
      break;

    // No direct mapping to Ecore_IMF API, use simple text layout.
    case ui::TEXT_INPUT_TYPE_DATE:
    case ui::TEXT_INPUT_TYPE_DATE_TIME:
    case ui::TEXT_INPUT_TYPE_DATE_TIME_LOCAL:
    case ui::TEXT_INPUT_TYPE_TIME:
    case ui::TEXT_INPUT_TYPE_WEEK:
    case ui::TEXT_INPUT_TYPE_DATE_TIME_FIELD:
      layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
      break;

    case ui::TEXT_INPUT_TYPE_NONE:
      return;

    default:
      NOTREACHED();
      return;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  // 2017 WebBrowser App want no recommended list same as 2015, 2016.
  if (content::IsWebBrowser())
    allow_prediction = false;

  // Always enable "Up" and "Down" key
  // Set IMEUp=2, IMEDown=2 will always enable the 'up' and 'down' arrow key
  std::string im_data("IMEUp=2&IMEDown=2");

  if (allow_prediction && is_get_lookup_table_from_app_)
    im_data.append("&layouttype=1");

  ecore_imf_context_input_panel_imdata_set(context_, im_data.c_str(),
                                           im_data.length() + 1);

  // For password input type,disable "Done" key when inputed text not meet
  // minlength
  if (ui::TEXT_INPUT_TYPE_PASSWORD == type &&
      surrounding_text_length_ < password_input_minlength_) {
    LOG(INFO) << "surrounding_text_length_:" << surrounding_text_length_
              << ",password_input_minlength_:" << password_input_minlength_
              << ",disable done key";
    disable_done_key = true;
  }
#endif

  Ecore_IMF_Input_Mode imf_mode;
  switch (mode) {
    case ui::TEXT_INPUT_MODE_NUMERIC:
      imf_mode = ECORE_IMF_INPUT_MODE_NUMERIC;
      break;
    default:
      imf_mode = ECORE_IMF_INPUT_MODE_ALPHA;
  }

  ecore_imf_context_input_panel_layout_set(context_, layout);
  ecore_imf_context_input_mode_set(context_, imf_mode);
  ecore_imf_context_input_panel_return_key_type_set(context_, return_key_type);
  ecore_imf_context_autocapital_type_set(context_, cap_type);
  ecore_imf_context_prediction_allow_set(context_, allow_prediction);
  ecore_imf_context_input_panel_return_key_disabled_set(context_,
                                                        disable_done_key);

#if defined(OS_TIZEN)
  Ecore_IMF_Input_Hints hints;
  if (is_multi_line) {
    hints = static_cast<Ecore_IMF_Input_Hints>(
        ecore_imf_context_input_hint_get(context_) |
        ECORE_IMF_INPUT_HINT_MULTILINE);
  } else {
    hints = static_cast<Ecore_IMF_Input_Hints>(
        ecore_imf_context_input_hint_get(context_) &
        ~ECORE_IMF_INPUT_HINT_MULTILINE);
  }
  ecore_imf_context_input_hint_set(context_, hints);
#endif

  // If the focused element supports inline rendering of composition text,
  // we receive and send related events to it. Otherwise, the events related
  // to the updates of composition text are directed to the candidate window.
  ecore_imf_context_use_preedit_set(context_, can_compose_inline);

  current_mode_ = mode;
}

bool IMContextEfl::IsDefaultReturnKeyType() {
  return ecore_imf_context_input_panel_return_key_type_get(context_) ==
         ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
}

#if defined(OS_TIZEN_MOBILE_PRODUCT)
void IMContextEfl::PrevNextButtonUpdate(bool prev_state, bool next_state) {
  ecore_imf_context_event_callback_del(context_,
                                       ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
                                       &IMFEventPrivateCmdCallback);
  if (!prev_state && !next_state) {
    ecore_imf_context_input_panel_imdata_set(context_, "action=browser_navi_00",
                                             22);  // single edit field
  }
  if (!prev_state && next_state) {
    ecore_imf_context_input_panel_imdata_set(context_, "action=browser_navi_01",
                                             22);  // first position
  }
  if (!next_state && prev_state) {
    ecore_imf_context_input_panel_imdata_set(context_, "action=browser_navi_10",
                                             22);  // End Position
  }
  if (prev_state && next_state) {
    ecore_imf_context_input_panel_imdata_set(context_, "action=browser_navi_11",
                                             22);  // middle Position
  }

  ecore_imf_context_event_callback_add(context_,
                                       ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
                                       &IMFEventPrivateCmdCallback, this);
}
#endif

bool IMContextEfl::ShouldHandleImmediately(const char* key) {
  // Do not forward keyevent now if there is fake key event
  // handling at the moment to preserve orders of events as in Webkit.
  // Some keys should be processed after composition text is
  // released.
  // So some keys are pushed to KeyDownEventQueue first
  // then it will be porcessed after composition text is processed.
  if ((IsPreeditQueueEmpty() || IsKeyUpQueueEmpty()) &&
      !(!IsCommitQueueEmpty() &&
        (!strcmp(key, "Return") || (IsTvProfile() && !strcmp(key, "Select")) ||
         (IsWearableProfile() && !strcmp(key, "BackSpace")) ||
         !strcmp(key, "Down") || !strcmp(key, "Up") || !strcmp(key, "Right") ||
         !strcmp(key, "Left") || !strcmp(key, "space") ||
         !strcmp(key, "Tab")))) {
    return true;
  }
  return false;
}

void IMContextEfl::UpdateInputMethodState(ui::TextInputType type,
                                          bool can_compose_inline,
                                          bool show_if_needed
#if defined(OS_TIZEN_TV_PRODUCT)
                                          ,
                                          int password_input_minlength
#endif
                                          ) {
#if defined(OS_TIZEN_TV_PRODUCT)
  if (current_type_ != type || can_compose_inline_ != can_compose_inline ||
      password_input_minlength_ != password_input_minlength) {
    UpdateInputMethodType(type, ui::TEXT_INPUT_MODE_DEFAULT, can_compose_inline,
                          password_input_minlength);
#else
  if (current_type_ != type || can_compose_inline_ != can_compose_inline) {
    UpdateInputMethodType(type, ui::TEXT_INPUT_MODE_DEFAULT,
                          can_compose_inline);
#endif
    // Workaround on platform issue:
    // http://107.108.218.239/bugzilla/show_bug.cgi?id=11494
    // Keyboard layout doesn't update after change.
    if (IsVisible() && type != ui::TEXT_INPUT_TYPE_NONE)
      HidePanel();
  }

  current_type_ = type;
  can_compose_inline_ = can_compose_inline;

  bool focus_in = type != ui::TEXT_INPUT_TYPE_NONE;
  if (focus_in == is_focused_ && (!show_if_needed || IsVisible()))
    return;

  if (focus_in && show_if_needed)
    ShowPanel();
  else if (focus_in)
    OnFocusIn();
  else if (IsVisible())
    HidePanel();
  else
    OnFocusOut();
}

void IMContextEfl::ShowPanel() {
  LOG(INFO) << "Show Input Panel!";
  is_showing_ = true;
  is_focused_ = true;
  view_->web_contents()->GetDelegate()->WillImePanelShow();

  ecore_imf_context_focus_in(context_);
  ecore_imf_context_input_panel_show(context_);
}

void IMContextEfl::HidePanel() {
  LOG(INFO) << "Hide Input Panel!";
  is_showing_ = false;
  is_focused_ = false;
  ecore_imf_context_focus_out(context_);
  ecore_imf_context_input_panel_hide(context_);
}

void IMContextEfl::UpdateCaretBounds(const gfx::Rect& caret_bounds) {
  if (IsVisible()) {
    int x = caret_bounds.x();
    int y = caret_bounds.y();
    int w = caret_bounds.width();
    int h = caret_bounds.height();
    ecore_imf_context_cursor_location_set(context_, x, y, w, h);
  }
}

void IMContextEfl::OnFocusIn() {
  if (current_type_ == ui::TEXT_INPUT_TYPE_NONE)
    return;

  ecore_imf_context_focus_in(context_);
  is_focused_ = true;
}

void IMContextEfl::OnFocusOut() {
  is_focused_ = false;
  CancelComposition();
  ecore_imf_context_focus_out(context_);
}

void IMContextEfl::OnResume() {
  if (should_show_on_resume_) {
    should_show_on_resume_ = false;
    ShowPanel();
  }
}

void IMContextEfl::OnSuspend() {
  if (view_->HasFocus() && IsVisible())
    should_show_on_resume_ = true;
}

void IMContextEfl::DoNotShowAfterResume() {
  should_show_on_resume_ = false;
}

void IMContextEfl::ResetIMFContext() {
  is_ime_ctx_reset_ = true;
  ecore_imf_context_reset(context_);
  is_ime_ctx_reset_ = false;
}

void IMContextEfl::CancelComposition() {
  ClearQueues();
  ResetIMFContext();

  if (composition_.text.length() > 0) {
    base::string16 empty;
    view_->ConfirmComposition(empty);
    composition_.text = base::UTF8ToUTF16("");
  }
}

void IMContextEfl::ConfirmComposition() {
  // Gtk use it to send the empty string as committed.
  // I'm not sure we need it (kbalazs).
}

void IMContextEfl::SetSurroundingText(std::string value) {
  is_surrounding_text_change_in_progress_ = false;
  surrounding_text_ = value;

#if defined(OS_TIZEN_TV_PRODUCT)
  // For password input type,disable "Done" key when inputed text not meet
  // minlength
  surrounding_text_length_ = base::UTF8ToUTF16(value).length();

  if (ui::TEXT_INPUT_TYPE_PASSWORD == current_type_ &&
      password_input_minlength_ != -1) {
    if (surrounding_text_length_ < password_input_minlength_) {
      ecore_imf_context_input_panel_return_key_disabled_set(context_, true);
    } else {
      ecore_imf_context_input_panel_return_key_disabled_set(context_, false);
    }
  }
#endif
}

void IMContextEfl::SetIsInFormTag(bool is_in_form_tag) {
  is_in_form_tag_ = is_in_form_tag;
  // TODO: workaround on tizen v3.0 platform issue
  // Even if virtual keyboard is shown,
  // the API 'ecore_imf_context_input_panel_state_get()'
  // returns '1' which means the keyboard is not shown.
  // It makes the 'HidePanel()' unreachable.
  if (!IsVisible())
    return;

  if (ecore_imf_context_input_panel_return_key_type_get(context_) ==
      ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH)
    return;

  if (is_in_form_tag_)
    ecore_imf_context_input_panel_return_key_type_set(
        context_, ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_GO);
  else {
    if (current_type_ == ui::TEXT_INPUT_TYPE_TEXT ||
        current_type_ == ui::TEXT_INPUT_TYPE_NUMBER ||
        current_type_ == ui::TEXT_INPUT_TYPE_PASSWORD ||
        current_type_ == ui::TEXT_INPUT_TYPE_EMAIL)
      ecore_imf_context_input_panel_return_key_type_set(
          context_, ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
    else
      ecore_imf_context_input_panel_return_key_type_set(
          context_, ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT);
  }
}

void IMContextEfl::SetCaretPosition(int position) {
  if (context_) {
    caret_position_ = position;

    if (!is_keyevent_processing_)
      ecore_imf_context_cursor_position_set(context_, position);
  }
}

void IMContextEfl::OnCommit(void* event_info) {
  if (is_ime_ctx_reset_) {
    return;
  }

  composition_ = ui::CompositionText();

  char* text = static_cast<char*>(event_info);

  base::string16 text16 = base::UTF8ToUTF16(text);
  // Only add commit to queue, till we dont know if key event
  // should be handled. It can be default prevented for exactly.
  commit_queue_.push(text16);

  is_surrounding_text_change_in_progress_ = true;

  // sending fake key event if hardware key is not handled as it is
  // in Webkit.
  SendFakeCompositionKeyEvent(text16);
}

void IMContextEfl::SendFakeCompositionKeyEvent(const base::string16& buf) {

  std::string str = base::UTF16ToUTF8(buf);

  Evas_Event_Key_Down downEvent;
  memset(&downEvent, 0, sizeof(Evas_Event_Key_Down));
  downEvent.key = str.c_str();
  downEvent.string = str.c_str();

  NativeWebKeyboardEvent n_event = MakeWebKeyboardEvent(true, &downEvent);
  n_event.is_system_key = true;

  // Key code should be '229' except ASCII key event about key down/up event.
  // It is according to Web spec about key code [1].
  // On TV WebAPPs case, key code should be '229' including ASCII key event
  // about key down/up event.
  // [1] https://lists.w3.org/Archives/Public/www-dom/2010JulSep/att-0182/
  // keyCode-spec.html
  // [2] http://developer.samsung.com/tv/develop/tutorials/user-input/
  // text-input-ime-external-keyboard
  if (n_event.windows_key_code == 0)
    n_event.windows_key_code = 229;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (content::IsTIZENWRT()) {
    // a-z, A-Z, 0-9
    if ((n_event.windows_key_code >= 65 && n_event.windows_key_code <= 90) ||
        (n_event.windows_key_code >= 48 && n_event.windows_key_code <= 57))
      n_event.windows_key_code = 229;
  }
#endif

  if (!view_->GetRenderWidgetHost())
    return;

  is_keyevent_processing_ = true;
  PushToKeyUpEventQueue(n_event.windows_key_code);
  view_->GetRenderWidgetHost()->ForwardKeyboardEvent(n_event);
}

void IMContextEfl::SetComposition(const char* buffer) {
  base::string16 text16 = base::UTF8ToUTF16(buffer);
  if (!text16.empty())
    SendFakeCompositionKeyEvent(text16.substr(text16.length() - 1));
  else
    SendFakeCompositionKeyEvent(text16);

  composition_ = ui::CompositionText();
  composition_.text = text16;

  composition_.ime_text_spans.push_back(
      ui::ImeTextSpan(0, composition_.text.length(), SK_ColorBLACK, false));

  composition_.selection = gfx::Range(composition_.text.length());

  is_surrounding_text_change_in_progress_ = true;

  // Only add preedit to queue, till we dont know if key event
  // should be handled. It can be default prevented for exactly.
  preedit_queue_.push(composition_);
}

void IMContextEfl::OnPreeditChanged(void* data,
                                    Ecore_IMF_Context* context,
                                    void* event_info) {
  if (is_ime_ctx_reset_)
    return;

  char* buffer = NULL;
  ecore_imf_context_preedit_string_get(context, &buffer, 0);

  if (!buffer) {
    return;
  }

  // 'buffer' is a null teminated utf-8 string,
  // it should be OK to find '\n' via strchr
  char* new_line = std::strchr(buffer, '\n');

  // SetComposition does not handle '\n' properly for
  // 1. Single line input(<input>)
  // 2. Multiple line input(<textarea>), when not in composition state
  if (new_line) {
    if (current_type_ != ui::TEXT_INPUT_TYPE_TEXT_AREA &&
        current_type_ != ui::TEXT_INPUT_TYPE_CONTENT_EDITABLE) {
      // 1. Single line case, replace '\n' to ' '
      do {
        *new_line = ' ';
        new_line = std::strchr(new_line + 1, '\n');
      } while (new_line != NULL);
    } else if (composition_.text.length() == 0) {
      // 2. Multiple line case, split to 2 SetComposition:
      // a. The first line text(without '\n'),
      // b. All of the text
      if (new_line == buffer) {
        // If first line starts with '\n', send a space instead
        SetComposition(" ");
      } else {
        // Send the first line
        *new_line = '\0';
        SetComposition(buffer);
        *new_line = '\n';
      }
    }
  }
  SetComposition(buffer);
  free(buffer);
}

// TODO(kbalazs): figure out what do we need from these callbacks.
// Tizen-WebKit-efl using all of them.

void IMContextEfl::OnInputPanelStateChanged(int state) {
  if (state == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
    if (view_->ewk_view()) {
      is_showing_ = true;
      evas_object_smart_callback_call(view_->ewk_view(),
                                      "editorclient,ime,opened", 0);
    }
  } else if (state == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
    // Hide clipboard window on state ECORE_IMF_INPUT_PANEL_STATE_HIDE in
    // ECORE_IMF_INPUT_PANEL_STATE_EVENT callback to keep clipboard behavior
    // compatible with Tizen 2.4.
    ClipboardHelperEfl::GetInstance()->CloseClipboardWindow();
    if (view_->ewk_view()) {
      is_showing_ = false;
      evas_object_smart_callback_call(view_->ewk_view(),
                                      "editorclient,ime,closed", 0);
    }
  }

  // ignore ECORE_IMF_INPUT_PANEL_WILL_SHOW value
}

void IMContextEfl::OnInputPanelGeometryChanged() {
  Eina_Rectangle rect;
  ecore_imf_context_input_panel_geometry_get(context_, &rect.x, &rect.y,
                                             &rect.w, &rect.h);
  if (view_->ewk_view())
    evas_object_smart_callback_call(view_->ewk_view(), "inputmethod,changed",
                                    static_cast<void*>(&rect));
}

void IMContextEfl::OnCandidateInputPanelStateChanged(int state) {
  if (view_->ewk_view()) {
    if (state == ECORE_IMF_CANDIDATE_PANEL_SHOW) {
      evas_object_smart_callback_call(view_->ewk_view(),
                                      "editorclient,candidate,opened", 0);
    } else {
      evas_object_smart_callback_call(view_->ewk_view(),
                                      "editorclient,candidate,closed", 0);
    }
  }
}

void IMContextEfl::OnCandidateInputPanelGeometryChanged() {}

bool IMContextEfl::OnRetrieveSurrounding(char** text, int* offset) {
  // |text| is for providing the value of input field and
  // |offset| is for providing cursor position,
  // when surrounding text & cursor position is requested from otherside.
  if (!text && !offset)
    return false;

  // If surrounding text is requested before the process of text is completed,
  // return false to ensure the value(surrounding text & cursor position).
  if (is_surrounding_text_change_in_progress_)
    return false;

  if (text)
    *text = strdup(surrounding_text_.c_str());

  if (offset)
    *offset = caret_position_;

  return true;
}

void IMContextEfl::OnDeleteSurrounding(void* event_info) {
  auto rfh = static_cast<content::RenderFrameHostImpl*>(
      view_->web_contents()->GetFocusedFrame());

  if (!rfh)
    return;

  is_surrounding_text_change_in_progress_ = true;

  CancelComposition();

  auto event = static_cast<Ecore_IMF_Event_Delete_Surrounding*>(event_info);

  int before = -(event->offset);
  int after = event->n_chars + event->offset;

  if (is_transaction_finished_) {
    rfh->GetFrameInputHandler()->ExtendSelectionAndDelete(before, after);
  } else {
    int begin = event->offset < 0 ? 0 : event->offset;
    int end = event->n_chars - begin;
    std::vector<blink::WebImeTextSpan> underlines;
    underlines.push_back(blink::WebImeTextSpan());
    // begin, end, SK_ColorBLACK, false));

    rfh->Send(new InputMsg_SetCompositionFromExistingText(
        rfh->GetRoutingID(), begin, end, underlines));
  }
}

void IMContextEfl::OnCandidateInputPanelLanguageChanged(Ecore_IMF_Context*) {
  RenderWidgetHostImpl* rwhi = GetRenderWidgetHostImpl();

  if (!rwhi || composition_.text.length() == 0)
    return;

  CancelComposition();
}

#if defined(OS_TIZEN_MOBILE_PRODUCT)
void IMContextEfl::OnIMFPrevNextButtonPressedCallback(std::string command) {
  ecore_imf_context_event_callback_del(context_,
                                       ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
                                       &IMFEventPrivateCmdCallback);
  auto render_frame_host = static_cast<content::RenderFrameHostImpl*>(
      view_->web_contents()->GetFocusedFrame());
  if (!render_frame_host)
    return;
  bool direction = false;
  if (!strcmp(command.c_str(), "MoveFocusNext"))
    direction = true;
  render_frame_host->Send(new EwkFrameMsg_MoveToNextOrPreviousSelectElement(
      render_frame_host->GetRoutingID(), direction));
}
#endif

bool IMContextEfl::IsVisible() {
  if (!context_)
    return false;

  // API 'ecore_imf_context_input_panel_state_get()' seems
  // to be buggy(ISF guys said it's an ASYNC api).
  // Even if virtual keyboard is shown,
  // the API 'ecore_imf_context_input_panel_state_get()' might
  // returns '1' which means the keyboard is not shown.
  return is_showing_;
}

bool IMContextEfl::WebViewWillBeResized() {
  if (!context_)
    return false;

  if (IsVisible())
    return default_view_size_ == view_->GetPhysicalBackingSize();

  return default_view_size_ != view_->GetPhysicalBackingSize();
}

void IMContextEfl::ClearQueues() {
  commit_queue_ = CommitQueue();
  preedit_queue_ = PreeditQueue();
  // ClearQueues only called to reset composition state.
  // To make sure keydown and keyup events are paired,
  // keydown & keyup event queues should not be cleared.
}

void IMContextEfl::IMFCommitCallback(void* data,
                                     Ecore_IMF_Context*,
                                     void* event_info) {
  static_cast<IMContextEfl*>(data)->OnCommit(event_info);
}

void IMContextEfl::IMFPreeditChangedCallback(void* data,
                                             Ecore_IMF_Context* context,
                                             void* event_info) {
  static_cast<IMContextEfl*>(data)->OnPreeditChanged(data, context, event_info);
}

void IMContextEfl::IMFInputPanelStateChangedCallback(void* data,
                                                     Ecore_IMF_Context*,
                                                     int state) {
  static_cast<IMContextEfl*>(data)->OnInputPanelStateChanged(state);
}

#if defined(OS_TIZEN_MOBILE_PRODUCT)
void IMContextEfl::IMFEventPrivateCmdCallback(void* data,
                                              Ecore_IMF_Context* context,
                                              void* eventInfo) {
  if (!data || !eventInfo)
    return;
  static_cast<IMContextEfl*>(data)->OnIMFPrevNextButtonPressedCallback(
      static_cast<char*>(eventInfo));
}
#endif

void IMContextEfl::IMFInputPanelGeometryChangedCallback(void* data,
                                                        Ecore_IMF_Context*,
                                                        int state) {
  static_cast<IMContextEfl*>(data)->OnInputPanelGeometryChanged();
}

void IMContextEfl::IMFCandidatePanelStateChangedCallback(void* data,
                                                         Ecore_IMF_Context*,
                                                         int state) {
  static_cast<IMContextEfl*>(data)->OnCandidateInputPanelStateChanged(state);
}

void IMContextEfl::IMFCandidatePanelGeometryChangedCallback(void* data,
                                                            Ecore_IMF_Context*,
                                                            int state) {
  static_cast<IMContextEfl*>(data)->OnCandidateInputPanelGeometryChanged();
}

Eina_Bool IMContextEfl::IMFRetrieveSurroundingCallback(void* data,
                                                       Ecore_IMF_Context*,
                                                       char** text,
                                                       int* offset) {
  return static_cast<IMContextEfl*>(data)->OnRetrieveSurrounding(text, offset);
}

void IMContextEfl::IMFDeleteSurroundingCallback(void* data,
                                                Ecore_IMF_Context*,
                                                void* event_info) {
  static_cast<IMContextEfl*>(data)->OnDeleteSurrounding(event_info);
}

void IMContextEfl::IMFCandidatePanelLanguageChangedCallback(
    void* data,
    Ecore_IMF_Context* context,
    int value) {
  static_cast<IMContextEfl*>(data)->OnCandidateInputPanelLanguageChanged(
      context);
}

#if defined(OS_TIZEN_TV_PRODUCT)
void IMContextEfl::OnIMFEventPrivateCommand(std::string command) {
  // When user leaves the IME from the top, we get IME_F31.
  // In IME 'autoword' mode, when an voice input accomplished,
  // IME send out IME_F29 event to notify app to hide IME panel
  if ((content::IsWebBrowser() && command == "IME_F31") ||
      (content::IsTIZENWRT() && command == "IME_F29"))
    HidePanel();
}

void IMContextEfl::IMFEventPrivateCommandCallback(void* data,
                                                  Ecore_IMF_Context* context,
                                                  void* eventInfo) {
  if (!data || !eventInfo)
    return;

  static_cast<IMContextEfl*>(data)->OnIMFEventPrivateCommand(
      static_cast<char*>(eventInfo));
}
#endif

// static
void IMContextEfl::IMFTransactionPrivateCommandSendCallback(
    void* data,
    Ecore_IMF_Context* context,
    void* eventInfo) {
  if (!data || !eventInfo)
    return;

  char* cmd = static_cast<char*>(eventInfo);
  auto imce = static_cast<IMContextEfl*>(data);
  if (!strcmp(cmd, "TRANSACTION_START")) {
    imce->is_transaction_finished_ = false;
    imce->composition_ = ui::CompositionText();
  } else if (!strcmp(cmd, "TRANSACTION_END")) {
    imce->is_transaction_finished_ = true;
  }
}

void IMContextEfl::HandleKeyEvent(bool processed,
                                  blink::WebInputEvent::Type type) {
  if (type == blink::WebInputEvent::kKeyDown) {
    ProcessNextCommitText(processed);
    ProcessNextPreeditText(processed);
    ProcessNextKeyUpEvent();
    ProcessNextKeyDownEvent();
  } else if (type == blink::WebInputEvent::kKeyUp) {
    is_keyevent_processing_ = false;
    ecore_imf_context_cursor_position_set(context_, caret_position_);
  }
}

void IMContextEfl::ProcessNextCommitText(bool processed) {
  while (!commit_queue_.empty()) {
    if (!processed && !(commit_queue_.front().empty()))
      view_->ConfirmComposition(commit_queue_.front());

    commit_queue_.pop();
  }
}

void IMContextEfl::ProcessNextPreeditText(bool processed) {
  if (preedit_queue_.empty())
    return;

  if (!processed)
    view_->SetComposition(preedit_queue_.front());
  else
    ResetIMFContext();

  preedit_queue_.pop();
}

void IMContextEfl::ProcessNextKeyDownEvent() {
  if (keydown_event_queue_.empty())
    return;

  GetRenderWidgetHostImpl()->ForwardKeyboardEvent(keydown_event_queue_.front());
  keydown_event_queue_.pop();
}

void IMContextEfl::ProcessNextKeyUpEvent() {
  if (keyup_event_queue_.empty())
    return;

  SendCompositionKeyUpEvent(keyup_event_queue_.front());
  keyup_event_queue_.pop();
}

void IMContextEfl::SendCompositionKeyUpEvent(int code) {
  blink::WebKeyboardEvent keyboard_event(
      blink::WebInputEvent::kKeyUp, blink::WebInputEvent::kNoModifiers, 0.0);
  keyboard_event.windows_key_code = code;

  GetRenderWidgetHostImpl()->ForwardKeyboardEvent(
      NativeWebKeyboardEvent(keyboard_event, view_->GetNativeView()));
}

void IMContextEfl::PushToKeyUpEventQueue(int key) {
  keyup_event_queue_.push(key);
}

void IMContextEfl::PushToKeyDownEventQueue(NativeWebKeyboardEvent key) {
  keydown_event_queue_.push(key);
}

RenderWidgetHostImpl* IMContextEfl::GetRenderWidgetHostImpl() const {
  RenderWidgetHost* rwh = view_->GetRenderWidgetHost();
  if (!rwh)
    return NULL;
  return RenderWidgetHostImpl::From(rwh);
}

void IMContextEfl::SetIMERecommendedWords(const std::string& im_data) {
  ecore_imf_context_input_panel_imdata_set(context_, im_data.c_str(),
                                           im_data.length() + 1);
}

void IMContextEfl::SetIMERecommendedWordsType(bool should_enable) {
  // currently IME only support enable setting recommended words
  // and not support disable it.
  // If support later, fix it.
  is_get_lookup_table_from_app_ = should_enable;

  if (should_enable) {
    std::string im_data("layouttype=1");
    ecore_imf_context_input_panel_imdata_set(context_, im_data.c_str(),
                                             im_data.length() + 1);
  }
}

}  // namespace content
