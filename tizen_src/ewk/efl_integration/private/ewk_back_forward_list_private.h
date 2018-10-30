// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_BACK_FORWARD_LIST_PRIVATE_H
#define EWK_BACK_FORWARD_LIST_PRIVATE_H

#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents_observer.h"
#include "private/back_forward_list.h"

#include <map>

class _Ewk_Back_Forward_List_Item
    : public back_forward_list::Item,
      public base::RefCounted<_Ewk_Back_Forward_List_Item> {
 public:
  _Ewk_Back_Forward_List_Item(content::NavigationEntry* entry) {
    Update(entry);
  }

  /* LCOV_EXCL_START */
  void Update(const content::NavigationEntry* entry) {
    if (entry) {
      url_ = entry->GetURL();
      original_url_ = entry->GetUserTypedURL();
      title_ = base::UTF16ToUTF8(entry->GetTitle());
    }
  }

  const GURL& GetURL() const override { return url_; }

  const GURL& GetOriginalURL() const override { return original_url_; }

  const std::string& GetTitle() const override { return title_; }
  /* LCOV_EXCL_STOP */

 private:
  GURL url_;
  GURL original_url_;
  std::string title_;
};

class _Ewk_Back_Forward_List : public back_forward_list::List,
                               public content::WebContentsObserver {
 public:
  typedef std::map<const content::NavigationEntry*,
                   scoped_refptr<_Ewk_Back_Forward_List_Item> >
      CacheMap;

  _Ewk_Back_Forward_List(content::NavigationController& controller);
  ~_Ewk_Back_Forward_List() {}

  int GetCurrentIndex() const override;
  int GetLength() const override;
  int GetBackListLength() const override;
  int GetForwardListLength() const override;
  back_forward_list::Item* GetItemAtIndex(int index) const override;

  void NewPageCommited(int prev_entry_index,
                       content::NavigationEntry* new_entry);
  void UpdateItemWithEntry(const content::NavigationEntry* entry);
  void ClearCache();

  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void NavigationEntryChanged(
      const content::EntryChangedDetails& change_details) override;

 private:
  _Ewk_Back_Forward_List_Item* FindOrCreateItem(int index) const;
  void InsertEntryToIndexes(unsigned index,
                            content::NavigationEntry* entry) const;

 private:
  content::NavigationController& navigation_controller_;
  mutable CacheMap cache_;
  mutable std::vector<content::NavigationEntry*> indexes_;
};

#endif  // EWK_BACK_FORWARD_LIST_PRIVATE_H
