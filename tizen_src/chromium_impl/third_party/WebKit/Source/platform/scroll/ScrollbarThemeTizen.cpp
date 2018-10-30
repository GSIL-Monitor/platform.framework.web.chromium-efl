// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromium_impl/third_party/WebKit/Source/platform/scroll/ScrollbarThemeTizen.h"

#include "platform/LayoutTestSupport.h"
#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/paint/DisplayItemCacheSkipper.h"
#include "platform/graphics/paint/DrawingRecorder.h"
#include "platform/runtime_enabled_features.h"
#include "platform/scroll/ScrollableArea.h"
#include "platform/scroll/Scrollbar.h"
#include "platform/scroll/ScrollbarThemeMock.h"
#include "platform/scroll/ScrollbarThemeOverlayMock.h"
#include "public/platform/Platform.h"
#include "public/platform/WebRect.h"
#include "public/platform/WebThemeEngine.h"

namespace blink {

namespace {

static bool UseMockTheme() {
  return LayoutTestSupport::IsRunningLayoutTest();
}

}  // namespace

// static
ScrollbarTheme& ScrollbarTheme::GetTheme() {
  if (ScrollbarTheme::MockScrollbarsEnabled()) {
    if (RuntimeEnabledFeatures::OverlayScrollbarsEnabled()) {
      DEFINE_STATIC_LOCAL(ScrollbarThemeOverlayMock, overlay_mock_theme, ());
      return overlay_mock_theme;
    }

    DEFINE_STATIC_LOCAL(ScrollbarThemeMock, mock_theme, ());
    return mock_theme;
  }

  if (ScrollbarTheme::NativeScrollbarsEnabled()) {
    DEFINE_STATIC_LOCAL(ScrollbarThemeTizen, native_theme, ());
    return native_theme;
  }

  return NativeTheme();
}

int ScrollbarThemeTizen::ScrollbarThickness(ScrollbarControlSize control_size) {
  IntSize scrollbar_size = Platform::Current()->ThemeEngine()->GetSize(
      WebThemeEngine::kPartScrollbarVerticalTrack,
      IsThinScrollbar(control_size));
  return scrollbar_size.Width();
}

int ScrollbarThemeTizen::MinimumThumbLength(
    const ScrollbarThemeClient& scrollbar) {
  if (scrollbar.Orientation() == kVerticalScrollbar) {
    IntSize size = Platform::Current()->ThemeEngine()->GetSize(
        WebThemeEngine::kPartScrollbarVerticalThumb,
        IsThinScrollbar(scrollbar.GetControlSize()));
    return size.Height();
  }

  // HorizontalScrollbar
  IntSize size = Platform::Current()->ThemeEngine()->GetSize(
      WebThemeEngine::kPartScrollbarHorizontalThumb,
      IsThinScrollbar(scrollbar.GetControlSize()));
  return size.Width();
}

void ScrollbarThemeTizen::PaintScrollCorner(
    GraphicsContext& context,
    const DisplayItemClient& display_item_client,
    const IntRect& corner_rect) {
  if (corner_rect.IsEmpty())
    return;

  DisplayItemCacheSkipper cacheSkipper(context);

  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, display_item_client, DisplayItem::kScrollbarCorner))
    return;

  DrawingRecorder recorder(context, display_item_client,
                           DisplayItem::kScrollbarCorner, corner_rect);
  Platform::Current()->ThemeEngine()->Paint(
      context.Canvas(), WebThemeEngine::kPartScrollbarCorner,
      WebThemeEngine::kStateNormal, WebRect(corner_rect), 0);
}

void ScrollbarThemeTizen::PaintTrackBackground(GraphicsContext& gc,
                                               const Scrollbar& scrollbar,
                                               const IntRect& rect) {
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          gc, scrollbar, DisplayItem::kScrollbarTrackBackground))
    return;

  DrawingRecorder recorder(gc, scrollbar,
                           DisplayItem::kScrollbarTrackBackground, rect);

  WebThemeEngine::State state = scrollbar.HoveredPart() == kTrackBGPart
                                    ? WebThemeEngine::kStateHover
                                    : WebThemeEngine::kStateNormal;

  if (UseMockTheme() && !scrollbar.Enabled())
    state = WebThemeEngine::kStateDisabled;

  WebThemeEngine::ExtraParams extra_params;
  extra_params.scrollbar_type.is_thin_scrollbar =
      IsThinScrollbar(scrollbar.GetControlSize());
  Platform::Current()->ThemeEngine()->Paint(
      gc.Canvas(),
      scrollbar.Orientation() == kHorizontalScrollbar
          ? WebThemeEngine::kPartScrollbarHorizontalTrack
          : WebThemeEngine::kPartScrollbarVerticalTrack,
      state, WebRect(rect), &extra_params);
}

