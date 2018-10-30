// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/input/scrollbar_animation_controller.h"

#include <algorithm>

#include "base/time/time.h"
#include "cc/trees/layer_tree_impl.h"

namespace cc {

std::unique_ptr<ScrollbarAnimationController>
ScrollbarAnimationController::CreateScrollbarAnimationControllerAndroid(
    ElementId scroll_element_id,
    ScrollbarAnimationControllerClient* client,
    base::TimeDelta fade_delay,
    base::TimeDelta fade_duration,
    float initial_opacity) {
  return base::WrapUnique(new ScrollbarAnimationController(
      scroll_element_id, client, fade_delay, fade_duration, initial_opacity));
}

std::unique_ptr<ScrollbarAnimationController>
ScrollbarAnimationController::CreateScrollbarAnimationControllerAuraOverlay(
    ElementId scroll_element_id,
    ScrollbarAnimationControllerClient* client,
    base::TimeDelta fade_delay,
    base::TimeDelta fade_duration,
    base::TimeDelta thinning_duration,
    float initial_opacity) {
  return base::WrapUnique(new ScrollbarAnimationController(
      scroll_element_id, client, fade_delay, fade_duration, thinning_duration,
      initial_opacity));
}

ScrollbarAnimationController::ScrollbarAnimationController(
    ElementId scroll_element_id,
    ScrollbarAnimationControllerClient* client,
    base::TimeDelta fade_delay,
    base::TimeDelta fade_duration,
    float initial_opacity)
    : client_(client),
      fade_delay_(fade_delay),
      fade_duration_(fade_duration),
      need_trigger_scrollbar_fade_in_(false),
      is_animating_(false),
      animation_change_(NONE),
      scroll_element_id_(scroll_element_id),
      currently_scrolling_(false),
      show_in_fast_scroll_(false),
      opacity_(initial_opacity),
      show_scrollbars_on_scroll_gesture_(false),
      need_thinning_animation_(false),
      is_mouse_down_(false),
      weak_factory_(this) {}

ScrollbarAnimationController::ScrollbarAnimationController(
    ElementId scroll_element_id,
    ScrollbarAnimationControllerClient* client,
    base::TimeDelta fade_delay,
    base::TimeDelta fade_duration,
    base::TimeDelta thinning_duration,
    float initial_opacity)
    : client_(client),
      fade_delay_(fade_delay),
      fade_duration_(fade_duration),
      need_trigger_scrollbar_fade_in_(false),
      is_animating_(false),
      animation_change_(NONE),
      scroll_element_id_(scroll_element_id),
      currently_scrolling_(false),
      show_in_fast_scroll_(false),
      opacity_(initial_opacity),
      show_scrollbars_on_scroll_gesture_(true),
      need_thinning_animation_(true),
      is_mouse_down_(false),
      weak_factory_(this) {
  vertical_controller_ = SingleScrollbarAnimationControllerThinning::Create(
      scroll_element_id, ScrollbarOrientation::VERTICAL, client,
      thinning_duration);
  horizontal_controller_ = SingleScrollbarAnimationControllerThinning::Create(
      scroll_element_id, ScrollbarOrientation::HORIZONTAL, client,
      thinning_duration);
}

ScrollbarAnimationController::~ScrollbarAnimationController() {}

ScrollbarSet ScrollbarAnimationController::Scrollbars() const {
  return client_->ScrollbarsFor(scroll_element_id_);
}

SingleScrollbarAnimationControllerThinning&
ScrollbarAnimationController::GetScrollbarAnimationController(
    ScrollbarOrientation orientation) const {
  DCHECK(need_thinning_animation_);
  if (orientation == ScrollbarOrientation::VERTICAL)
    return *(vertical_controller_.get());
  else
    return *(horizontal_controller_.get());
}

void ScrollbarAnimationController::StartAnimation() {
  DCHECK(animation_change_ != NONE);
  delayed_scrollbar_animation_.Cancel();
  need_trigger_scrollbar_fade_in_ = false;
  is_animating_ = true;
  last_awaken_time_ = base::TimeTicks();
  client_->SetNeedsAnimateForScrollbarAnimation();
}

void ScrollbarAnimationController::StopAnimation() {
  delayed_scrollbar_animation_.Cancel();
  need_trigger_scrollbar_fade_in_ = false;
  is_animating_ = false;
  animation_change_ = NONE;
}

void ScrollbarAnimationController::PostDelayedAnimation(
    AnimationChange animation_change) {
  animation_change_ = animation_change;
  delayed_scrollbar_animation_.Cancel();
  delayed_scrollbar_animation_.Reset(
      base::Bind(&ScrollbarAnimationController::StartAnimation,
                 weak_factory_.GetWeakPtr()));
  client_->PostDelayedScrollbarAnimationTask(
      delayed_scrollbar_animation_.callback(), fade_delay_);
}

bool ScrollbarAnimationController::Animate(base::TimeTicks now) {
  bool animated = false;

  if (is_animating_) {
    DCHECK(animation_change_ != NONE);
    if (last_awaken_time_.is_null())
      last_awaken_time_ = now;

    float progress = AnimationProgressAtTime(now);
    RunAnimationFrame(progress);

    if (is_animating_)
      client_->SetNeedsAnimateForScrollbarAnimation();
    animated = true;
  }

  if (need_thinning_animation_) {
    animated |= vertical_controller_->Animate(now);
    animated |= horizontal_controller_->Animate(now);
  }

  return animated;
}

float ScrollbarAnimationController::AnimationProgressAtTime(
    base::TimeTicks now) {
  base::TimeDelta delta = now - last_awaken_time_;
  float progress = delta.InSecondsF() / fade_duration_.InSecondsF();
  return std::max(std::min(progress, 1.f), 0.f);
}

void ScrollbarAnimationController::RunAnimationFrame(float progress) {
  float opacity;

  DCHECK(animation_change_ != NONE);
  if (animation_change_ == FADE_IN) {
    opacity = std::max(progress, opacity_);
  } else {
    opacity = std::min(1.f - progress, opacity_);
  }

  ApplyOpacityToScrollbars(opacity);
#if defined(TIZEN_VD_NATIVE_SCROLLBARS)
  if (progress == 1.f || should_stop_animation_)
#else
  if (progress == 1.f)
#endif
    StopAnimation();
}

void ScrollbarAnimationController::DidScrollBegin() {
  currently_scrolling_ = true;
}

void ScrollbarAnimationController::DidScrollEnd() {
  bool has_scrolled = show_in_fast_scroll_;
  show_in_fast_scroll_ = false;

  currently_scrolling_ = false;

  // We don't fade out scrollbar if they need thinning animation and mouse is
  // near.
  if (need_thinning_animation_ && MouseIsNearAnyScrollbar())
    return;

  if (has_scrolled)
    PostDelayedAnimation(FADE_OUT);
}

void ScrollbarAnimationController::DidScrollUpdate() {
  if (need_thinning_animation_ && Captured())
    return;

  StopAnimation();

  Show();

  // As an optimization, we avoid spamming fade delay tasks during active fast
  // scrolls.  But if we're not within one, we need to post every scroll update.
  if (!currently_scrolling_) {
    // We don't fade out scrollbar if they need thinning animation and mouse is
    // near.
    if (!need_thinning_animation_ || !MouseIsNearAnyScrollbar())
      PostDelayedAnimation(FADE_OUT);
  } else {
    show_in_fast_scroll_ = true;
  }

  if (need_thinning_animation_) {
    vertical_controller_->UpdateThumbThicknessScale();
    horizontal_controller_->UpdateThumbThicknessScale();
  }
}

void ScrollbarAnimationController::WillUpdateScroll() {
  if (show_scrollbars_on_scroll_gesture_)
    DidScrollUpdate();
}

void ScrollbarAnimationController::DidRequestShowFromMainThread() {
  DidScrollUpdate();
}

void ScrollbarAnimationController::DidMouseDown() {
  if (!need_thinning_animation_)
    return;

  is_mouse_down_ = true;

  if (ScrollbarsHidden()) {
    if (need_trigger_scrollbar_fade_in_) {
      delayed_scrollbar_animation_.Cancel();
      need_trigger_scrollbar_fade_in_ = false;
    }
    return;
  }

  vertical_controller_->DidMouseDown();
  horizontal_controller_->DidMouseDown();
}

void ScrollbarAnimationController::DidMouseUp() {
  if (!need_thinning_animation_)
    return;

  is_mouse_down_ = false;

  if (!Captured()) {
    if (MouseIsNearAnyScrollbar() && ScrollbarsHidden()) {
      PostDelayedAnimation(FADE_IN);
      need_trigger_scrollbar_fade_in_ = true;
    }
    return;
  }

  vertical_controller_->DidMouseUp();
  horizontal_controller_->DidMouseUp();

  if (!MouseIsNearAnyScrollbar() && !ScrollbarsHidden())
    PostDelayedAnimation(FADE_OUT);
}

void ScrollbarAnimationController::DidMouseLeave() {
  if (!need_thinning_animation_)
    return;

  vertical_controller_->DidMouseLeave();
  horizontal_controller_->DidMouseLeave();

  delayed_scrollbar_animation_.Cancel();
  need_trigger_scrollbar_fade_in_ = false;

  if (ScrollbarsHidden() || Captured())
    return;

  PostDelayedAnimation(FADE_OUT);
}

void ScrollbarAnimationController::DidMouseMove(
    const gfx::PointF& device_viewport_point) {
  if (!need_thinning_animation_)
    return;

  bool need_trigger_scrollbar_fade_in_before = need_trigger_scrollbar_fade_in_;

  vertical_controller_->DidMouseMove(device_viewport_point);
  horizontal_controller_->DidMouseMove(device_viewport_point);

  if (Captured()) {
    DCHECK(!ScrollbarsHidden());
    return;
  }

  if (ScrollbarsHidden()) {
    // Do not fade in scrollbar when user interacting with the content below
    // scrollbar.
    if (is_mouse_down_)
      return;
    need_trigger_scrollbar_fade_in_ = MouseIsNearAnyScrollbar();
    if (need_trigger_scrollbar_fade_in_before !=
        need_trigger_scrollbar_fade_in_) {
      if (need_trigger_scrollbar_fade_in_) {
        PostDelayedAnimation(FADE_IN);
      } else {
        delayed_scrollbar_animation_.Cancel();
      }
    }
  } else {
    if (MouseIsNearAnyScrollbar()) {
      Show();
      StopAnimation();
    } else if (!is_animating_) {
      PostDelayedAnimation(FADE_OUT);
    }
  }
}

bool ScrollbarAnimationController::MouseIsOverScrollbarThumb(
    ScrollbarOrientation orientation) const {
  DCHECK(need_thinning_animation_);
  return GetScrollbarAnimationController(orientation)
      .mouse_is_over_scrollbar_thumb();
}

bool ScrollbarAnimationController::MouseIsNearScrollbarThumb(
    ScrollbarOrientation orientation) const {
  DCHECK(need_thinning_animation_);
  return GetScrollbarAnimationController(orientation)
      .mouse_is_near_scrollbar_thumb();
}

bool ScrollbarAnimationController::MouseIsNearScrollbar(
    ScrollbarOrientation orientation) const {
  DCHECK(need_thinning_animation_);
  return GetScrollbarAnimationController(orientation)
      .mouse_is_near_scrollbar_track();
}

bool ScrollbarAnimationController::MouseIsNearAnyScrollbar() const {
  DCHECK(need_thinning_animation_);
  return vertical_controller_->mouse_is_near_scrollbar_track() ||
         horizontal_controller_->mouse_is_near_scrollbar_track();
}

bool ScrollbarAnimationController::ScrollbarsHidden() const {
  return opacity_ == 0.0f;
}

bool ScrollbarAnimationController::Captured() const {
  DCHECK(need_thinning_animation_);
  return GetScrollbarAnimationController(VERTICAL).captured() ||
         GetScrollbarAnimationController(HORIZONTAL).captured();
}

void ScrollbarAnimationController::Show() {
  delayed_scrollbar_animation_.Cancel();
  ApplyOpacityToScrollbars(1.0f);
}

#if defined(S_TERRACE_SUPPORT)
bool ScrollbarAnimationController::ShouldUseFastScrollbar(
    ScrollbarLayerImplBase* scrollbar) {
  bool fastScrollbarEnabled = false;
  bool big_sub_scrolling_layer = false;
  if (scrollbar->layer_tree_impl()) {
    fastScrollbarEnabled =
        scrollbar->layer_tree_impl()->get_fast_scroller_enabled();
    if (!fastScrollbarEnabled) {
        return false;
    }
    big_sub_scrolling_layer =
        scrollbar->layer_tree_impl()->CheckBigSubScrollingLayer();
    if (big_sub_scrolling_layer) {
        return false;
    }
    LayerImpl* outerScrollingLayer =
        scrollbar->layer_tree_impl()->OuterViewportScrollLayer();
    LayerImpl* innerScrollingLayer =
        scrollbar->layer_tree_impl()->InnerViewportScrollLayer();
    int outerScrollingLayerId = 0;
    int innerScrollingLayerId = 0;
    // FIXME: m60_3112 bringup, needs to be checked by author
    // https://codereview.chromium.org/2827163005
    // int scrollLayerId = scrollbar->ScrollLayerId();
    int scrollLayerId = -1;
    if (outerScrollingLayer) {
      outerScrollingLayerId = outerScrollingLayer->id();
    }
    if (innerScrollingLayer) {
      innerScrollingLayerId = innerScrollingLayer->id();
    }

    if (scrollLayerId == outerScrollingLayerId
        || scrollLayerId == innerScrollingLayerId) {
      fastScrollbarEnabled = true;
    } else {
      fastScrollbarEnabled = false;
    }
  }
  return fastScrollbarEnabled;
}
#endif

void ScrollbarAnimationController::ApplyOpacityToScrollbars(float opacity) {
  for (ScrollbarLayerImplBase* scrollbar : Scrollbars()) {
    DCHECK(scrollbar->is_overlay_scrollbar());
    float effective_opacity = scrollbar->CanScrollOrientation() ? opacity : 0;
#if defined(S_TERRACE_SUPPORT)
    if (ShouldUseFastScrollbar(scrollbar)
        && scrollbar->orientation() == VERTICAL) {
      effective_opacity = 0.001f;
    }
#endif
    scrollbar->SetOverlayScrollbarLayerOpacityAnimated(effective_opacity);
#if defined(TIZEN_VD_NATIVE_SCROLLBARS)
    if (scrollbar->ScrollCornerLayerId() != Layer::INVALID_ID)
      scrollbar->SetOverlayScrollbarLayerOpacityAnimated(effective_opacity);
#endif
  }

  bool previouslyVisible = opacity_ > 0.0f;
  bool currentlyVisible = opacity > 0.0f;

  if (opacity_ != opacity)
    client_->SetNeedsRedrawForScrollbarAnimation();

#if !defined(TIZEN_VD_NATIVE_SCROLLBARS)
  opacity_ = opacity;
#endif

  if (previouslyVisible != currentlyVisible)
    client_->DidChangeScrollbarVisibility();
}

#if defined(TIZEN_VD_NATIVE_SCROLLBARS)
void ScrollbarAnimationController::SetShouldStopAnimation(
    bool should_stop_animation) {
  should_stop_animation_ = should_stop_animation;
}
#endif
}  // namespace cc
