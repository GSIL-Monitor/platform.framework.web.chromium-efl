// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SCROLL_CLIENT_OBSERVER_H
#define SCROLL_CLIENT_OBSERVER_H

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

class ScrollClientObserver
    : public content::WebContentsUserData<ScrollClientObserver>,
      public content::WebContentsObserver {
 public:
  ~ScrollClientObserver() override {}

  // content::WebContentsObserver implementation.
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  explicit ScrollClientObserver(content::WebContents*);
  friend class content::WebContentsUserData<ScrollClientObserver>;

  void OnDidEdgeScrollBy(gfx::Point, bool);
  void OnScrollbarThumbPartFocusChanged(int, bool);
  void OnRunArrowScroll();

  DISALLOW_COPY_AND_ASSIGN(ScrollClientObserver);
};

#endif
