// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/web_drag_source_efl.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/download/drag_download_util.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/paths_efl.h"
#include "content/public/common/drop_data.h"
#include "third_party/skia/include/core/SkPixelRef.h"
#include "ui/display/screen.h"

namespace content {

namespace {
const char* const kDefaultDragIcon = "broken_image.png";
const int kDefaultDragIconWidth = 105;
const int kDefaultDragIconHeight = 120;

blink::WebDragOperation GetWebOperationFromMask(
    blink::WebDragOperationsMask allowed_ops) {
  if (allowed_ops & blink::kWebDragOperationCopy)
    return blink::kWebDragOperationCopy;
  if (allowed_ops & blink::kWebDragOperationLink)
    return blink::kWebDragOperationLink;
  if (allowed_ops & blink::kWebDragOperationMove)
    return blink::kWebDragOperationMove;
  if (allowed_ops & blink::kWebDragOperationGeneric)
    return blink::kWebDragOperationGeneric;
  if (allowed_ops & blink::kWebDragOperationPrivate)
    return blink::kWebDragOperationPrivate;
  if (allowed_ops & blink::kWebDragOperationDelete)
    return blink::kWebDragOperationDelete;
  return blink::kWebDragOperationNone;
}

Evas_Object* DragIconCreateCb(void* data,
                              Evas_Object* win,
                              Evas_Coord* xoff,
                              Evas_Coord* yoff) {
  WebDragSourceEfl* web_drag_source_efl = static_cast<WebDragSourceEfl*>(data);
  return web_drag_source_efl->DragIconCreate(win, xoff, yoff);
}

void DragPosCb(void* data,
               Evas_Object* obj,
               Evas_Coord x,
               Evas_Coord y,
               Elm_Xdnd_Action action) {
  WebDragSourceEfl* web_drag_source_efl = static_cast<WebDragSourceEfl*>(data);
  web_drag_source_efl->DragPos(x, y, action);
}

void DragAcceptCb(void* data, Evas_Object* obj, Eina_Bool doaccept) {}

void DragStateCb(void* data, Evas_Object* obj) {
  WebDragSourceEfl* web_drag_source_efl = static_cast<WebDragSourceEfl*>(data);
  web_drag_source_efl->DragState();
}

}  // namespace

WebDragSourceEfl::WebDragSourceEfl(WebContents* web_contents)
    : parent_view_(static_cast<Evas_Object*>(web_contents->GetNativeView())),
      web_contents_(static_cast<WebContentsImpl*>(web_contents)),
      source_rwh_(nullptr),
      drag_failed_(false),
      drag_started_(false),
      drag_action_(blink::kWebDragOperationNone),
      page_scale_factor_(1.0f) {
  device_scale_factor_ =
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
}

WebDragSourceEfl::~WebDragSourceEfl() {}

bool WebDragSourceEfl::StartDragging(const DropData& drop_data,
                                     blink::WebDragOperationsMask allowed_ops,
                                     const gfx::Point& root_location,
                                     const SkBitmap& image,
                                     const gfx::Vector2d& image_offset,
                                     RenderWidgetHostImpl* source_rwh) {
  // Guard against re-starting before previous drag completed.
  if (drag_started_) {
    NOTREACHED();
    return false;
  }

  base::string16 elm_drag_data;
  int targets_mask = 0;
  initial_position_ = root_location;

  if (!drop_data.text.is_null() && !drop_data.text.string().empty()) {
    elm_drag_data = drop_data.text.string();
    targets_mask |= ELM_SEL_FORMAT_TEXT;
  }
  if (drop_data.url.is_valid()) {
    elm_drag_data = base::UTF8ToUTF16(drop_data.url.spec());
    targets_mask |= ELM_SEL_FORMAT_TEXT;
  }
  if (!drop_data.html.is_null() && !drop_data.html.string().empty()) {
    elm_drag_data = drop_data.html.string();
    targets_mask |= ELM_SEL_FORMAT_HTML;
  }
  if (!drop_data.file_contents.empty()) {
    elm_drag_data =
        base::UTF8ToUTF16(drop_data.file_contents_filename_extension);
    targets_mask |= ELM_SEL_FORMAT_IMAGE;
  }
  if (!drop_data.download_metadata.empty() &&
      ParseDownloadMetadata(drop_data.download_metadata,
                            &wide_download_mime_type_,
                            &download_file_name_,
                            &download_url_)) {
    elm_drag_data = drop_data.download_metadata;
    targets_mask |= ELM_SEL_FORMAT_IMAGE;
  }
  if (!drop_data.custom_data.empty()) {
    elm_drag_data = drop_data.custom_data.begin()->first;
    targets_mask |= ELM_SEL_FORMAT_NONE;
  }

  int action = 0;

  if (allowed_ops & blink::kWebDragOperationCopy)
    action |= ELM_XDND_ACTION_COPY;
  if (allowed_ops & blink::kWebDragOperationLink)
    action |= ELM_XDND_ACTION_LINK;
  if (allowed_ops & blink::kWebDragOperationMove)
    action |= ELM_XDND_ACTION_MOVE;
  if (!action)
    action = ELM_XDND_ACTION_UNKNOWN;

  image_ = image;
  image_offset_ = image_offset;
  drag_action_ = allowed_ops;
  drag_failed_ = false;
  source_rwh_ = source_rwh;

  drag_started_ = elm_drag_start(parent_view_,
                                 static_cast<Elm_Sel_Format>(targets_mask),
                                 base::UTF16ToUTF8(elm_drag_data).c_str(),
                                 static_cast<Elm_Xdnd_Action>(action),
                                 DragIconCreateCb, this,
                                 DragPosCb, this,
                                 DragAcceptCb, this,
                                 DragStateCb, this);

  // Sometimes the drag fails to start; |drag_started_| will be false and we won't
  // get a drag-end signal.
  if (!drag_started_) {
    LOG(WARNING) << "Failed to start drag and drop";
    drag_failed_ = true;
    return false;
  }

  return true;
}

Evas_Object* WebDragSourceEfl::DragIconCreate(Evas_Object* win,
                                              Evas_Coord* xoff,
                                              Evas_Coord* yoff) const {
  int w = image_.width(), h = image_.height();

  if (xoff)
    *xoff = initial_position_.x();
  if (yoff)
    *yoff = initial_position_.y();
  Evas* evas = evas_object_evas_get(win);
  Evas_Object* icon = evas_object_image_add(evas);
  if (w > 0 && h > 0) {
    evas_object_image_size_set(icon, w, h);
    evas_object_image_data_copy_set(icon, image_.pixelRef()->pixels());
  } else {
    w = kDefaultDragIconWidth;
    h = kDefaultDragIconHeight;
    base::FilePath path;
    PathService::Get(PathsEfl::IMAGE_RESOURCE_DIR, &path);
    path = path.Append(FILE_PATH_LITERAL(kDefaultDragIcon));
    evas_object_image_file_set(icon, path.AsUTF8Unsafe().c_str(), nullptr);
  }
  evas_object_image_fill_set(icon, 0, 0, w, h);
  evas_object_image_alpha_set(icon, EINA_TRUE);
  evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_move(icon, initial_position_.x(), initial_position_.y());
  evas_object_resize(icon, w, h);
  evas_object_show(icon);

  return icon;
}

void WebDragSourceEfl::DragPos(Evas_Coord x,
                               Evas_Coord y,
                               Elm_Xdnd_Action action) {
  gfx::Point screen_pt =
      gfx::Point(x / device_scale_factor_, y / device_scale_factor_);
  last_pointer_pos_ = screen_pt;
}

void WebDragSourceEfl::DragState() {
  gfx::Point client_pt = gfx::Point(last_pointer_pos_.x() / page_scale_factor_,
                                    last_pointer_pos_.y() / page_scale_factor_);

  web_contents_->DragSourceEndedAt(client_pt.x(), client_pt.y(),
                                   last_pointer_pos_.x(), last_pointer_pos_.y(),
                                   GetWebOperationFromMask(drag_action_),
                                   source_rwh_);

  web_contents_->SystemDragEnded(source_rwh_);
  drag_started_ = false;
}

}  // namespace content
