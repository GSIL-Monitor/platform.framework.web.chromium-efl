// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include "autofill_popup_view_efl.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "content/common/paths_efl.h"
#include "eweb_view.h"
#include "tizen/system_info.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect_f.h"

#define AUTOFILL_POPUP_LABEL_COUNT  6 // Autofill component send 6 at max
#define AUTOFILL_POPUP_LABEL_LEN    100

using namespace password_manager;

namespace autofill {
namespace {
  // const values taken from webkit2-efl
  constexpr int bg_color_red = 200, bg_color_green = 200, bg_color_blue = 200;
  constexpr int border_size = 3;
  std::vector<std::string> labels_(AUTOFILL_POPUP_LABEL_COUNT);
} // namespace

enum AutofillSavePassword {
  AUTOFILL_SAVE_PASS_NEVER  = 0,
  AUTOFILL_SAVE_PASS_YES    = 1,
  AUTOFILL_SAVE_PASS_NOTNOW = 2,
};

AutofillPopupViewEfl::AutofillPopupViewEfl(EWebView* view)
    : webview_(view),
      autofill_popup_(nullptr),
      autofill_list_(nullptr),
      password_popup_(nullptr),
      selected_line_(-1) {
  auto native_view =
      static_cast<Evas_Object*>(webview_->web_contents().GetNativeView());

  autofill_popup_ = elm_layout_add(native_view);
  if (!autofill_popup_)
    return;

  auto smart_parent = evas_object_smart_parent_get(native_view);
  if (!smart_parent) {
    LOG(ERROR) << "Unable to get smart parent from native view";
    evas_object_del(autofill_popup_);
    autofill_popup_ = nullptr;
    return;
  }
  evas_object_smart_member_add(autofill_popup_, smart_parent);
  base::FilePath edj_dir;
  base::FilePath autofill_edj;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  autofill_edj = edj_dir.Append(FILE_PATH_LITERAL("AutofillPopup.edj"));
  elm_layout_file_set(autofill_popup_,
      autofill_edj.AsUTF8Unsafe().c_str(),
      "formdata_list");
  autofill_list_ = elm_genlist_add(autofill_popup_);
}

AutofillPopupViewEfl::~AutofillPopupViewEfl()
{
  if (autofill_popup_) {
    evas_object_smart_member_del(autofill_popup_);
    evas_object_del(autofill_popup_);
  }
  if (password_popup_)
    evas_object_del(password_popup_);
}

void AutofillPopupViewEfl::Show()
{
  if (autofill_popup_)
    evas_object_show(autofill_popup_);
  if (delegate_)
    delegate_->OnPopupShown();
}

void AutofillPopupViewEfl::Hide()
{
  if (autofill_popup_)
    evas_object_hide(autofill_popup_);
  if (delegate_)
    delegate_->OnPopupHidden();
}

void AutofillPopupViewEfl::ShowSavePasswordPopup(
    std::unique_ptr<PasswordFormManager> form_to_save) {
  if (password_popup_) {
    evas_object_del(password_popup_);
    password_popup_ = NULL;
  }
  form_manager_ = std::move(form_to_save);
  password_popup_ = elm_popup_add(webview_->evas_object());
  elm_popup_content_text_wrap_type_set(password_popup_, ELM_WRAP_CHAR);
  elm_object_domain_translatable_part_text_set(
      password_popup_, "title,text", "WebKit",
      "IDS_WEBVIEW_HEADER_SAVE_SIGN_IN_INFO");

#if defined(USE_EFL) && !defined(OS_TIZEN)
  // Fix the positioning of the password popup on desktop build
  auto rwhv = webview_->web_contents().GetRenderWidgetHostView();
  if (rwhv) {
    int w = 0, h = 0;
    evas_object_geometry_get(password_popup_, 0, 0, &w, &h);
    gfx::Size size = rwhv->GetVisibleViewportSize();
    evas_object_move(password_popup_, (size.width() - w) / 2,
                     (size.height() - h) / 2);
  }
#endif

  evas_object_show(password_popup_);

  Evas_Object* btn_never = elm_button_add(password_popup_);
  elm_object_domain_translatable_part_text_set(btn_never, NULL, "WebKit",
                                               "IDS_WEBVIEW_BUTTON2_NEVER");
  elm_object_part_content_set(password_popup_, "button1", btn_never);
  evas_object_smart_callback_add(btn_never,
      "clicked",
      savePasswordNeverCb,
      this);

  Evas_Object* btn_not_now = elm_button_add(password_popup_);
  elm_object_domain_translatable_part_text_set(btn_not_now, NULL, "WebKit",
                                               "IDS_WEBVIEW_BUTTON_LATER_ABB");
  elm_object_part_content_set(password_popup_, "button2", btn_not_now);
  evas_object_smart_callback_add(btn_not_now,
      "clicked",
      savePasswordNotNowCb,
      this);

  Evas_Object* btn_yes = elm_button_add(password_popup_);
  elm_object_domain_translatable_part_text_set(btn_yes, NULL, "WebKit",
                                               "IDS_WEBVIEW_BUTTON_SAVE");
  elm_object_part_content_set(password_popup_, "button3", btn_yes);
  evas_object_smart_callback_add(btn_yes,
      "clicked",
      savePasswordYesCb,
      this);
}

void AutofillPopupViewEfl::UpdateFormDataPopup(const gfx::RectF& bounds) {
  if (bounds.IsEmpty())
    return;

  Elm_Genlist_Item_Class* list_Items = NULL;
  double scale_factor = 1.0;
  if (!autofill_list_)
    return;
  Evas_Object* border_up = elm_bg_add(autofill_popup_);
  Evas_Object* border_down = elm_bg_add(autofill_popup_);
  Evas_Object* border_left = elm_bg_add(autofill_popup_);
  Evas_Object* border_right = elm_bg_add(autofill_popup_);

  elm_genlist_clear(autofill_list_);
  list_Items = elm_genlist_item_class_new();
  list_Items->item_style = "default";
  list_Items->func.text_get = getItemLabel;
  list_Items->func.content_get = NULL;
  list_Items->func.state_get = NULL;
  list_Items->func.del = NULL;
  for (size_t i = 0; i < values_.size(); ++i) {
    elm_genlist_item_append(autofill_list_,
        list_Items,
        reinterpret_cast<void*>(static_cast<long>(i)),
        NULL,
        ELM_GENLIST_ITEM_NONE,
        itemSelectCb,
        static_cast<void*>(this));
  }
  if (IsMobileProfile() || IsWearableProfile()) {
    scale_factor =
        display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  } else if (IsTvProfile()) {
    scale_factor = webview_->GetScale();
  }
#ifdef OS_TIZEN
  elm_object_scale_set(autofill_list_, webview_->GetScale() / 2);
#else
  elm_object_scale_set(autofill_list_, scale_factor);
#endif
  evas_object_show(autofill_list_);
  int list_height = bounds.height() * scale_factor;
  // Show at max 3 item for mobile device
  if (IsMobileProfile() || IsWearableProfile()) {
    if (values_.size() > 3) {
      list_height = 3 * list_height;
    } else {
      list_height = values_.size() * list_height;
    }
  } else {
    list_height = values_.size() * list_height;
  }

  // const values are taken from webkit2-efl
  elm_bg_color_set(border_up, bg_color_red, bg_color_green, bg_color_blue);
  evas_object_size_hint_min_set(border_up, bounds.width(), border_size);
  evas_object_show(border_up);

  elm_bg_color_set(border_down, bg_color_red, bg_color_green, bg_color_blue);
  evas_object_size_hint_min_set(border_down, bounds.width(), border_size);
  evas_object_show(border_down);

  elm_bg_color_set(border_left, bg_color_red, bg_color_green, bg_color_blue);
  evas_object_size_hint_min_set(border_left, border_size, list_height);
  evas_object_show(border_left);

  elm_bg_color_set(border_right, bg_color_red, bg_color_green, bg_color_blue);
  evas_object_size_hint_min_set(border_right, border_size, list_height);
  evas_object_show(border_right);

  elm_object_part_content_set(autofill_popup_, "list_container", autofill_list_);
  elm_object_part_content_set(autofill_popup_, "border_up", border_up);
  elm_object_part_content_set(autofill_popup_, "border_down", border_down);
  elm_object_part_content_set(autofill_popup_, "border_left", border_left);
  elm_object_part_content_set(autofill_popup_, "border_right", border_right);

  evas_object_size_hint_min_set(autofill_popup_, bounds.width(), list_height);
  evas_object_move(autofill_popup_, bounds.x() * scale_factor, (bounds.y() + bounds.height()) * scale_factor);
  evas_object_resize(autofill_popup_, bounds.width() * scale_factor, list_height);
  evas_object_propagate_events_set(autofill_popup_, false);
}

void AutofillPopupViewEfl::UpdateLocation(const gfx::RectF& bounds) {
  if (bounds.IsEmpty())
    return;

  double scale_factor =
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();

  evas_object_move(autofill_popup_, bounds.x() * scale_factor,
                   (bounds.y() + bounds.height()) * scale_factor);
}

bool isAutofillSpecial(autofill::Suggestion suggestion) {
  // Autofill data values added in the setting have frontend_id bigger than 0.
  // This values should be shown in autofill popup,
  // Add checking statement |suggestion.frontend_id <= 0|.
  return suggestion.frontend_id <= 0 &&
         suggestion.frontend_id != POPUP_ITEM_ID_AUTOCOMPLETE_ENTRY &&
         suggestion.frontend_id != POPUP_ITEM_ID_DATALIST_ENTRY;
}

void AutofillPopupViewEfl::InitFormData(
          const std::vector<autofill::Suggestion>& suggestions,
          base::WeakPtr<AutofillPopupDelegate> delegate){
  values_ = suggestions;
  // m42 adds some "special" autofill sugesstions.
  // This suggestions, like list separator is not useful for us
  // so, let's delete them.
  values_.erase(
    std::remove_if(values_.begin(), values_.end(), isAutofillSpecial),
    values_.end());

  for (size_t i = 0; i < values_.size() && i < AUTOFILL_POPUP_LABEL_COUNT;
       ++i) {
    labels_[i] = base::UTF16ToUTF8(values_[i].value);
    if (!values_[i].label.empty())
      labels_[i].append(" (" + base::UTF16ToUTF8(values_[i].label) + ")");
    // To keep compatibility with webkit2-efl
    if (AUTOFILL_POPUP_LABEL_LEN < labels_[i].length())
      labels_[i].resize(AUTOFILL_POPUP_LABEL_LEN);
  }
  delegate_ = delegate;
}

void AutofillPopupViewEfl::AcceptSuggestion(size_t index)
{
  if (delegate_) {
    if (index < values_.size())
      delegate_->DidAcceptSuggestion(values_[index].value, values_[index].frontend_id, index);
  }
}

void AutofillPopupViewEfl::AcceptPasswordSuggestion(int option)
{
  if (password_popup_) {
    evas_object_del(password_popup_);
    password_popup_ = NULL;
  }
  if (!form_manager_)
    return;
  switch (static_cast<AutofillSavePassword>(option)) {
    case AUTOFILL_SAVE_PASS_NEVER: {
      form_manager_->PermanentlyBlacklist();
      break;
    }
    case AUTOFILL_SAVE_PASS_YES: {
      form_manager_->Save();
      break;
    }
    case AUTOFILL_SAVE_PASS_NOTNOW:
    default: {
      break;
    }
  }
}

void AutofillPopupViewEfl::SetSelectedLine(size_t selected_line)
{
  if (selected_line_ == selected_line)
    return;
  selected_line_ = selected_line;
  if (delegate_) {
    if (selected_line_ < values_.size()) {
      delegate_->DidSelectSuggestion(values_[selected_line].value,
          selected_line);
    }
    else {
      delegate_->ClearPreviewedForm();
    }
  }
}

// Static
char* AutofillPopupViewEfl::getItemLabel(void* data, Evas_Object* obj, const char* part)
{
  size_t index = (size_t)data;
  if (AUTOFILL_POPUP_LABEL_COUNT > index)
    return strdup(labels_[index].c_str());
  else
    return strdup("");
}

void AutofillPopupViewEfl::itemSelectCb(void* data, Evas_Object* obj, void* event_info)
{
  size_t index = (size_t)elm_object_item_data_get(static_cast<Elm_Object_Item*>(event_info));
  AutofillPopupViewEfl* autofill_popup = static_cast<AutofillPopupViewEfl*>(data);
  if (autofill_popup)
    autofill_popup->AcceptSuggestion(index);
}

void AutofillPopupViewEfl::savePasswordNeverCb(void *data, Evas_Object *obj, void *event_info)
{
  AutofillPopupViewEfl* autofill_popup = static_cast<AutofillPopupViewEfl*>(data);
  if (autofill_popup)
    autofill_popup->AcceptPasswordSuggestion(AUTOFILL_SAVE_PASS_NEVER);
}

void AutofillPopupViewEfl::savePasswordYesCb(void *data, Evas_Object *obj, void *event_info)
{
  AutofillPopupViewEfl* autofill_popup = static_cast<AutofillPopupViewEfl*>(data);
  if (autofill_popup)
    autofill_popup->AcceptPasswordSuggestion(AUTOFILL_SAVE_PASS_YES);
}

void AutofillPopupViewEfl::savePasswordNotNowCb(void *data, Evas_Object *obj, void *event_info)
{
  AutofillPopupViewEfl* autofill_popup = static_cast<AutofillPopupViewEfl*>(data);
  if (autofill_popup)
    autofill_popup->AcceptPasswordSuggestion(AUTOFILL_SAVE_PASS_NOTNOW);
}

} // namespace autofill

#endif // TIZEN_AUTOFILL_SUPPORT
