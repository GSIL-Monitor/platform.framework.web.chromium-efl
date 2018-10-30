// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_notification_private.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/codec/png_codec.h"

using content::BrowserThread;
using namespace blink::mojom;

Ewk_Notification::Ewk_Notification(const std::string& body,
                                   const std::string& title,
                                   const SkBitmap& icon,
                                   bool silent,
                                   uint64_t notificationID,
                                   const GURL& origin)
    : body_(body),
      icon_(icon),
      title_(title),
      silent_(silent),
      notificationID_(notificationID),
      origin_(new _Ewk_Security_Origin(origin)) {}

Ewk_Notification::~Ewk_Notification() {}

const char* Ewk_Notification::GetBody() const {
  return body_.c_str();
}

const char* Ewk_Notification::GetTitle() const {
  return title_.c_str();
}

bool Ewk_Notification::IsSilent() const {
  return silent_;
}

Evas_Object* Ewk_Notification::GetIcon(Evas* evas) const {
  if (icon_.isNull()) {
    return nullptr;
  }

  Evas_Object* icon = evas_object_image_filled_add(evas);
  evas_object_image_size_set(icon, icon_.width(), icon_.height());
  evas_object_image_colorspace_set(icon, EVAS_COLORSPACE_ARGB8888);
  evas_object_image_alpha_set(icon, EINA_TRUE);
  evas_object_image_data_copy_set(icon, icon_.getPixels());

  return icon;
}

int Ewk_Notification::GetID() const {
  return notificationID_;
}

const _Ewk_Security_Origin* Ewk_Notification::GetSecurityOrigin() const {
  return origin_.get();
}

bool Ewk_Notification::SaveAsPng(const char* path) const {
  std::vector<unsigned char> png_data;
  if (gfx::PNGCodec::EncodeBGRASkBitmap(icon_, false, &png_data)) {
    if (base::WriteFile(base::FilePath(path),
                        reinterpret_cast<char*>(png_data.data()),
                        png_data.size()) != -1)
      return true;
  }
  return false;
}

Ewk_Notification_Permission_Request::Ewk_Notification_Permission_Request(
    const base::Callback<void(PermissionStatus)>& callback,
    const GURL& source_origin)
    : origin_(new _Ewk_Security_Origin(source_origin)), callback_(callback) {}

Ewk_Notification_Permission_Request::~Ewk_Notification_Permission_Request() {}

const _Ewk_Security_Origin*
Ewk_Notification_Permission_Request::GetSecurityOrigin() const {
  return origin_.get();
}

void Ewk_Notification_Permission_Request::RunCallback(bool allowed) {
  callback_.Run(allowed ? PermissionStatus::GRANTED : PermissionStatus::DENIED);
}
