// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_INPUT_PICKER_INPUT_PICKER_H_
#define EWK_EFL_INTEGRATION_BROWSER_INPUT_PICKER_INPUT_PICKER_H_

#include <Evas.h>
#include <ctime>
#include <string>

#include "ewk/efl_integration/eweb_view.h"
#include "ui/base/ime/text_input_type.h"

#if defined(OS_TIZEN)
#include <efl_extension.h>
#endif

class EWebView;

namespace content {

class InputPicker {
 public:
  explicit InputPicker(EWebView* view);
  ~InputPicker();

  void ShowColorPicker(int r, int g, int b, int a);
  void ShowDatePicker(ui::TextInputType input_type, double input_date);

 private:
  class Layout {
   public:
    static Layout* CreateAndShowColorPickerLayout(
        InputPicker* parent, int r, int g, int b);
    static Layout* CreateAndShowDateLayout(
        InputPicker* parent, struct tm* currentTime, ui::TextInputType type);
    static Layout* CreateAndShowDateTimeLayout(
        InputPicker* parent, struct tm* currentTime, ui::TextInputType type);
    static Layout* CreateAndShowTimeLayout(
        InputPicker* parent, struct tm* currentTime);

    ~Layout();

   private:
    explicit Layout(InputPicker* parent);

    bool AddBaseLayout(const char* title, const char* layout_group);
    bool AddButtons();
    bool AddColorSelector(int r, int g, int b);
    void AddColorPickerCallbacks();
    void DeleteColorPickerCallbacks();
    void AddDatePickerCallbacks();
    void DeleteDatePickerCallbacks();

    bool SetDatetimePicker(Evas_Object* picker, const char* style);
	
    static void ColorPickerSelectFinishedCallback(void* data,
                                                  Evas_Object* obj,
                                                  void* event_info);
    static void ColorPickerItemSelectedCallback(void* data,
                                                Evas_Object* obj,
                                                void* event_info);
    static void DatePickerSelectFinishedCallback(void* data,
                                                 Evas_Object* obj,
                                                 void* event_info);
    static void DatePickerItemChangedCallback(void* data,
                                              Evas_Object* obj,
                                              void* event_info);

    static void ColorPickerBackKeyCallback(void* data,
                                           Evas_Object* obj,
                                           void* event_info);
    static void DatePickerBackKeyCallback(void* data,
                                          Evas_Object* obj,
                                          void* event_info);

    InputPicker* parent_;

    Evas_Object* conformant_;
    Evas_Object* popup_;
    Evas_Object* layout_;
    Evas_Object* set_button_;
    Evas_Object* cancel_button_;
    Evas_Object* color_picker_;
    Evas_Object* color_rect_;
    Evas_Object* date_picker_;
    Evas_Object* time_picker_;
#if defined(OS_TIZEN) && !defined(OS_TIZEN_TV_PRODUCT)
    Eext_Circle_Surface* circle_surface_;
#endif
    ui::TextInputType input_type_;
    bool is_color_picker_;
    int red_;
    int green_;
    int blue_;
  };

  void RemoveColorPicker();
  void RemoveDatePicker(bool cancel);

  EWebView* web_view_;
  std::unique_ptr<Layout> picker_layout_;
};

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_BROWSER_INPUT_PICKER_INPUT_PICKER_H_
