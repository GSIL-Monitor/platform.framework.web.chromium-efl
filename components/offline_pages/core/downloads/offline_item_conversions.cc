// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/downloads/offline_item_conversions.h"

#include "base/strings/utf_string_conversions.h"
#include "components/offline_pages/core/background/save_page_request.h"
#include "components/offline_pages/core/offline_page_item.h"

using OfflineItemFilter = offline_items_collection::OfflineItemFilter;
using OfflineItemState = offline_items_collection::OfflineItemState;
using OfflineItemProgressUnit =
    offline_items_collection::OfflineItemProgressUnit;

namespace offline_pages {

OfflineItem OfflineItemConversions::CreateOfflineItem(
    const OfflinePageItem& page,
    bool is_suggested) {
  OfflineItem item;
  item.id = ContentId(kOfflinePageNamespace, page.client_id.id);
  item.title = base::UTF16ToUTF8(page.title);
  item.filter = OfflineItemFilter::FILTER_PAGE;
  item.state = OfflineItemState::COMPLETE;
  item.total_size_bytes = page.file_size;
  item.creation_time = page.creation_time;
  item.last_accessed_time = page.last_access_time;
  item.file_path = page.file_path;
  item.mime_type = "text/html";
  item.page_url = page.url;
  item.original_url = page.original_url;
  item.progress.value = 100;
  item.progress.max = 100;
  item.progress.unit = OfflineItemProgressUnit::PERCENTAGE;
  item.is_suggested = is_suggested;

  return item;
}

OfflineItem OfflineItemConversions::CreateOfflineItem(
    const SavePageRequest& request) {
  OfflineItem item;
  item.id = ContentId(kOfflinePageNamespace, request.client_id().id);
  item.filter = OfflineItemFilter::FILTER_PAGE;
  item.creation_time = request.creation_time();
  item.total_size_bytes = -1L;
  item.received_bytes = 0;
  item.mime_type = "text/html";
  item.page_url = request.url();
  item.original_url = request.original_url();
  switch (request.request_state()) {
    case SavePageRequest::RequestState::AVAILABLE:
      item.state = OfflineItemState::PENDING;
      break;
    case SavePageRequest::RequestState::OFFLINING:
      item.state = OfflineItemState::IN_PROGRESS;
      break;
    case SavePageRequest::RequestState::PAUSED:
      item.state = OfflineItemState::PAUSED;
      break;
    default:
      NOTREACHED();
  }

  item.progress.value = 0;
  item.progress.unit = OfflineItemProgressUnit::PERCENTAGE;

  return item;
}

}  // namespace offline_pages
