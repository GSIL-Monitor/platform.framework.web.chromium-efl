// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_TEST_API_H_
#define ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_TEST_API_H_

#include "ash/highlighter/highlighter_selection_observer.h"
#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {

class FastInkPoints;
class HighlighterController;

// An api for testing the HighlighterController class.
// Inheriting from HighlighterSelectionObserver to provide the tests
// with access to gesture recognition results.
class HighlighterControllerTestApi : public HighlighterSelectionObserver {
 public:
  explicit HighlighterControllerTestApi(HighlighterController* instance);
  ~HighlighterControllerTestApi() override;

  void CallMetalayerDone();
  void SetEnabled(bool enabled);
  void DestroyPointerView();
  void SimulateInterruptedStrokeTimeout();
  bool IsShowingHighlighter() const;
  bool IsFadingAway() const;
  bool IsWaitingToResumeStroke() const;
  bool IsShowingSelectionResult() const;
  const FastInkPoints& points() const;
  const FastInkPoints& predicted_points() const;

  void ResetEnabledState() { handle_enabled_state_changed_called_ = false; }
  bool handle_enabled_state_changed_called() const {
    return handle_enabled_state_changed_called_;
  }
  bool enabled() const { return enabled_; }

  void ResetSelection() {
    handle_selection_called_ = false;
    handle_failed_selection_called_ = false;
  }
  bool handle_selection_called() const { return handle_selection_called_; }
  bool handle_failed_selection_called() const {
    return handle_failed_selection_called_;
  }
  const gfx::Rect& selection() const { return selection_; }

 private:
  // HighlighterSelectionObserver:
  void HandleSelection(const gfx::Rect& rect) override;
  void HandleFailedSelection() override;
  void HandleEnabledStateChange(bool enabled) override;

  HighlighterController* instance_;

  bool handle_selection_called_ = false;
  bool handle_failed_selection_called_ = false;
  bool handle_enabled_state_changed_called_ = false;
  gfx::Rect selection_;
  bool enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(HighlighterControllerTestApi);
};

}  // namespace ash

#endif  // ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_TEST_API_H_