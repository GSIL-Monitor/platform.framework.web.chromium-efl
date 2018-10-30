// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "eweb_view.h"
#include "scroll_detector.h"

ScrollDetector::ScrollDetector(EWebView* web_view)
    : web_view_(web_view),
      cached_is_main_frame_pinned_to_left_(false),
      cached_is_main_frame_pinned_to_right_(false),
      cached_is_main_frame_pinned_to_top_(false),
      cached_is_main_frame_pinned_to_bottom_(false),
      edge_Flag_(false),
      scroll_offset_changed_(false) {
}

/* LCOV_EXCL_START */
void ScrollDetector::OnChangeScrollOffset(const gfx::Vector2d& scroll_position) {
  edge_Flag_ = true;

  gfx::Vector2d scroll_delta = scroll_position;
  scroll_delta -= last_scroll_position_;
  last_scroll_position_ = scroll_position;

  DetectEdge(scroll_delta);

  scroll_offset_changed_ = false;
}

void ScrollDetector::DetectEdge(const gfx::Vector2d& scroll_delta) {
  if (scroll_delta.x()) {
    CheckForLeftRightEdges(last_scroll_position_.x());
  }

  if (scroll_delta.y()) {
    CheckForTopBottomEdges(last_scroll_position_.y());
  }
}

void ScrollDetector::SetMaxScroll(int x, int y) {
  max_Scroll_.set_x(x);
  max_Scroll_.set_y(y);
}

void ScrollDetector::CheckForLeftRightEdges(int scroll_x) {
  bool is_pinned_to_left = scroll_x <= min_Scroll_.x();
  bool is_pinned_to_right = scroll_x >= max_Scroll_.x();
  if (is_pinned_to_left && (is_pinned_to_left != cached_is_main_frame_pinned_to_left_)) {
    edge_Flag_ = false;
    web_view_->SmartCallback<EWebViewCallbacks::EdgeLeft>().call();
  }

  if (is_pinned_to_right && (is_pinned_to_right != cached_is_main_frame_pinned_to_right_)) {
    edge_Flag_ = false;
    web_view_->SmartCallback<EWebViewCallbacks::EdgeRight>().call();
  }

  cached_is_main_frame_pinned_to_left_ = is_pinned_to_left;
  cached_is_main_frame_pinned_to_right_ = is_pinned_to_right;
}

void ScrollDetector::CheckForTopBottomEdges(int scroll_y) {
  bool is_pinned_to_top = scroll_y <= min_Scroll_.y();
  bool is_pinned_to_bottom = scroll_y >= max_Scroll_.y();
  if (is_pinned_to_top && (is_pinned_to_top != cached_is_main_frame_pinned_to_top_)) {
    edge_Flag_ = false;
    web_view_->SmartCallback<EWebViewCallbacks::EdgeTop>().call();
  }

  if (is_pinned_to_bottom && (is_pinned_to_bottom != cached_is_main_frame_pinned_to_bottom_)) {
    edge_Flag_ = false;
    web_view_->SmartCallback<EWebViewCallbacks::EdgeBottom>().call();
  }
  cached_is_main_frame_pinned_to_top_ = is_pinned_to_top;
  cached_is_main_frame_pinned_to_bottom_ = is_pinned_to_bottom;
}
/* LCOV_EXCL_STOP */
