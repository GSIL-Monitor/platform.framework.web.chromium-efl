// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _SCROLL_DETECTOR_H
#define _SCROLL_DETECTOR_H

#include "ui/gfx/geometry/rect.h"

class EWebView;

class ScrollDetector {
 public:
  ScrollDetector(EWebView* web_view);
  void OnChangeScrollOffset(const gfx::Vector2d& scroll_position);
  void SetMaxScroll(int x, int y);
  gfx::Vector2d GetLastScrollPosition() const {
    return last_scroll_position_; // LCOV_EXCL_LINE
  }
  bool IsScrollOffsetChanged() const { return scroll_offset_changed_; }
  void SetScrollOffsetChanged() {
    scroll_offset_changed_ = true; // LCOV_EXCL_LINE
  }

 private:
  void DetectEdge(const gfx::Vector2d& scroll_delta);
  void CheckForLeftRightEdges(int scroll_x);
  void CheckForTopBottomEdges(int sroll_y);

  EWebView* web_view_;
  gfx::Vector2d last_scroll_position_;
  gfx::Vector2d max_Scroll_;
  gfx::Vector2d min_Scroll_;
  bool cached_is_main_frame_pinned_to_left_;
  bool cached_is_main_frame_pinned_to_right_;
  bool cached_is_main_frame_pinned_to_top_;
  bool cached_is_main_frame_pinned_to_bottom_;
  bool edge_Flag_;
  bool scroll_offset_changed_;
};

#endif //_SCROLL_DETECTOR_H
