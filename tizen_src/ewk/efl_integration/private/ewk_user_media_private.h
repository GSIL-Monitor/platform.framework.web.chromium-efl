// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_user_media_private_h
#define ewk_user_media_private_h

#include <content/public/browser/browser_thread.h>

#include "base/callback.h"
#include "content/public/common/media_stream_request.h"
#include "private/ewk_security_origin_private.h"
#include "public/ewk_security_origin.h"
#include "web_contents_delegate_efl.h"

using content::BrowserThread;

/* LCOV_EXCL_START */
class _Ewk_User_Media_Permission_Request {
 public:
  _Ewk_User_Media_Permission_Request(
    content::WebContentsDelegateEfl* web_contents,
    const content::MediaStreamRequest& media_request,
    const content::MediaResponseCallback& callback);

  const _Ewk_Security_Origin* Origin() const { return origin_.get(); }
  content::WebContentsDelegateEfl* WebContents() const { return web_contents_; }
  void ProceedPermissionCallback(bool allowed);
  void Decide(bool allowed) { decided_ = allowed; }
  bool IsDecided() const { return decided_; }
  void Suspend() { suspended_ = true; };
  bool IsSuspended() const { return suspended_; }
  bool IsAudioRequested() const {
    return content::IsAudioInputMediaType(request_.audio_type);
  }
  bool IsVideoRequested() const {
    return content::IsVideoMediaType(request_.video_type);
  }

 private:
  content::WebContentsDelegateEfl* web_contents_;
  content::MediaStreamRequest request_;
  content::MediaResponseCallback callback_;

  std::unique_ptr<_Ewk_Security_Origin> origin_;
  bool decided_;
  bool suspended_;
};
/* LCOV_EXCL_STOP */

#endif // ewk_user_media_private_h
