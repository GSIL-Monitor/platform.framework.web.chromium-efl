// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_FORM_NAVIGABLE_PICKER_H_
#define EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_FORM_NAVIGABLE_PICKER_H_

#include "browser/select_picker/select_picker_base.h"

class FormNavigablePicker : public SelectPickerBase {
 public:
  ~FormNavigablePicker() override;
  void UpdateFormNavigation(int form_element_count,
                            int current_node_index) override;
  void UpdatePickerData(int selected_index,
                        const std::vector<content::MenuItem>& items,
                        bool is_multiple_selection) override;

 protected:
  explicit FormNavigablePicker(EWebView* web_view,
                               int selected_index,
                               bool is_multi_select,
                               Elm_Genlist_Item_Scrollto_Type scrollto_type =
                                   ELM_GENLIST_ITEM_SCROLLTO_TOP);
  void RegisterCallbacks(const char* edj, bool overlay);
  void RegisterCallbacks();
  Evas_Object* AddBackground();
  void AddDoneButton();
  void ShowButtons();
  void RequestFormNavigationInformation();
  void FormNavigate(bool direction);
  void CloseList();

 private:
  void UpdateNavigationButtons();
  static void KeyUpCallback(void* data,
                            Evas* e,
                            Evas_Object* obj,
                            void* event_info);
#if defined(OS_TIZEN)
  static void HWBackKeyCallback(void* data, Evas_Object* obj, void* event_info);
#endif
  static void ListClosedCallback(void* data,
                                 Evas_Object* obj,
                                 const char* emission,
                                 const char* source);
  static void NavigateToPrevCallback(void* data,
                                     Evas_Object* obj,
                                     const char* emission,
                                     const char* source);
  static void NavigateToNextCallback(void* data,
                                     Evas_Object* obj,
                                     const char* emission,
                                     const char* source);
  struct FormNavigatorInfo {
    int count;
    int index;
    bool is_prev;
    bool is_next;
  } form_navigator_info_;
};

#endif  // EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_FORM_NAVIGABLE_PICKER_H_
