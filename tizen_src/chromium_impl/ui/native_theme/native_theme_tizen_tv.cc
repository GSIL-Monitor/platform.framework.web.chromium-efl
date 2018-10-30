// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/native_theme/native_theme_tizen_tv.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "cc/paint/paint_canvas.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/gfx/skia_util.h"
#include "ui/native_theme/common_theme.h"
#include "ui/native_theme/native_theme_features.h"
#include "ui/resources/grit/ui_resources_tizen.h"

namespace ui {

namespace {
const unsigned int kLocationVariation = 3;
const unsigned int kSizeVariation = 5;

const unsigned int kDefaultScrollbarWidth[] = {15, 54};
const unsigned int kDefaultScrollbarButtonLength[] = {0, 54};

#define VERTICAL_IMAGE_GRID(x) \
  { x##_TOP, x##_CENTER, x##_BOTTOM, }
#define HORIZONTAL_IMAGE_GRID(x) \
  { x##_LEFT, x##_CENTER, x##_RIGHT, }

const int kThinVerticalTrackImages[] =
    VERTICAL_IMAGE_GRID(IDR_TIZEN_THIN_SCROLLBAR_VERTICAL_TRACK);
const int kThinVerticalThumbImages[] =
    VERTICAL_IMAGE_GRID(IDR_TIZEN_THIN_SCROLLBAR_VERTICAL_THUMB);

const int kThinHorizontalTrackImages[] =
    HORIZONTAL_IMAGE_GRID(IDR_TIZEN_THIN_SCROLLBAR_HORIZONTAL_TRACK);
const int kThinHorizontalThumbImages[] =
    HORIZONTAL_IMAGE_GRID(IDR_TIZEN_THIN_SCROLLBAR_HORIZONTAL_THUMB);

const int kThickVerticalTrackNormalImages[] =
    VERTICAL_IMAGE_GRID(IDR_TIZEN_THICK_SCROLLBAR_VERTICAL_TRACK_NORMAL);
const int kThickVerticalThumbNormalImages[] =
    VERTICAL_IMAGE_GRID(IDR_TIZEN_THICK_SCROLLBAR_VERTICAL_THUMB_NORMAL);
const int kThickVerticalThumbPressedImages[] =
    VERTICAL_IMAGE_GRID(IDR_TIZEN_THICK_SCROLLBAR_VERTICAL_THUMB_PRESSED);

const int kThickHorizontalTrackNormalImages[] =
    HORIZONTAL_IMAGE_GRID(IDR_TIZEN_THICK_SCROLLBAR_HORIZONTAL_TRACK_NORMAL);
const int kThickHorizontalThumbNormalImages[] =
    HORIZONTAL_IMAGE_GRID(IDR_TIZEN_THICK_SCROLLBAR_HORIZONTAL_THUMB_NORMAL);
const int kThickHorizontalThumbPressedImages[] =
    HORIZONTAL_IMAGE_GRID(IDR_TIZEN_THICK_SCROLLBAR_HORIZONTAL_THUMB_PRESSED);

#undef VERTICAL_IMAGE_GRID
#undef HORIZONTAL_IMAGE_GRID

bool IsScrollbarPart(NativeTheme::Part part) {
  switch (part) {
    case NativeTheme::kScrollbarDownArrow:
    case NativeTheme::kScrollbarLeftArrow:
    case NativeTheme::kScrollbarRightArrow:
    case NativeTheme::kScrollbarUpArrow:
    case NativeTheme::kScrollbarHorizontalThumb:
    case NativeTheme::kScrollbarVerticalThumb:
    case NativeTheme::kScrollbarHorizontalTrack:
    case NativeTheme::kScrollbarVerticalTrack:
      return true;
    default:
      break;
  }
  return false;
}

NativeTheme::ScrollDirection GetScrollDirection(NativeTheme::Part part) {
  switch (part) {
    case NativeTheme::kScrollbarDownArrow:
      return NativeTheme::kScrollDown;
    case NativeTheme::kScrollbarLeftArrow:
      return NativeTheme::kScrollLeft;
    case NativeTheme::kScrollbarRightArrow:
      return NativeTheme::kScrollRight;
    case NativeTheme::kScrollbarUpArrow:
      return NativeTheme::kScrollUp;
    default:
      break;
  }
  return NativeTheme::kNumDirections;
}

const int GetArrowButtonImageIds(NativeTheme::State state) {
  switch (state) {
    case NativeTheme::kDisabled:
      return IDR_TIZEN_THICK_SCROLLBAR_ARROW_BUTTON_DISABLED;
    case NativeTheme::kNormal:
      return IDR_TIZEN_THICK_SCROLLBAR_ARROW_BUTTON_NORMAL;
    case NativeTheme::kHovered:
    case NativeTheme::kPressed:
      return IDR_TIZEN_THICK_SCROLLBAR_ARROW_BUTTON_HOVER;
    default:
      break;
  }
  return 0;
}

const gfx::ImageSkia GetArrowButtonImage(NativeTheme::ScrollDirection direction,
                                         NativeTheme::State state) {
  ui::ResourceBundle& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  const gfx::ImageSkia* base_image =
      resource_bundle.GetImageSkiaNamed(GetArrowButtonImageIds(state));
  if (!base_image)
    return gfx::ImageSkia();

  switch (direction) {
    case NativeTheme::kScrollUp: {
      return *base_image;
    }
    case NativeTheme::kScrollRight: {
      return gfx::ImageSkiaOperations::CreateRotatedImage(
          *base_image, SkBitmapOperations::ROTATION_90_CW);
    }
    case NativeTheme::kScrollDown: {
      return gfx::ImageSkiaOperations::CreateRotatedImage(
          *base_image, SkBitmapOperations::ROTATION_180_CW);
    }
    case NativeTheme::kScrollLeft: {
      return gfx::ImageSkiaOperations::CreateRotatedImage(
          *base_image, SkBitmapOperations::ROTATION_270_CW);
    }
    default:
      break;
  }
  return gfx::ImageSkia();
}

const int* GetPartImageIds(bool is_thin_scrollbar,
                           NativeTheme::Part part,
                           NativeTheme::State state,
                           size_t* num) {
  DCHECK(num);
  *num = 3;
  switch (state) {
    case NativeTheme::kDisabled:
    case NativeTheme::kNormal:
    case NativeTheme::kHovered: {
      switch (part) {
        case NativeTheme::kScrollbarVerticalTrack:
          return (is_thin_scrollbar ? kThinVerticalTrackImages
                                    : kThickVerticalTrackNormalImages);
        case NativeTheme::kScrollbarHorizontalTrack:
          return (is_thin_scrollbar ? kThinHorizontalTrackImages
                                    : kThickHorizontalTrackNormalImages);
        case NativeTheme::kScrollbarVerticalThumb:
          return (is_thin_scrollbar ? kThinVerticalThumbImages
                                    : kThickVerticalThumbNormalImages);
        case NativeTheme::kScrollbarHorizontalThumb:
          return (is_thin_scrollbar ? kThinHorizontalThumbImages
                                    : kThickHorizontalThumbNormalImages);
        default:
          break;
      }
      break;
    }
    case NativeTheme::kPressed: {
      switch (part) {
        case NativeTheme::kScrollbarVerticalTrack:
          return (is_thin_scrollbar ? kThinVerticalTrackImages
                                    : kThickVerticalTrackNormalImages);
        case NativeTheme::kScrollbarHorizontalTrack:
          return (is_thin_scrollbar ? kThinHorizontalTrackImages
                                    : kThickHorizontalTrackNormalImages);
        case NativeTheme::kScrollbarVerticalThumb:
          return (is_thin_scrollbar ? kThinVerticalThumbImages
                                    : kThickVerticalThumbPressedImages);
        case NativeTheme::kScrollbarHorizontalThumb:
          return (is_thin_scrollbar ? kThinHorizontalThumbImages
                                    : kThickHorizontalThumbPressedImages);
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  return nullptr;
}

std::vector<const gfx::ImageSkia*> GetPartImage(bool is_small,
                                                NativeTheme::Part part,
                                                NativeTheme::State state) {
  size_t num_ids = 0;
  const int* ids = GetPartImageIds(is_small, part, state, &num_ids);
  if (!ids)
    return std::vector<const gfx::ImageSkia*>();

  std::vector<const gfx::ImageSkia*> images;
  images.reserve(num_ids);
  ui::ResourceBundle& resource_bundle = ui::ResourceBundle::GetSharedInstance();
  for (size_t i = 0; i < num_ids; i++)
    images.push_back(resource_bundle.GetImageSkiaNamed(ids[i]));
  return images;
}

void PaintImagesVertically(gfx::Canvas* canvas,
                           const gfx::ImageSkia& top_image,
                           const gfx::ImageSkia& center_image,
                           const gfx::ImageSkia& bottom_image,
                           const gfx::Rect& rect) {
  canvas->DrawImageInt(top_image, 0, 0, top_image.width(), top_image.height(),
                       rect.x(), rect.y(), rect.width(), top_image.height(),
                       false);
  int y = rect.y() + top_image.height();
  const int center_height =
      rect.height() - top_image.height() - bottom_image.height();
  canvas->DrawImageInt(center_image, 0, 0, center_image.width(),
                       center_image.height(), rect.x(), y, rect.width(),
                       center_height, false);
  y += center_height;
  canvas->DrawImageInt(bottom_image, 0, 0, bottom_image.width(),
                       bottom_image.height(), rect.x(), y, rect.width(),
                       bottom_image.height(), false);
}

void PaintImagesHorizontally(gfx::Canvas* canvas,
                             const gfx::ImageSkia& left_image,
                             const gfx::ImageSkia& center_image,
                             const gfx::ImageSkia& right_image,
                             const gfx::Rect& rect) {
  canvas->DrawImageInt(left_image, 0, 0, left_image.width(),
                       left_image.height(), rect.x(), rect.y(),
                       left_image.width(), rect.height(), false);

  int x = rect.x() + left_image.width();
  const int center_width =
      rect.width() - left_image.width() - right_image.width();
  canvas->DrawImageInt(center_image, 0, 0, center_image.width(),
                       center_image.height(), x, rect.y(), center_width,
                       rect.height(), false);
  x += center_width;
  canvas->DrawImageInt(right_image, 0, 0, right_image.width(),
                       right_image.height(), x, rect.y(), right_image.width(),
                       rect.height(), false);
}

// Creates a gfx::Canvas wrapping an cc::PaintCanvas.
std::unique_ptr<gfx::Canvas> CommonThemeCreateCanvas(cc::PaintCanvas* canvas) {
  SkMatrix matrix = canvas->getTotalMatrix();
  float device_scale = static_cast<float>(SkScalarAbs(matrix.getScaleX()));
  return base::WrapUnique(new gfx::Canvas(canvas, device_scale));
}

}  // namespace

// static
NativeTheme* NativeTheme::GetInstanceForWeb() {
  if (IsNativeScrollbarEnabled())
    return NativeThemeTizenTV::web_instance();

  return NativeThemeAura::web_instance();
}

// static
NativeThemeTizenTV* NativeThemeTizenTV::web_instance() {
  CR_DEFINE_STATIC_LOCAL(NativeThemeTizenTV, s_native_theme_for_web, ());
  return &s_native_theme_for_web;
}

NativeThemeTizenTV::NativeThemeTizenTV()
    : NativeThemeAura(IsOverlayScrollbarEnabled()) {
  const State states[] = {
      State::kDisabled, State::kHovered, State::kNormal, State::kPressed,
  };

  const ScrollbarType types[] = {
      ScrollbarType::kThinScrollbar, ScrollbarType::kThickScrollbar,
  };

  const ScrollDirection directions[] = {
      ScrollDirection::kScrollUp, ScrollDirection::kScrollRight,
      ScrollDirection::kScrollDown, ScrollDirection::kScrollLeft,
  };

  for (size_t i = 0; i < arraysize(states); i++) {
    State state = states[i];
    for (size_t j = 0; j < arraysize(types); j++) {
      ScrollbarType type = types[j];
      bool is_thin_scrollbar = (type == kThinScrollbar);
      track_vertical_images_[type][state] =
          GetPartImage(is_thin_scrollbar, kScrollbarVerticalTrack, state);
      track_horizontal_images_[type][state] =
          GetPartImage(is_thin_scrollbar, kScrollbarHorizontalTrack, state);
      thumb_vertical_images_[type][state] =
          GetPartImage(is_thin_scrollbar, kScrollbarVerticalThumb, state);
      thumb_horizontal_images_[type][state] =
          GetPartImage(is_thin_scrollbar, kScrollbarHorizontalThumb, state);
    }

    for (size_t k = 0; k < arraysize(directions); k++) {
      ScrollDirection direction = directions[k];
      arrow_button_images_[direction][state] =
          GetArrowButtonImage(direction, state);
    }
  }
}

NativeThemeTizenTV::~NativeThemeTizenTV() {}

gfx::Size NativeThemeTizenTV::GetPartSize(Part part,
                                          State state,
                                          const ExtraParams& extra) const {
  if (!IsScrollbarPart(part) || !IsNativeScrollbarEnabled())
    return NativeThemeAura::GetPartSize(part, state, extra);

  ScrollbarType type =
      extra.scrollbar_type.is_thin_scrollbar ? kThinScrollbar : kThickScrollbar;
  int scrollbar_width = kDefaultScrollbarWidth[type];
  int scrollbar_button_length = kDefaultScrollbarButtonLength[type];
  switch (part) {
    case kScrollbarDownArrow:
    case kScrollbarUpArrow:
      return gfx::Size(scrollbar_width, scrollbar_button_length);
    case kScrollbarLeftArrow:
    case kScrollbarRightArrow:
      return gfx::Size(scrollbar_button_length, scrollbar_width);
    case kScrollbarHorizontalThumb:
      return gfx::Size(2 * scrollbar_width, scrollbar_width);
    case kScrollbarVerticalThumb:
      return gfx::Size(scrollbar_width, 2 * scrollbar_width);
    case kScrollbarHorizontalTrack:
      return gfx::Size(0, scrollbar_width);
    case kScrollbarVerticalTrack:
      return gfx::Size(scrollbar_width, 0);
    default:
      break;
  }
  return gfx::Size();
}

void NativeThemeTizenTV::Paint(cc::PaintCanvas* canvas,
                               Part part,
                               State state,
                               const gfx::Rect& rect,
                               const ExtraParams& extra) const {
  if (rect.IsEmpty())
    return;

  if (IsScrollbarPart(part) && IsNativeScrollbarEnabled()) {
    switch (part) {
      case kScrollbarDownArrow:
      case kScrollbarUpArrow:
      case kScrollbarLeftArrow:
      case kScrollbarRightArrow:
        PaintArrowButtonWithExtraParams(canvas, part, state, rect,
                                        extra.scrollbar_type);
        return;
      case kScrollbarHorizontalThumb:
      case kScrollbarVerticalThumb:
        PaintScrollbarThumbWithExtraParams(canvas, part, state, rect,
                                           extra.scrollbar_type);
        return;
      case kScrollbarHorizontalTrack:
      case kScrollbarVerticalTrack:
        PaintScrollbarTrackWithExtraParams(canvas, part, state, rect,
                                           extra.scrollbar_track,
                                           extra.scrollbar_type);
        return;
      default:
        break;
    }
  }
  NativeThemeBase::Paint(canvas, part, state, rect, extra);
}

void NativeThemeTizenTV::PaintArrowButtonBackgroundWithExtraParams(
    cc::PaintCanvas* paint_canvas,
    Part part,
    State state,
    const gfx::Rect& rect,
    const ScrollbarTypeExtraParams& extra) const {
  std::unique_ptr<gfx::Canvas> canvas(CommonThemeCreateCanvas(paint_canvas));
  ScrollbarType type =
      extra.is_thin_scrollbar ? kThinScrollbar : kThickScrollbar;
  switch (part) {
    case kScrollbarDownArrow:
    case kScrollbarUpArrow: {
      const std::vector<const gfx::ImageSkia*>& track_vertical_images =
          track_vertical_images_[type][state];
      if (part == kScrollbarDownArrow) {
        PaintImagesVertically(canvas.get(),
                              *track_vertical_images[kVerticalCenter],
                              *track_vertical_images[kVerticalCenter],
                              *track_vertical_images[kVerticalBottom], rect);
        break;
      }
      // kScrollbarUpArrow
      PaintImagesVertically(canvas.get(), *track_vertical_images[kVerticalTop],
                            *track_vertical_images[kVerticalCenter],
                            *track_vertical_images[kVerticalCenter], rect);
      break;
    }
    case kScrollbarLeftArrow:
    case kScrollbarRightArrow: {
      const std::vector<const gfx::ImageSkia*>& track_horizontal_images =
          track_horizontal_images_[type][state];
      if (part == kScrollbarLeftArrow) {
        PaintImagesHorizontally(
            canvas.get(), *track_horizontal_images[kHorizontalLeft],
            *track_horizontal_images[kHorizontalCenter],
            *track_horizontal_images[kHorizontalCenter], rect);
        break;
      }
      // kScrollbarRightArrow
      PaintImagesHorizontally(canvas.get(),
                              *track_horizontal_images[kHorizontalCenter],
                              *track_horizontal_images[kHorizontalCenter],
                              *track_horizontal_images[kHorizontalRight], rect);
      break;
    }
    default:
      break;
  }
}

void NativeThemeTizenTV::PaintArrowButtonWithExtraParams(
    cc::PaintCanvas* paint_canvas,
    Part part,
    State state,
    const gfx::Rect& rect,
    const ScrollbarTypeExtraParams& extra) const {
  PaintArrowButtonBackgroundWithExtraParams(paint_canvas, part, state, rect,
                                            extra);
  ScrollDirection direction = GetScrollDirection(part);
  if (direction == kNumDirections)
    return;

  const gfx::ImageSkia arrow_button_image =
      arrow_button_images_[direction][state];
  std::unique_ptr<gfx::Canvas> canvas(CommonThemeCreateCanvas(paint_canvas));
  canvas->DrawImageInt(arrow_button_image, 0, 0, arrow_button_image.width(),
                       arrow_button_image.height(), rect.x(), rect.y(),
                       rect.width(), rect.height(), false);
}

void NativeThemeTizenTV::PaintScrollbarTrackWithExtraParams(
    cc::PaintCanvas* paint_canvas,
    Part part,
    State state,
    const gfx::Rect& rect,
    const ScrollbarTrackExtraParams& extra_params,
    const ScrollbarTypeExtraParams& extra) const {
  ScrollbarType type =
      extra.is_thin_scrollbar ? kThinScrollbar : kThickScrollbar;
  std::unique_ptr<gfx::Canvas> canvas(CommonThemeCreateCanvas(paint_canvas));
  if (part == kScrollbarVerticalTrack) {
    const std::vector<const gfx::ImageSkia*>& track_vertical_images =
        track_vertical_images_[type][state];
    if (type == kThickScrollbar) {
      // The thickScrollbar has buttons.
      // Top and bottom track will be painted when painting arrow buttons.
      PaintImagesVertically(canvas.get(),
                            *track_vertical_images[kVerticalCenter],
                            *track_vertical_images[kVerticalCenter],
                            *track_vertical_images[kVerticalCenter], rect);
      return;
    }
    // kThinScrollbar
    PaintImagesVertically(canvas.get(), *track_vertical_images[kVerticalTop],
                          *track_vertical_images[kVerticalCenter],
                          *track_vertical_images[kVerticalBottom], rect);
    return;
  }
  // kScrollbarHorizontalTrack
  const std::vector<const gfx::ImageSkia*>& track_horizontal_images =
      track_horizontal_images_[type][state];
  if (type == kThickScrollbar) {
    // The thickScrollbar has buttons.
    // Left and right track will be painted when painting arrow buttons.
    PaintImagesHorizontally(canvas.get(),
                            *track_horizontal_images[kHorizontalCenter],
                            *track_horizontal_images[kHorizontalCenter],
                            *track_horizontal_images[kHorizontalCenter], rect);
    return;
  }
  // kThinScrollbar
  PaintImagesHorizontally(canvas.get(),
                          *track_horizontal_images[kHorizontalLeft],
                          *track_horizontal_images[kHorizontalCenter],
                          *track_horizontal_images[kHorizontalRight], rect);
}

void NativeThemeTizenTV::PaintScrollbarThumbWithExtraParams(
    cc::PaintCanvas* paint_canvas,
    Part part,
    State state,
    const gfx::Rect& rect,
    const ScrollbarTypeExtraParams& extra) const {
  ScrollbarType type =
      extra.is_thin_scrollbar ? kThinScrollbar : kThickScrollbar;
  gfx::Rect adjusted_rect =
      gfx::Rect(rect.x() + kLocationVariation, rect.y() + kLocationVariation,
                rect.width() - kSizeVariation, rect.height() - kSizeVariation);

  std::unique_ptr<gfx::Canvas> canvas(CommonThemeCreateCanvas(paint_canvas));
  if (part == kScrollbarVerticalThumb) {
    const std::vector<const gfx::ImageSkia*>& thumb_vertical_images =
        thumb_vertical_images_[type][state];
    PaintImagesVertically(canvas.get(), *thumb_vertical_images[kVerticalTop],
                          *thumb_vertical_images[kVerticalCenter],
                          *thumb_vertical_images[kVerticalBottom],
                          adjusted_rect);
    return;
  }
  // kScrollbarHorizontalThumb
  const std::vector<const gfx::ImageSkia*>& thumb_horizontal_images =
      thumb_horizontal_images_[type][state];
  PaintImagesHorizontally(
      canvas.get(), *thumb_horizontal_images[kHorizontalLeft],
      *thumb_horizontal_images[kHorizontalCenter],
      *thumb_horizontal_images[kHorizontalRight], adjusted_rect);
}

void NativeThemeTizenTV::PaintScrollbarCorner(cc::PaintCanvas* paint_canvas,
                                              State state,
                                              const gfx::Rect& rect) const {
  cc::PaintFlags paint;
  paint.setColor(SkColorSetRGB(0xFF, 0xFF, 0xFF));
  paint.setStyle(cc::PaintFlags::kFill_Style);
  paint.setBlendMode(SkBlendMode::kSrc);
  paint_canvas->drawIRect(RectToSkIRect(rect), paint);
}

}  // namespace ui
