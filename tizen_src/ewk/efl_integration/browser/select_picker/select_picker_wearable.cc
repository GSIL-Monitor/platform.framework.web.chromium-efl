// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/select_picker/select_picker_wearable.h"

SelectPickerEflWearable::SelectPickerEflWearable(EWebView* web_view,
                                                 int selected_index,
                                                 bool is_multiple_selection)
    : SelectPickerBase(web_view,
                       selected_index,
                       is_multiple_selection,
                       ELM_GENLIST_ITEM_SCROLLTO_MIDDLE) {
  evas_object_size_hint_weight_set(layout_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
}

SelectPickerEflWearable::~SelectPickerEflWearable() {
  Hide();
}

void SelectPickerEflWearable::Init(const std::vector<content::MenuItem>& items,
                           const gfx::Rect& bounds) {
  CreateAndPopulatePopupList(items);
}

void SelectPickerEflWearable::CreateAndPopulatePopupList(
    const std::vector<content::MenuItem>& items) {
  popup_list_ = elm_genlist_add(layout_);
  elm_genlist_homogeneous_set(popup_list_, EINA_TRUE);
  elm_genlist_mode_set(popup_list_, ELM_LIST_COMPRESS);
  evas_object_size_hint_weight_set(popup_list_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(popup_list_, EVAS_HINT_FILL, EVAS_HINT_FILL);
  elm_scroller_content_min_limit(popup_list_, EINA_FALSE, EINA_TRUE);
  elm_genlist_multi_select_set(popup_list_, is_multiple_selection_);
  InitializeSelectedPickerData(items);
  elm_object_content_set(layout_, popup_list_);
}
