// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_TV_H_
#define EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_TV_H_

#include "browser/select_picker/form_navigable_picker.h"
#include "ui/gfx/geometry/point.h"

class SelectPickerEflTv : public FormNavigablePicker {
 public:
  explicit SelectPickerEflTv(EWebView* web_view,
                           int selected_index,
                           bool is_multiple_selection);
  ~SelectPickerEflTv() override;

 protected:
  const char* GetItemStyle() const override;
  void Init(const std::vector<content::MenuItem>& items,
            const gfx::Rect& bounds) override;
  void CreateAndPopulatePopupList(
      const std::vector<content::MenuItem>& items) override;

 private:
  void CoordinatePopupPosition(const gfx::Point& origin);
  void NextItem(bool foward);
  static Eina_Bool EcoreEventFilterCallback(void* user_data,
                                            void* /*loop_data*/,
                                            int type,
                                            void* event);
  Ecore_Event_Filter* ecore_events_filter_;
};

#endif  // EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_TV_H_
