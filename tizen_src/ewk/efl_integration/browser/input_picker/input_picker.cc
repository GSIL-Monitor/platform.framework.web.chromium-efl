// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "input_picker.h"

#include <Elementary.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "common/web_contents_utils.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/web_contents.h"

#if defined(OS_TIZEN)
#include <vconf/vconf.h>
#endif

namespace content {

static const char* kDefaultDatetimeFormat = "%Y/%m/%d %H:%M";

InputPicker::Layout::Layout(InputPicker* parent)
    : parent_(parent),
      conformant_(nullptr),
      popup_(nullptr),
      layout_(nullptr),
      set_button_(nullptr),
      cancel_button_(nullptr),
      color_picker_(nullptr),
      color_rect_(nullptr),
      date_picker_(nullptr),
      time_picker_(nullptr),
#if defined(OS_TIZEN) && !defined(OS_TIZEN_TV_PRODUCT)
      circle_surface_(nullptr),
#endif
      input_type_(ui::TEXT_INPUT_TYPE_NONE),
      is_color_picker_(false),
      red_(0),
      green_(0),
      blue_(0) {
  evas_object_focus_set(parent_->web_view_->evas_object(), false);
  // FIXME: Workaround. OSP requirement. OSP want to block own touch event
  // while webkit internal picker is running.
  evas_object_smart_callback_call(
      parent_->web_view_->evas_object(), "input,picker,show", 0);
}

InputPicker::Layout::~Layout() {
  // FIXME: Workaround. OSP requirement. OSP want to block own touch event
  // while webkit internal picker is running.
  evas_object_smart_callback_call(parent_->web_view_->evas_object(),
      "input,picker,hide", 0);
  evas_object_focus_set(parent_->web_view_->evas_object(), true);

  if (!conformant_)
    return;

  if (is_color_picker_)
    DeleteColorPickerCallbacks();
  else
    DeleteDatePickerCallbacks();

  evas_object_del(conformant_);
  conformant_ = nullptr;
}

// static
InputPicker::Layout* InputPicker::Layout::CreateAndShowColorPickerLayout(
    InputPicker* parent, int r, int g, int b) {
  if (IsWearableProfile()) {
    LOG(INFO) << "Wearable color picker not supported by platform.";
    return nullptr;
  }
  std::unique_ptr<Layout> picker_layout(new Layout(parent));
  picker_layout->is_color_picker_ = true;

  if (!picker_layout->AddBaseLayout(
      dgettext("WebKit", "IDS_WEBVIEW_HEADER_SELECT_COLOUR"),
      "colorselector_popup_layout")) {
    return nullptr;
  }

  picker_layout->color_rect_ =
      evas_object_rectangle_add(evas_object_evas_get(picker_layout->layout_));
  if (!picker_layout->color_rect_)
    return nullptr;

  evas_object_size_hint_weight_set(picker_layout->color_rect_,
      EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_show(picker_layout->color_rect_);
  evas_object_color_set(picker_layout->color_rect_, r, g, b, 255);
  elm_object_part_content_set(
      picker_layout->layout_, "rect", picker_layout->color_rect_);

  if (!picker_layout->AddColorSelector(r, g, b))
    return nullptr;

  if (!picker_layout->AddButtons())
    return nullptr;

  picker_layout->red_ = r;
  picker_layout->green_ = g;
  picker_layout->blue_ = b;

  picker_layout->AddColorPickerCallbacks();
  evas_object_show(picker_layout->popup_);
  return picker_layout.release();
}

static char* GetDateTimeFormat() {
#if defined(OS_TIZEN)
  char* language = getenv("LANGUAGE");
  if (setenv("LANGUAGE", "en_US", 1))
    LOG(ERROR) << "setenv failed ";

  char* region_format = vconf_get_str(VCONFKEY_REGIONFORMAT);
  if (!region_format)
    return nullptr;

  int time_value = 0;
  char buf[256] = { 0, };
  if (vconf_get_int(VCONFKEY_REGIONFORMAT_TIME1224, &time_value) < 0)
    return nullptr;
  if (time_value == VCONFKEY_TIME_FORMAT_24)
    snprintf(buf, sizeof(buf), "%s_DTFMT_24HR", region_format);
  else
    snprintf(buf, sizeof(buf), "%s_DTFMT_12HR", region_format);

  free(region_format);

  // FIXME: Workaround fix for region format.
  int buf_length = strlen(buf);
  for (int i = 0; i < buf_length - 4; i++) {
    if (buf[i] == 'u' && buf[i + 1] == 't' && buf[i + 2] == 'f') {
      if (buf[i + 3] == '8') {
        // utf8 -> UTF-8
        for (int j = buf_length; j > i + 3; j--)
          buf[j] = buf[j - 1];
        buf[i + 3] = '-';
        buf[buf_length + 1] = '\0';
      } else if (buf[i + 3] == '-' && buf[i + 4] == '8') {
        // utf-8 -> UTF-8
      } else {
        break;
      }

      buf[i] = 'U';
      buf[i + 1] = 'T';
      buf[i + 2] = 'F';
      break;
    }
  }

  char* date_time_format = dgettext("dt_fmt", buf);

  if (!language || !strcmp(language, "")) {
    if (unsetenv("LANGUAGE"))
      LOG(ERROR) << "unsetenv failed ";
  } else if (setenv("LANGUAGE", language, 1))
    LOG(ERROR) << "setenv failed";

  // FIXME: Workaround fix for not supported dt_fmt.
  // Use default format if dt_fmt string is not exist.
  if (strlen(date_time_format) == strlen(buf) &&
      !strncmp(date_time_format, buf, strlen(buf))) {
    return nullptr;
  }

  return strdup(date_time_format);
#else
  return nullptr;
#endif
}

// static
InputPicker::Layout* InputPicker::Layout::CreateAndShowDateLayout(
    InputPicker* parent, struct tm* current_time, ui::TextInputType type) {
  std::unique_ptr<Layout> picker_layout(new Layout(parent));

  picker_layout->input_type_ = type;

  std::string title;
  switch (type) {
    case ui::TEXT_INPUT_TYPE_DATE: {
      title = "IDS_WEBVIEW_HEADER_SET_DATE";
      break;
    }
    case ui::TEXT_INPUT_TYPE_WEEK: {
      title = "IDS_WEBVIEW_HEADER_SET_WEEK";
      break;
    }
    case ui::TEXT_INPUT_TYPE_MONTH: {
      title = "IDS_WEBVIEW_HEADER_SET_MONTH";
      break;
    }
    default:
      NOTREACHED();
      break;
  }

  if (!picker_layout->AddBaseLayout(
    dgettext("WebKit", title.c_str()), "date_popup")) {
    return nullptr;
  }

  picker_layout->date_picker_ = elm_datetime_add(picker_layout->layout_);
  if (!picker_layout->date_picker_)
    return nullptr;

  if (IsWearableProfile()) {
    if (!picker_layout->SetDatetimePicker(
        picker_layout->date_picker_, "datepicker/circle")) {
      return nullptr;
    }
  } else {
    elm_object_part_content_set(picker_layout->layout_,
        "elm.swallow.datetime", picker_layout->date_picker_);
  }

  char* format = GetDateTimeFormat();
  if (format) {
    elm_datetime_format_set(picker_layout->date_picker_, format);
    free(format);
  } else {
    elm_datetime_format_set(picker_layout->date_picker_,
                            kDefaultDatetimeFormat);
  }

  elm_datetime_value_set(picker_layout->date_picker_, current_time);

  if (!picker_layout->AddButtons())
    return nullptr;

  if (type == ui::TEXT_INPUT_TYPE_MONTH) {
    elm_datetime_field_visible_set(
      picker_layout->date_picker_, ELM_DATETIME_DATE, EINA_FALSE);
  }
  elm_datetime_field_visible_set(
      picker_layout->date_picker_, ELM_DATETIME_HOUR, EINA_FALSE);
  elm_datetime_field_visible_set(
      picker_layout->date_picker_, ELM_DATETIME_MINUTE, EINA_FALSE);

  picker_layout->AddDatePickerCallbacks();
  if (!IsWearableProfile())
    evas_object_show(picker_layout->popup_);

  return picker_layout.release();
}

// static
InputPicker::Layout* InputPicker::Layout::CreateAndShowDateTimeLayout(
    InputPicker* parent, struct tm* current_time, ui::TextInputType type) {
  if (IsWearableProfile()) {
    LOG(INFO) << "Multiple pages for date/time picker not supported by platform";
    return nullptr;
  }
  std::unique_ptr<Layout> picker_layout(new Layout(parent));

  picker_layout->input_type_ = type;

  elm_object_scale_set(picker_layout->popup_, 0.7);
  if (!picker_layout->AddBaseLayout(dgettext(
        "WebKit", "IDS_WEBVIEW_HEADER_SET_DATE_AND_TIME"), "datetime_popup")) {
    return nullptr;
  }

  picker_layout->time_picker_ = elm_datetime_add(picker_layout->layout_);
  picker_layout->date_picker_ = elm_datetime_add(picker_layout->layout_);
  if (!picker_layout->time_picker_ || !picker_layout->date_picker_)
    return nullptr;

  elm_object_part_content_set(picker_layout->layout_,
                              "elm.swallow.datetime",
                              picker_layout->time_picker_);
  elm_object_part_content_set(picker_layout->layout_,
                              "elm.swallow.datetime2",
                              picker_layout->date_picker_);
  elm_object_style_set(picker_layout->time_picker_, "time_layout");

  char* format = GetDateTimeFormat();
  if (format) {
    elm_datetime_format_set(picker_layout->date_picker_, format);
    elm_datetime_format_set(picker_layout->time_picker_, format);
    free(format);
  } else {
    elm_datetime_format_set(picker_layout->date_picker_,
                            kDefaultDatetimeFormat);
    elm_datetime_format_set(picker_layout->time_picker_,
                            kDefaultDatetimeFormat);
  }

  elm_datetime_value_set(picker_layout->date_picker_, current_time);
  elm_datetime_value_set(picker_layout->time_picker_, current_time);
  if (!picker_layout->AddButtons())
    return nullptr;

  picker_layout->AddDatePickerCallbacks();
  evas_object_show(picker_layout->popup_);

  return picker_layout.release();
}

bool InputPicker::Layout::SetDatetimePicker(
    Evas_Object* picker, const char* style) {
  if (!IsWearableProfile())
    return false;
#if defined(OS_TIZEN) && !defined(OS_TIZEN_TV_PRODUCT)
  Evas_Object* circle_datetime =
      eext_circle_object_datetime_add(picker, circle_surface_);
  if (!circle_datetime)
    return false;
  eext_rotary_object_event_activated_set(picker, EINA_TRUE);
#endif
  elm_object_style_set(picker, style);
  elm_object_part_content_set(layout_, "elm.swallow.content", picker);
  return true;
}

// static
InputPicker::Layout* InputPicker::Layout::CreateAndShowTimeLayout(
    InputPicker* parent, struct tm* current_time) {
  std::unique_ptr<Layout> picker_layout(new Layout(parent));

  picker_layout->input_type_ = ui::TEXT_INPUT_TYPE_TIME;

  if (!picker_layout->AddBaseLayout(
      dgettext("WebKit", "IDS_WEBVIEW_HEADER_SET_TIME"), "date_popup")) {
    return nullptr;
  }

  picker_layout->time_picker_ = elm_datetime_add(picker_layout->layout_);
  if (!picker_layout->time_picker_)
    return nullptr;

  if (IsWearableProfile()) {
    if (!picker_layout->SetDatetimePicker(
        picker_layout->time_picker_, "timepicker/circle")) {
      return nullptr;
    }
  } else {
    elm_object_style_set(picker_layout->time_picker_, "time_layout");
    elm_object_part_content_set(picker_layout->layout_,
        "elm.swallow.datetime", picker_layout->time_picker_);
  }

  char* format = GetDateTimeFormat();
  if (format) {
    elm_datetime_format_set(picker_layout->time_picker_, format);
    free(format);
  } else {
    elm_datetime_format_set(picker_layout->time_picker_,
                            kDefaultDatetimeFormat);
  }

  elm_datetime_value_set(picker_layout->time_picker_, current_time);

  if (!picker_layout->AddButtons())
    return nullptr;

  elm_datetime_field_visible_set(
      picker_layout->time_picker_, ELM_DATETIME_YEAR, EINA_FALSE);
  elm_datetime_field_visible_set(
      picker_layout->time_picker_, ELM_DATETIME_MONTH, EINA_FALSE);
  elm_datetime_field_visible_set(
      picker_layout->time_picker_, ELM_DATETIME_DATE, EINA_FALSE);

  picker_layout->AddDatePickerCallbacks();
  if (!IsWearableProfile())
    evas_object_show(picker_layout->popup_);

  return picker_layout.release();
}

bool InputPicker::Layout::AddBaseLayout(const char* title,
                                        const char* layout_group) {
  Evas_Object* top_widget = parent_->web_view_->GetElmWindow();

  conformant_ = elm_conformant_add(top_widget);
  if (!conformant_)
    return false;

  evas_object_size_hint_weight_set(
      conformant_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  elm_win_resize_object_add(top_widget, conformant_);
  evas_object_show(conformant_);

  if (IsWearableProfile()) {
#if defined(OS_TIZEN) && !defined(OS_TIZEN_TV_PRODUCT)
    circle_surface_ = eext_circle_surface_conformant_add(conformant_);
    if (!circle_surface_)
      return false;
#endif
    layout_ = elm_layout_add(conformant_);
    if (!layout_)
      return false;
    elm_layout_theme_set(layout_, "layout", "circle", "datetime");
    elm_object_part_text_set(layout_, "elm.text", title);
    elm_object_content_set(conformant_, layout_);
  } else {
    Evas_Object* layout = elm_layout_add(conformant_);
    if (!layout)
      return false;

    elm_layout_theme_set(layout, "layout", "application", "default");
    evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_show(layout);
    elm_object_content_set(conformant_, layout);

    popup_ = elm_popup_add(layout);
    if (!popup_)
      return false;

    elm_popup_align_set(popup_, ELM_NOTIFY_ALIGN_FILL, 1.0);

    elm_object_part_text_set(popup_, "title,text", title);
    evas_object_size_hint_weight_set(popup_, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);

    layout_ = elm_layout_add(popup_);
    if (!layout_)
      return false;

    elm_object_content_set(popup_, layout_);
    base::FilePath edj_dir;
    base::FilePath control_path;
    PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
    control_path = edj_dir.Append(FILE_PATH_LITERAL("control.edj"));
    elm_layout_file_set(layout_, control_path.AsUTF8Unsafe().c_str(), layout_group);
    evas_object_size_hint_align_set(layout_, EVAS_HINT_FILL, EVAS_HINT_FILL);
  }
  evas_object_size_hint_weight_set(layout_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

  return true;
}

bool InputPicker::Layout::AddButtons() {
  if (IsWearableProfile()) {
    set_button_ = elm_button_add(layout_);
  } else {
    set_button_ = elm_button_add(popup_);
    cancel_button_ = elm_button_add(popup_);
    if (!cancel_button_)
      return false;
    elm_object_domain_translatable_part_text_set(
        cancel_button_, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_CANCEL_ABB4");
  }

  if (!set_button_)
    return false;

  elm_object_domain_translatable_part_text_set(set_button_, NULL, "WebKit",
                                               "IDS_WEBVIEW_BUTTON_SET_ABB2");
  if (IsWearableProfile()) {
    elm_object_style_set(set_button_, "bottom");
    elm_object_part_content_set(layout_, "elm.swallow.btn", set_button_);
  } else {
    elm_object_style_set(set_button_, "popup");
    elm_object_style_set(cancel_button_, "popup");
    elm_object_part_content_set(popup_, "button2", set_button_);
    elm_object_part_content_set(popup_, "button1", cancel_button_);
    evas_object_focus_set(cancel_button_, true);
  }
  evas_object_focus_set(set_button_, true);

  return true;
}

bool InputPicker::Layout::AddColorSelector(int r, int g, int b) {
  color_picker_ = elm_colorselector_add(layout_);
  if (!color_picker_)
    return false;

  elm_colorselector_mode_set(color_picker_, ELM_COLORSELECTOR_PALETTE);
  evas_object_size_hint_fill_set(color_picker_, EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_size_hint_weight_set(
      color_picker_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

  if (IsTvProfile()) {
    elm_colorselector_color_set(color_picker_, r, g, b, 255);
  } else {
    Eina_List* color_list = const_cast<Eina_List*>(
        elm_colorselector_palette_items_get(color_picker_));
    Eina_List* list = nullptr;
    Elm_Object_Item* it = nullptr;
    void* item = nullptr;
    int red = 0;
    int green = 0;
    int blue = 0;
    int alpha = 0;

    EINA_LIST_FOREACH(color_list, list, item) {
      if (item) {
        Elm_Object_Item* elm_item = static_cast<Elm_Object_Item*>(item);
        elm_colorselector_palette_item_color_get(
            elm_item, &red, &green, &blue, &alpha);
        if (red == r && green == g && blue == b) {
          it = elm_item;
          break;
        }
      }
    }

    if (!it)
      it = static_cast<Elm_Object_Item*>(eina_list_nth(color_list, 0));

    elm_object_item_signal_emit(it, "elm,state,selected", "elm");
  }

  elm_object_part_content_set(layout_, "colorpalette", color_picker_);

  return true;
}

void InputPicker::Layout::AddColorPickerCallbacks() {
  evas_object_smart_callback_add(color_picker_,
                                 "color,item,selected",
                                 ColorPickerItemSelectedCallback,
                                 color_rect_);
  evas_object_smart_callback_add(set_button_, "clicked",
                                 ColorPickerSelectFinishedCallback, this);

  if (!IsWearableProfile())
    evas_object_smart_callback_add(cancel_button_, "clicked",
                                   ColorPickerBackKeyCallback, this);

#if defined(OS_TIZEN)
  eext_object_event_callback_add(layout_,
                                 EEXT_CALLBACK_BACK,
                                 ColorPickerBackKeyCallback,
                                 this);
#endif
}

void InputPicker::Layout::DeleteColorPickerCallbacks() {
  if (color_picker_) {
    evas_object_smart_callback_del(color_picker_,
                                   "color,item,selected",
                                   ColorPickerItemSelectedCallback);
  }

  if (set_button_) {
    evas_object_smart_callback_del(set_button_, "clicked",
                                   ColorPickerSelectFinishedCallback);
  }
  if (!IsWearableProfile() && cancel_button_) {
    evas_object_smart_callback_del(cancel_button_, "clicked",
                                   ColorPickerBackKeyCallback);
  }

#if defined(OS_TIZEN)
  if (layout_) {
    eext_object_event_callback_del(
        layout_, EEXT_CALLBACK_BACK, ColorPickerBackKeyCallback);
  }
#endif
}

void InputPicker::Layout::AddDatePickerCallbacks() {
  evas_object_smart_callback_add(set_button_, "clicked",
                                 DatePickerSelectFinishedCallback, this);
  if (!IsWearableProfile())
    evas_object_smart_callback_add(cancel_button_, "clicked",
                                   DatePickerBackKeyCallback, this);

  if (input_type_ == ui::TEXT_INPUT_TYPE_DATE_TIME_FIELD ||
      input_type_ == ui::TEXT_INPUT_TYPE_DATE_TIME ||
      input_type_ == ui::TEXT_INPUT_TYPE_DATE_TIME_LOCAL) {
    evas_object_smart_callback_add(date_picker_,
                                   "changed",
                                   DatePickerItemChangedCallback,
                                   this);
    evas_object_smart_callback_add(time_picker_,
                                   "changed",
                                   DatePickerItemChangedCallback,
                                   this);
  }

  if (IsTvProfile()) {
    elm_object_signal_emit(layout_, "TV", "align,swallow.datetime");
  }

#if defined(OS_TIZEN)
  eext_object_event_callback_add(layout_, EEXT_CALLBACK_BACK,
      DatePickerBackKeyCallback, this);
#endif
}

void InputPicker::Layout::DeleteDatePickerCallbacks() {
  if (set_button_) {
    evas_object_smart_callback_del(set_button_, "clicked",
                                   DatePickerSelectFinishedCallback);
  }
  if (!IsWearableProfile() && cancel_button_) {
    evas_object_smart_callback_del(cancel_button_, "clicked",
                                   DatePickerBackKeyCallback);
  }

  if (input_type_ == ui::TEXT_INPUT_TYPE_DATE_TIME_FIELD ||
      input_type_ == ui::TEXT_INPUT_TYPE_DATE_TIME ||
      input_type_ == ui::TEXT_INPUT_TYPE_DATE_TIME_LOCAL) {
    if (date_picker_) {
      evas_object_smart_callback_del(date_picker_,
                                     "changed",
                                     DatePickerItemChangedCallback);
    }
    if (time_picker_) {
      evas_object_smart_callback_del(time_picker_,
                                     "changed",
                                     DatePickerItemChangedCallback);
    }
  }

#if defined(OS_TIZEN)
  if (layout_) {
    eext_object_event_callback_del(
        layout_, EEXT_CALLBACK_BACK, DatePickerBackKeyCallback);
  }
#endif
}

// static
void InputPicker::Layout::ColorPickerItemSelectedCallback(void* data,
                                                          Evas_Object* obj,
                                                          void* event_info) {
  int r(0), g(0), b(0), a(0);
  Elm_Object_Item* color_it = static_cast<Elm_Object_Item*>(event_info);
  elm_colorselector_palette_item_color_get(color_it, &r, &g, &b, &a);
  evas_object_color_set(static_cast<Evas_Object*>(data), r, g, b, a);
}

// static
void InputPicker::Layout::ColorPickerSelectFinishedCallback(void* data,
                                                            Evas_Object* obj,
                                                            void* event_info) {
  Layout* picker_layout = static_cast<Layout*>(data);

  int r(0), g(0), b(0), a(0);
  evas_object_color_get(picker_layout->color_rect_, &r, &g, &b, &a);

  picker_layout->parent_->web_view_->
      web_contents().DidChooseColorInColorChooser(SkColorSetARGB(a, r, g, b));
  picker_layout->parent_->RemoveColorPicker();
}

// static
void InputPicker::Layout::DatePickerSelectFinishedCallback(void* data,
                                                           Evas_Object* obj,
                                                           void* event_info) {
  struct tm current_time;
  memset(&current_time, 0, sizeof(struct tm));

  Layout* picker_layout = static_cast<Layout*>(data);

  if (picker_layout->input_type_ == ui::TEXT_INPUT_TYPE_TIME)
    elm_datetime_value_get(picker_layout->time_picker_, &current_time);
  else
    elm_datetime_value_get(picker_layout->date_picker_, &current_time);

  char dateStr[20] = { 0, };

  switch (picker_layout->input_type_) {
    case ui::TEXT_INPUT_TYPE_DATE: {
      strftime(dateStr, 20, "%F", &current_time);
      break;
    }
    case ui::TEXT_INPUT_TYPE_DATE_TIME_FIELD:
    case ui::TEXT_INPUT_TYPE_DATE_TIME: {
      strftime(dateStr, 20, "%FT%RZ", &current_time);
      break;
    }
    case ui::TEXT_INPUT_TYPE_DATE_TIME_LOCAL: {
      strftime(dateStr, 20, "%FT%R", &current_time);
      break;
    }
    case ui::TEXT_INPUT_TYPE_TIME: {
      strftime(dateStr, 20, "%R", &current_time);
      break;
    }
    case ui::TEXT_INPUT_TYPE_WEEK: {
      // Call mktime: current_time.tm_wday will be set
      mktime(&current_time);
      strftime(dateStr, 20, "%G-W%V", &current_time);
      break;
    }
    case ui::TEXT_INPUT_TYPE_MONTH: {
      strftime(dateStr, 20, "%Y-%m", &current_time);
      break;
    }
    default:
      NOTREACHED();
      break;
  }

  picker_layout->parent_->web_view_->
      web_contents().DidReplaceDateTime(std::string(dateStr));
  picker_layout->parent_->RemoveDatePicker(false);
}

// static
void InputPicker::Layout::DatePickerItemChangedCallback(void* data,
                                                        Evas_Object* obj,
                                                        void* event_info) {
  struct tm current_time;
  memset(&current_time, 0, sizeof(struct tm));

  Layout* picker_layout = static_cast<Layout*>(data);
  if (obj == picker_layout->date_picker_) {
    elm_datetime_value_get(picker_layout->date_picker_, &current_time);
    elm_datetime_value_set(picker_layout->time_picker_, &current_time);
  } else if (obj == picker_layout->time_picker_) {
    elm_datetime_value_get(picker_layout->time_picker_, &current_time);
    elm_datetime_value_set(picker_layout->date_picker_, &current_time);
  }
}

// static
void InputPicker::Layout::ColorPickerBackKeyCallback(void* data,
                                                     Evas_Object* obj,
                                                     void* event_info) {
  Layout* picker_layout = static_cast<Layout*>(data);

  picker_layout->parent_->web_view_->
      web_contents().DidChooseColorInColorChooser(SkColorSetARGB(255,
          picker_layout->red_, picker_layout->green_, picker_layout->blue_));
  picker_layout->parent_->RemoveColorPicker();
}

// static
void InputPicker::Layout::DatePickerBackKeyCallback(void* data,
                                                    Evas_Object* obj,
                                                    void* event_info) {
  Layout* picker_layout = static_cast<Layout*>(data);
  if (picker_layout) {
    // pass true to RemoveDatePicker to cancelDateTimeDialog
    picker_layout->parent_->RemoveDatePicker(true);
  }
}

InputPicker::InputPicker(EWebView* view)
    : web_view_(view),
      picker_layout_(nullptr) {
}

InputPicker::~InputPicker() {}

void InputPicker::ShowColorPicker(int r, int g, int b, int a) {
  picker_layout_.reset(
      Layout::CreateAndShowColorPickerLayout(this, r, g, b));
  if (!picker_layout_) {
    LOG(ERROR) << "Failed to create color picker.";
    // We need to notify engine that default color is chosen
    // otherwise selecting will never be finished.
    web_view_->web_contents().DidChooseColorInColorChooser(
        SkColorSetARGB(a, r, g, b));
  }
}

void InputPicker::ShowDatePicker(ui::TextInputType input_type,
                                 double input_date) {
  web_view_->ExecuteEditCommand("Unselect", 0);

  time_t timep;
  struct tm tm;
  if (!std::isfinite(input_date)) {
    time(&timep);
    localtime_r(&timep, &tm);
  } else if (input_type == ui::TEXT_INPUT_TYPE_MONTH) {
    // When type is month, input_date is number of month since epoch.
    unsigned int year = floor(input_date / 12.0) + 1970.0;
    unsigned int month = input_date - (year - 1970) * 12 + 1;
    CHECK_LE(month, 12u);

    char date[12];
    snprintf(date, sizeof(date), "%d-%d", year, month);
    char* last_char = strptime(date, "%Y-%m", &tm);
    DCHECK(last_char);
  } else {
    // In all other cases, input_date is number of milliseconds since epoch.
    timep = base::Time::FromDoubleT(input_date / 1000).ToTimeT();
    gmtime_r(&timep, &tm);
  }
  struct tm* current_time = &tm;

  switch (input_type) {
    case ui::TEXT_INPUT_TYPE_DATE:
    case ui::TEXT_INPUT_TYPE_WEEK:
    case ui::TEXT_INPUT_TYPE_MONTH: {
      picker_layout_.reset(
          Layout::CreateAndShowDateLayout(this, current_time, input_type));
      break;
    }
    case ui::TEXT_INPUT_TYPE_DATE_TIME_FIELD:
    case ui::TEXT_INPUT_TYPE_DATE_TIME:
    case ui::TEXT_INPUT_TYPE_DATE_TIME_LOCAL: {
      picker_layout_.reset(
          Layout::CreateAndShowDateTimeLayout(this, current_time, input_type));
      break;
    }
    case ui::TEXT_INPUT_TYPE_TIME: {
      picker_layout_.reset(
          Layout::CreateAndShowTimeLayout(this, current_time));
      break;
    }
    default:
      LOG(ERROR) << "Invalid date picker type.";
      break;
  }

  if (!picker_layout_) {
    LOG(ERROR) << "Failed to create date picker.";
    // We need to notify engine that empty string is chosen
    // otherwise selecting will never be finished.
    web_view_->web_contents().DidReplaceDateTime(std::string());
  }
}

void InputPicker::RemoveColorPicker() {
  if (!picker_layout_)
    return;

  picker_layout_.reset();
  web_view_->web_contents().DidEndColorChooser();
}

void InputPicker::RemoveDatePicker(bool cancel) {
  if (!picker_layout_)
    return;

  picker_layout_.reset();
  if (cancel)
    web_view_->web_contents().DidCancelDialog();
}

}  // namespace content