void ScrollbarThemeTizen::PaintButton(GraphicsContext& gc,
                                      const Scrollbar& scrollbar,
                                      const IntRect& rect,
                                      ScrollbarPart part) {
  DisplayItem::Type display_item_type = ButtonPartToDisplayItemType(part);
  if (DrawingRecorder::UseCachedDrawingIfPossible(gc, scrollbar,
                                                  display_item_type))
    return;

  WebThemeEngine::Part paint_part;
  WebThemeEngine::State state = WebThemeEngine::kStateNormal;
  bool check_min = false;
  bool check_max = false;

  if (scrollbar.Orientation() == kHorizontalScrollbar) {
    if (part == kBackButtonStartPart) {
      paint_part = WebThemeEngine::kPartScrollbarLeftArrow;
      check_min = true;
    } else if (UseMockTheme() && part != kForwardButtonEndPart) {
      return;
    } else {
      paint_part = WebThemeEngine::kPartScrollbarRightArrow;
      check_max = true;
    }
  } else {
    if (part == kBackButtonStartPart) {
      paint_part = WebThemeEngine::kPartScrollbarUpArrow;
      check_min = true;
    } else if (UseMockTheme() && part != kForwardButtonEndPart) {
      return;
    } else {
      paint_part = WebThemeEngine::kPartScrollbarDownArrow;
      check_max = true;
    }
  }

  DrawingRecorder recorder(gc, scrollbar, display_item_type, rect);

  if (UseMockTheme() && !scrollbar.Enabled()) {
    state = WebThemeEngine::kStateDisabled;
  } else if (!UseMockTheme() &&
             ((check_min && (scrollbar.CurrentPos() <= 0)) ||
              (check_max && scrollbar.CurrentPos() >= scrollbar.Maximum()))) {
    state = WebThemeEngine::kStateDisabled;
  } else {
    if (part == scrollbar.PressedPart())
      state = WebThemeEngine::kStatePressed;
    else if (part == scrollbar.HoveredPart())
      state = WebThemeEngine::kStateHover;
  }

  WebThemeEngine::ExtraParams extra_params;
  extra_params.scrollbar_type.is_thin_scrollbar =
      IsThinScrollbar(scrollbar.GetControlSize());
  Platform::Current()->ThemeEngine()->Paint(gc.Canvas(), paint_part, state,
                                            WebRect(rect), &extra_params);
}

void ScrollbarThemeTizen::PaintThumb(GraphicsContext& gc,
                                     const Scrollbar& scrollbar,
                                     const IntRect& rect) {
  if (DrawingRecorder::UseCachedDrawingIfPossible(gc, scrollbar,
                                                  DisplayItem::kScrollbarThumb))
    return;

  DrawingRecorder recorder(gc, scrollbar, DisplayItem::kScrollbarThumb, rect);

  WebThemeEngine::State state;
  if (scrollbar.PressedPart() == kThumbPart)
    state = WebThemeEngine::kStatePressed;
  else if (scrollbar.HoveredPart() == kThumbPart)
    state = WebThemeEngine::kStateHover;
  else
    state = WebThemeEngine::kStateNormal;

  WebThemeEngine::ExtraParams extra_params;
  extra_params.scrollbar_type.is_thin_scrollbar =
      IsThinScrollbar(scrollbar.GetControlSize());
  Platform::Current()->ThemeEngine()->Paint(
      gc.Canvas(),
      scrollbar.Orientation() == kHorizontalScrollbar
          ? WebThemeEngine::kPartScrollbarHorizontalThumb
          : WebThemeEngine::kPartScrollbarVerticalThumb,
      state, WebRect(rect), &extra_params);
}

IntSize ScrollbarThemeTizen::ButtonSize(const ScrollbarThemeClient& scrollbar) {
  if (scrollbar.Orientation() == kVerticalScrollbar) {
    IntSize size = Platform::Current()->ThemeEngine()->GetSize(
        WebThemeEngine::kPartScrollbarUpArrow,
        IsThinScrollbar(scrollbar.GetControlSize()));
    return IntSize(size.Width(), scrollbar.Height() < 2 * size.Height()
                                     ? scrollbar.Height() / 2
                                     : size.Height());
  }

  // HorizontalScrollbar
  IntSize size = Platform::Current()->ThemeEngine()->GetSize(
      WebThemeEngine::kPartScrollbarLeftArrow,
      IsThinScrollbar(scrollbar.GetControlSize()));
  return IntSize(scrollbar.Width() < 2 * size.Width() ? scrollbar.Width() / 2
                                                      : size.Width(),
                 size.Height());
}

IntRect ScrollbarThemeTizen::CornerRect(const ScrollbarThemeClient& scrollbar) {
  if (scrollbar.Orientation() == kHorizontalScrollbar)
    return IntRect();

  int thickness = ScrollbarThickness(scrollbar.GetControlSize());
  return IntRect(scrollbar.X(), scrollbar.Y() + scrollbar.Height(), thickness,
                 scrollbar.CornerHeight());
}
}  // namespace blink
