// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef selection_box_efl_h
#define selection_box_efl_h

#include <Evas.h>

#include "content/public/common/context_menu_params.h"
#include "content/public/common/menu_item.h"
#include "ui/gfx/range/range.h"
#include "ui/gfx/geometry/rect.h"

class EWebView;

namespace content {

class RenderWidgetHostViewEfl;
// Hold the data related to drwaing the selection handlers
// and context menu. Also stores all the data required for selection
// controlling
class SelectionBoxEfl {
 public:
  SelectionBoxEfl(RenderWidgetHostViewEfl* rwhv);
  ~SelectionBoxEfl();

  void SetStatus(bool enable);
  bool GetStatus() const { return status_; }
  void SetEditable(bool enable) { GetContextMenuParams()->is_editable = enable; }
  bool GetEditable() const { return GetContextMenuParams()->is_editable; }
  void UpdateSelectStringData(const base::string16& text);
  // Returns true if the rectangle is changed.
  bool UpdateRectData(const gfx::Rect& left_rect, const gfx::Rect& right_rect);
  void ClearRectData();
  bool IsInEditField() const;
  gfx::Rect GetLeftRect() const { return left_rect_; }
  gfx::Rect GetRightRect() const { return right_rect_; }
  void SetIsAnchorFirst(bool value) { is_anchor_first_ = value; }
  bool GetIsAnchorFirst() const { return is_anchor_first_; }
  ContextMenuParams* GetContextMenuParams() const { return context_params_.get(); }

 private:
  // Save the state of selection, if active or not
  bool status_;

  // Start of selection
  gfx::Rect left_rect_;

  // End of selection
  gfx::Rect right_rect_;

  bool is_anchor_first_;

  // Contains the menu item data for which context needs to be populated
  std::unique_ptr<ContextMenuParams> context_params_;

  RenderWidgetHostViewEfl* rwhv_;
};

}
#endif
