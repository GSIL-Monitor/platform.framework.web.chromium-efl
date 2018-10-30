// Copyright 2014-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_H_
#define EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_H_

#include <Elementary.h>

#include "browser/select_picker/form_navigable_picker.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

class EWebView;
class GenlistCallbackData;

class SelectPickerEfl : public FormNavigablePicker {
 public:

  explicit SelectPickerEfl(EWebView* web_view,
                           int selected_index,
                           bool is_multiple_selection);
  ~SelectPickerEfl() override;

  gfx::Rect GetGeometryDIP() const override;

 protected:

  void Init(const std::vector<content::MenuItem>& items,
            const gfx::Rect& bounds) override;
  void ItemSelected(GenlistCallbackData* data, void* event_info) override;
  void ClearData() override;
  void CreateAndPopulatePopupList(
      const std::vector<content::MenuItem>& items) override;

 private:
  static Evas_Object* IconGetCallback(void*, Evas_Object*, const char*);
  static void ShowPickerDoneCallback(void* data,
                                     Evas_Object*,
                                     const char*,
                                     const char*);
  void DestroyRadioList();
#if defined(TIZEN_ATK_SUPPORT)
  void AddAtkObject();
  void RemoveAtkObject();

  static Eina_Bool AccessNavigateToPrevCallback(
      void* data,
      Evas_Object* obj,
      Elm_Access_Action_Info* action_info);
  static Eina_Bool AccessNavigateToNextCallback(
      void* data,
      Evas_Object* obj,
      Elm_Access_Action_Info* action_info);
  static Eina_Bool AccessDoneCallback(void* data,
                                      Evas_Object* obj,
                                      Elm_Access_Action_Info* action_info);
#endif

  static Eina_Bool EcoreEventFilterCallback(void* data,
                                            void* loop_data,
                                            int type,
                                            void* event);

  Ecore_Event_Filter* ecore_events_filter_;

  Evas_Object* radio_main_;
#if defined(TIZEN_ATK_SUPPORT)
  Evas_Object* prev_button_;
  Evas_Object* next_button_;
  Evas_Object* done_button_;
  Evas_Object* ao_prev_button_;
  Evas_Object* ao_next_button_;
  Evas_Object* ao_done_button_;
#endif
};

#endif  // EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_H_
