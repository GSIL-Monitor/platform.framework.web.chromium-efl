// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_back_forward_list_private.h"

#include "content/public/browser/navigation_details.h"

_Ewk_Back_Forward_List::_Ewk_Back_Forward_List(
    content::NavigationController& controller)
    : navigation_controller_(controller) {}

/* LCOV_EXCL_START */
int _Ewk_Back_Forward_List::GetCurrentIndex() const {
  return navigation_controller_.GetCurrentEntryIndex();
}

int _Ewk_Back_Forward_List::GetLength() const {
  return navigation_controller_.GetEntryCount();
}

int _Ewk_Back_Forward_List::GetBackListLength() const {
  int current = navigation_controller_.GetCurrentEntryIndex();
  return current < 0 ? 0 : current;
}

int _Ewk_Back_Forward_List::GetForwardListLength() const {
  int current = navigation_controller_.GetCurrentEntryIndex();
  return navigation_controller_.GetEntryCount() - current - 1;
}
/* LCOV_EXCL_STOP */

back_forward_list::Item* _Ewk_Back_Forward_List::GetItemAtIndex(
    int index) const {
  index += navigation_controller_.GetCurrentEntryIndex();
  int count = navigation_controller_.GetEntryCount();
  if (index < 0 || index >= count) {
    return NULL;
  }
  return FindOrCreateItem(index); // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
void _Ewk_Back_Forward_List::NewPageCommited(
    int prev_entry_index,
    content::NavigationEntry* new_entry) {
  int current = prev_entry_index + 1;
  if (current != navigation_controller_.GetCurrentEntryIndex()) {
    return;
  }

  // When user went back several pages and now loaded
  // some new page (so new entry is commited) then all forward items
  // were deleted, so we have to do the same in our cache.
  for (unsigned i = current; i < indexes_.size(); ++i) {
    content::NavigationEntry* entry = indexes_[i];
    indexes_[i] = NULL;
    if (entry) {
      cache_.erase(entry);
    }
  }

  InsertEntryToIndexes(current, new_entry);
  cache_[new_entry] = scoped_refptr<_Ewk_Back_Forward_List_Item>(
      new _Ewk_Back_Forward_List_Item(new_entry));
}
/* LCOV_EXCL_STOP */

void _Ewk_Back_Forward_List::UpdateItemWithEntry(
    const content::NavigationEntry* entry) {
  if (entry) {
    CacheMap::iterator it = cache_.find(entry);
    if (it != cache_.end()) {
      it->second->Update(entry); // LCOV_EXCL_LINE
    }
  }
}

/* LCOV_EXCL_START */
void _Ewk_Back_Forward_List::ClearCache() {
  indexes_.clear();
  cache_.clear();
}
void _Ewk_Back_Forward_List::NavigationEntryCommitted(
    const content::LoadCommittedDetails& load_details) {
  NewPageCommited(load_details.previous_entry_index, load_details.entry);
}

void _Ewk_Back_Forward_List::NavigationEntryChanged(
    const content::EntryChangedDetails& change_details) {
  UpdateItemWithEntry(change_details.changed_entry);
}

_Ewk_Back_Forward_List_Item* _Ewk_Back_Forward_List::FindOrCreateItem(
    int index) const {
  content::NavigationEntry* entry =
      navigation_controller_.GetEntryAtIndex(index);

  if (!entry) {
    return NULL;
  }

  _Ewk_Back_Forward_List_Item* item = NULL;
  CacheMap::iterator it = cache_.find(entry);
  if (it != cache_.end()) {
    // item already in cache
    item = it->second.get();
  } else {
    // need to create new item
    item = new _Ewk_Back_Forward_List_Item(entry);
    cache_[entry] = scoped_refptr<_Ewk_Back_Forward_List_Item>(item);
  }

  InsertEntryToIndexes(index, entry);

  return item;
}

void _Ewk_Back_Forward_List::InsertEntryToIndexes(
    unsigned index,
    content::NavigationEntry* entry) const {
  if (index == indexes_.size()) {
    indexes_.push_back(entry);
    return;
  }

  if (index > indexes_.size()) {
    indexes_.resize(index + 1, NULL);
  }
  indexes_[index] = entry;
}
/* LCOV_EXCL_STOP */
