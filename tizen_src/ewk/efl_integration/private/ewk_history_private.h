// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_HISTORY_PRIVATE_H
#define EWK_HISTORY_PRIVATE_H

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "private/back_forward_list.h"

class _Ewk_History_Item :
    public back_forward_list::Item {
 public:
  _Ewk_History_Item(content::NavigationEntry* entry) {
    if (entry) {
      url_ = entry->GetURL();
      original_url_ = entry->GetUserTypedURL();
      title_ = base::UTF16ToUTF8(entry->GetTitle());
    }
  }

  const GURL& GetURL() const override {
    return url_;
  }

  const GURL& GetOriginalURL() const override {
    return original_url_;
  }

  const std::string& GetTitle() const override {
    return title_;
  }

 private:
  GURL url_;
  GURL original_url_;
  std::string title_;
};


class _Ewk_History :
    public back_forward_list::List {
 public:
  _Ewk_History(content::NavigationController &controller);
  ~_Ewk_History();

  int GetCurrentIndex() const override;
  int GetLength() const override;
  int GetBackListLength() const override;
  int GetForwardListLength() const override;
  back_forward_list::Item* GetItemAtIndex(int index) const override;

 private:
  const int current_index_;
  std::vector<_Ewk_History_Item*> items_;
};


#endif // EWK_HISTORY_PRIVATE_H
