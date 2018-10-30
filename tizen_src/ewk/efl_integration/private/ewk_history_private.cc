// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_history_private.h"

_Ewk_History::_Ewk_History(
    content::NavigationController &controller)
    : current_index_(controller.GetCurrentEntryIndex()) {
  int cnt = controller.GetEntryCount();
  if (cnt) {
    items_.resize(cnt, 0);
    for (int i = 0; i < cnt; ++i) {
      _Ewk_History_Item* item = new _Ewk_History_Item(
          controller.GetEntryAtIndex(i));
      items_[i] = item;
    }
  }
}

_Ewk_History::~_Ewk_History() {
  for (unsigned i = 0; i < items_.size(); ++i) {
    delete items_[i];
  }
}

int _Ewk_History::GetCurrentIndex() const {
  return current_index_;
}

int _Ewk_History::GetLength() const {
  return items_.size();
}

int _Ewk_History::GetBackListLength() const {
  return current_index_ < 0 ? 0 : current_index_;
}

int _Ewk_History::GetForwardListLength() const {
  return items_.size() - current_index_ - 1;
}

back_forward_list::Item* _Ewk_History::GetItemAtIndex(int index) const {
  int actualIndex = index + current_index_;
  if (actualIndex < 0 || actualIndex >= static_cast<int>(items_.size())) {
    return 0;
  }
  return items_[actualIndex];
}
