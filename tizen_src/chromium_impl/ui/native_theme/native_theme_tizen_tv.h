// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_NATIVE_THEME_NATIVE_THEME_TIZEN_H_
#define UI_NATIVE_THEME_NATIVE_THEME_TIZEN_H_

#include "ui/gfx/image/image_skia.h"
#include "ui/native_theme/native_theme_aura.h"

namespace ui {

// Tizen implementation of native theme support.
class NATIVE_THEME_EXPORT NativeThemeTizenTV : public NativeThemeAura {
 public:
  static NativeThemeTizenTV* web_instance();

 protected:
  NativeThemeTizenTV();
  ~NativeThemeTizenTV() override;

  // NativeTheme implementation:
  gfx::Size GetPartSize(Part, State, const ExtraParams&) const override;

  void Paint(cc::PaintCanvas*,
             Part,
             State,
             const gfx::Rect&,
             const ExtraParams&) const override;

  // This functions is implemented only for NativeThemeTizenTV.
  void PaintArrowButtonBackgroundWithExtraParams(
      cc::PaintCanvas*,
      Part,
      State,
      const gfx::Rect&,
      const ScrollbarTypeExtraParams&) const;

  void PaintArrowButtonWithExtraParams(cc::PaintCanvas*,
                                       Part,
                                       State,
                                       const gfx::Rect&,
                                       const ScrollbarTypeExtraParams&) const;

  void PaintScrollbarTrackWithExtraParams(
      cc::PaintCanvas*,
      Part,
      State,
      const gfx::Rect&,
      const ScrollbarTrackExtraParams&,
      const ScrollbarTypeExtraParams&) const;

  void PaintScrollbarThumbWithExtraParams(
      cc::PaintCanvas*,
      Part,
      State,
      const gfx::Rect&,
      const ScrollbarTypeExtraParams&) const;

  void PaintScrollbarCorner(cc::PaintCanvas*,
                            State,
                            const gfx::Rect&) const override;

 private:
  enum VerticalPart {
    kVerticalTop = 0,
    kVerticalCenter = 1,
    kVerticalBottom = 2
  };

  enum HorizontalPart {
    kHorizontalLeft = 0,
    kHorizontalCenter = 1,
    kHorizontalRight = 2
  };

  gfx::ImageSkia arrow_button_images_[kNumDirections][kNumStates];
  std::vector<const gfx::ImageSkia*> track_vertical_images_[kNumScrollbarTypes]
                                                           [kNumStates];
  std::vector<const gfx::ImageSkia*>
      track_horizontal_images_[kNumScrollbarTypes][kNumStates];
  std::vector<const gfx::ImageSkia*> thumb_vertical_images_[kNumScrollbarTypes]
                                                           [kNumStates];
  std::vector<const gfx::ImageSkia*>
      thumb_horizontal_images_[kNumScrollbarTypes][kNumStates];

  DISALLOW_COPY_AND_ASSIGN(NativeThemeTizenTV);
};

}  // namespace ui
#endif  // UI_NATIVE_THEME_NATIVE_THEME_TIZEN_H_
