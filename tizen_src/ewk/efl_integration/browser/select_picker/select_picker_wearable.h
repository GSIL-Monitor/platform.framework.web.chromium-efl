// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_WEARABLE_H_
#define EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_WEARABLE_H_

#include "browser/select_picker/select_picker_base.h"

class SelectPickerEflWearable : public SelectPickerBase {
 public:
  explicit SelectPickerEflWearable(EWebView* web_view,
                           int selected_index,
                           bool is_multiple_selection);
  ~SelectPickerEflWearable() override;

 protected:
  void Init(const std::vector<content::MenuItem>& items,
            const gfx::Rect& bounds) override;
  void CreateAndPopulatePopupList(
      const std::vector<content::MenuItem>& items) override;
};

#endif  // EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_WEARABLE_H_
