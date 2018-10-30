// Copyright (c) 2014-2015 Samsung Electronics. All rights reserved.
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "content/common/cursors/webcursor_efl.h"

#include "ecore_x_wayland_wrapper.h"

#include "base/logging.h"
#include "third_party/WebKit/public/platform/WebCursorInfo.h"

using namespace blink;

#if defined(USE_WAYLAND)
const char* GetCursorName(int type) {
  switch (type) {
#if defined(OS_TIZEN_TV_PRODUCT)
    case WebCursorInfo::kTypeTVDefault:
    case WebCursorInfo::kTypeTVPointer:
      return "normal_default";
    case WebCursorInfo::kTypeTVCross:
    case WebCursorInfo::kTypeTVHand:
      return "normal_pnh";
    case WebCursorInfo::kTypeTVIBeam:
      return "normal_input_field";
    case WebCursorInfo::kTypeTVNone:
      return "normal_transparent";
    case WebCursorInfo::kTypeWebDefault:
    case WebCursorInfo::kTypeWebPointer:
      return "normal_web_default";
    case WebCursorInfo::kTypeWebPNRPointer:
      return "normal_web_pnr";
    case WebCursorInfo::kTypeWebCross:
      return "normal_web_pnh";
    case WebCursorInfo::kTypeWebHand:
      return "normal_web_over";
    case WebCursorInfo::kTypeWebIBeam:
      return "normal_web_ibeam";
#endif
    case WebCursorInfo::kTypePointer:
      return "left_ptr";
    case WebCursorInfo::kTypeCross:
      return "left_ptr"; // ECORE_X_CURSOR_CROSS
    case WebCursorInfo::kTypeHand:
      return "hand1";
    case WebCursorInfo::kTypeIBeam:
      return "xterm";
    case WebCursorInfo::kTypeWait:
      return "watch";
    case WebCursorInfo::kTypeHelp:
      return "left_ptr"; // ECORE_X_CURSOR_QUESTION_ARROW
    case WebCursorInfo::kTypeEastResize:
      return "right_side";
    case WebCursorInfo::kTypeNorthResize:
      return "top_side";
    case WebCursorInfo::kTypeNorthEastResize:
      return "top_right_corner";
    case WebCursorInfo::kTypeNorthWestResize:
      return "top_left_corner";
    case WebCursorInfo::kTypeSouthResize:
      return "bottom_side";
    case WebCursorInfo::kTypeSouthEastResize:
      return "bottom_right_corner";
    case WebCursorInfo::kTypeSouthWestResize:
      return "bottom_left_corner";
    case WebCursorInfo::kTypeWestResize:
      return "left_side";
    case WebCursorInfo::kTypeNorthSouthResize:
      return "top_side"; // ECORE_X_CURSOR_SB_V_DOUBLE_ARROW
    case WebCursorInfo::kTypeEastWestResize:
      return "left_side"; // ECORE_X_CURSOR_SB_H_DOUBLE_ARROW
    case WebCursorInfo::kTypeNorthEastSouthWestResize:
      return "top_right_corner";
    case WebCursorInfo::kTypeNorthWestSouthEastResize:
      return "top_left_corner";
    case WebCursorInfo::kTypeColumnResize:
      return "left_side"; // ECORE_X_CURSOR_SB_H_DOUBLE_ARROW
    case WebCursorInfo::kTypeRowResize:
      return "top_side"; // ECORE_X_CURSOR_SB_V_DOUBLE_ARROW
    case WebCursorInfo::kTypeMiddlePanning:
      return "left_ptr"; // ECORE_X_CURSOR_FLEUR
    case WebCursorInfo::kTypeEastPanning:
      return "right_side"; // ECORE_X_CURSOR_SB_RIGHT_ARROW
    case WebCursorInfo::kTypeNorthPanning:
      return "top_side"; // ECORE_X_CURSOR_SB_UP_ARROW
    case WebCursorInfo::kTypeNorthEastPanning:
      return "top_right_corner"; // ECORE_X_CURSOR_TOP_RIGHT_CORNER
    case WebCursorInfo::kTypeNorthWestPanning:
      return "top_left_corner"; // ECORE_X_CURSOR_TOP_LEFT_CORNER
    case WebCursorInfo::kTypeSouthPanning:
      return "bottom_side"; // ECORE_X_CURSOR_SB_DOWN_ARROW
    case WebCursorInfo::kTypeSouthEastPanning:
      return "botton_right_corner"; // ECORE_X_CURSOR_BOTTOM_RIGHT_CORNER
    case WebCursorInfo::kTypeSouthWestPanning:
      return "botton_left_corner"; // ECORE_X_CURSOR_BOTTOM_LEFT_CORNER
    case WebCursorInfo::kTypeWestPanning:
      return "left_side"; //ECORE_X_CURSOR_SB_LEFT_ARROW
    case WebCursorInfo::kTypeMove:
      return "left_ptr"; //ECORE_X_CURSOR_FLEUR
    case WebCursorInfo::kTypeVerticalText:
      return "xterm";
    case WebCursorInfo::kTypeCell:
      return "xterm";
    case WebCursorInfo::kTypeContextMenu:
      return "xterm";
    case WebCursorInfo::kTypeAlias:
      return "xterm";
    case WebCursorInfo::kTypeProgress:
      return "watch";
    case WebCursorInfo::kTypeNoDrop:
      return "xterm";
    case WebCursorInfo::kTypeCopy:
      return "xterm";
    case WebCursorInfo::kTypeNone:
      return "left_ptr";
    case WebCursorInfo::kTypeNotAllowed:
      return "left_ptr";
    case WebCursorInfo::kTypeZoomIn:
      return "left_ptr";
    case WebCursorInfo::kTypeZoomOut:
      return "left_ptr";
    case WebCursorInfo::kTypeGrab:
    case WebCursorInfo::kTypeGrabbing:
      return "grabbing";
    case WebCursorInfo::kTypeCustom:
      return "left_ptr";
  }
  NOTREACHED();
  return "left_ptr";
}
#else
int GetCursorType(int type) {
  switch (type) {
    case WebCursorInfo::kTypePointer:
      return ECORE_X_CURSOR_ARROW;
    case WebCursorInfo::kTypeCross:
      return ECORE_X_CURSOR_CROSS;
    case WebCursorInfo::kTypeHand:
      return ECORE_X_CURSOR_HAND2;
    case WebCursorInfo::kTypeIBeam:
      return ECORE_X_CURSOR_XTERM;
    case WebCursorInfo::kTypeWait:
      return ECORE_X_CURSOR_WATCH;
    case WebCursorInfo::kTypeHelp:
      return ECORE_X_CURSOR_QUESTION_ARROW;
    case WebCursorInfo::kTypeEastResize:
      return ECORE_X_CURSOR_RIGHT_SIDE;
    case WebCursorInfo::kTypeNorthResize:
      return ECORE_X_CURSOR_TOP_SIDE;
    case WebCursorInfo::kTypeNorthEastResize:
      return ECORE_X_CURSOR_TOP_RIGHT_CORNER;
    case WebCursorInfo::kTypeNorthWestResize:
      return ECORE_X_CURSOR_TOP_LEFT_CORNER;
    case WebCursorInfo::kTypeSouthResize:
      return ECORE_X_CURSOR_BOTTOM_SIDE;
    case WebCursorInfo::kTypeSouthEastResize:
      return ECORE_X_CURSOR_BOTTOM_RIGHT_CORNER;
    case WebCursorInfo::kTypeSouthWestResize:
      return ECORE_X_CURSOR_BOTTOM_LEFT_CORNER;
    case WebCursorInfo::kTypeWestResize:
      return ECORE_X_CURSOR_LEFT_SIDE;
    case WebCursorInfo::kTypeNorthSouthResize:
      return ECORE_X_CURSOR_SB_V_DOUBLE_ARROW;
    case WebCursorInfo::kTypeEastWestResize:
      return ECORE_X_CURSOR_SB_H_DOUBLE_ARROW;
    case WebCursorInfo::kTypeNorthEastSouthWestResize:
    case WebCursorInfo::kTypeNorthWestSouthEastResize:
      // There isn't really a useful cursor available for these.
      return ECORE_X_CURSOR_XTERM;
    case WebCursorInfo::kTypeColumnResize:
      return ECORE_X_CURSOR_SB_H_DOUBLE_ARROW;  // TODO(evanm): is this correct?
    case WebCursorInfo::kTypeRowResize:
      return ECORE_X_CURSOR_SB_V_DOUBLE_ARROW;  // TODO(evanm): is this correct?
    case WebCursorInfo::kTypeMiddlePanning:
      return ECORE_X_CURSOR_FLEUR;
    case WebCursorInfo::kTypeEastPanning:
      return ECORE_X_CURSOR_SB_RIGHT_ARROW;
    case WebCursorInfo::kTypeNorthPanning:
      return ECORE_X_CURSOR_SB_UP_ARROW;
    case WebCursorInfo::kTypeNorthEastPanning:
      return ECORE_X_CURSOR_TOP_RIGHT_CORNER;
    case WebCursorInfo::kTypeNorthWestPanning:
      return ECORE_X_CURSOR_TOP_LEFT_CORNER;
    case WebCursorInfo::kTypeSouthPanning:
      return ECORE_X_CURSOR_SB_DOWN_ARROW;
    case WebCursorInfo::kTypeSouthEastPanning:
      return ECORE_X_CURSOR_BOTTOM_RIGHT_CORNER;
    case WebCursorInfo::kTypeSouthWestPanning:
      return ECORE_X_CURSOR_BOTTOM_LEFT_CORNER;
    case WebCursorInfo::kTypeWestPanning:
      return ECORE_X_CURSOR_SB_LEFT_ARROW;
    case WebCursorInfo::kTypeMove:
      return ECORE_X_CURSOR_FLEUR;
    case WebCursorInfo::kTypeVerticalText:
      return ECORE_X_CURSOR_XTERM;
    case WebCursorInfo::kTypeCell:
      return ECORE_X_CURSOR_XTERM;
    case WebCursorInfo::kTypeContextMenu:
      return ECORE_X_CURSOR_XTERM;
    case WebCursorInfo::kTypeAlias:
      return ECORE_X_CURSOR_XTERM;
    case WebCursorInfo::kTypeProgress:
      return ECORE_X_CURSOR_WATCH;
    case WebCursorInfo::kTypeNoDrop:
      return ECORE_X_CURSOR_XTERM;
    case WebCursorInfo::kTypeCopy:
      return ECORE_X_CURSOR_XTERM;
    case WebCursorInfo::kTypeNone:
      return ECORE_X_CURSOR_XTERM;
    case WebCursorInfo::kTypeNotAllowed:
      return ECORE_X_CURSOR_XTERM;
    case WebCursorInfo::kTypeZoomIn:
    case WebCursorInfo::kTypeZoomOut:
    case WebCursorInfo::kTypeGrab:
    case WebCursorInfo::kTypeGrabbing:
    case WebCursorInfo::kTypeCustom:
      return ECORE_X_CURSOR_XTERM; // Need to check the ECORE_X_CURSOR_XTERM
  }
  NOTREACHED();
  return ECORE_X_CURSOR_XTERM;
}
#endif // defined(USE_WAYLAND)
