// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_SOURCE_EFL_H_
#define CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_SOURCE_EFL_H_

#include <Elementary.h>

#include "base/files/file_path.h"
#include "base/strings/string16.h"
#include "third_party/WebKit/public/platform/WebDragOperation.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/vector2d.h"
#include "url/gurl.h"

namespace content {

class RenderWidgetHostImpl;
class WebContents;
class WebContentsImpl;

struct DropData;

// WebDragSourceEfl takes care of managing the drag from a WebContents
// with Efl.
class WebDragSourceEfl {
 public:
  explicit WebDragSourceEfl(WebContents* web_contents);
  virtual ~WebDragSourceEfl();

  // Starts a drag for the WebContents this object was created for.
  // Returns false if the drag could not be started.
  bool StartDragging(const DropData& drop_data,
                     blink::WebDragOperationsMask allowed_ops,
                     const gfx::Point& root_location,
                     const SkBitmap& image,
                     const gfx::Vector2d& image_offset,
                     RenderWidgetHostImpl* source_rwh);

  void Reset() { drag_started_ = false; }
  void SetLastPoint(gfx::Point point) { last_point_ = point; }
  void SetPageScaleFactor(float scale) { page_scale_factor_ = scale; }
  bool IsDragging() { return drag_started_; }

  // EFL callbacks
  Evas_Object* DragIconCreate(Evas_Object* win,
                              Evas_Coord* xoff,
                              Evas_Coord* yoff) const;
  void DragPos(Evas_Coord x, Evas_Coord y, Elm_Xdnd_Action action);
  void DragState();

 private:
  Evas_Object* parent_view_;

  // The tab we're manging the drag for.
  WebContentsImpl* web_contents_;

  // Browser side IPC handler.
  RenderWidgetHostImpl* source_rwh_;

  // The image used for depicting the drag, and the offset between the cursor
  // and the top left pixel.
  SkBitmap image_;
  gfx::Vector2d image_offset_;

  // Whether the current drag has failed. Meaningless if we are not the source
  // for a current drag.
  bool drag_failed_;

  // True set once drag starts.  A false value indicates that there is
  // no drag currently in progress.
  bool drag_started_;

  // Last registered point from drag_pos_cb callback.
  gfx::Point last_point_;

  // Maks of all available drag actions linke copy, move or link
  blink::WebDragOperationsMask drag_action_;

  // The file mime type for a drag-out download.
  base::string16 wide_download_mime_type_;

  // The file name to be saved to for a drag-out download.
  base::FilePath download_file_name_;

  // The URL to download from for a drag-out download.
  GURL download_url_;

  float device_scale_factor_;
  float page_scale_factor_;

  gfx::Point initial_position_;
  gfx::Point last_pointer_pos_;

  DISALLOW_COPY_AND_ASSIGN(WebDragSourceEfl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_CONTENTS_WEB_DRAG_SOURCE_EFL_H_
