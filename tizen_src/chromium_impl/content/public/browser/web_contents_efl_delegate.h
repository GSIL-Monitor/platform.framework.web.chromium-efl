// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_EFL_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_EFL_DELEGATE_H_

#include "base/callback.h"
#include "content/common/content_export.h"

class GURL;

namespace content {

class WebContents;

class CONTENT_EXPORT WebContentsEflDelegate {
 public:
  virtual ~WebContentsEflDelegate() {}

  // Callback function allowing the embedder to create new web contents object
  // in an asynchronous manner. The callback takes one opaque parameter
  // intended to pass platform specific data to WebContents constructor. It
  // returns a pointer to new WebContents created by the function. The caller
  // is expected to take ownership of the returned object.
  typedef base::Callback<WebContents*(void*)> WebContentsCreateCallback;

  // Callback allowing the embedder to resume or block new window request. The
  // argument specifies if the request should be allowed (true) or blocked
  // (false).
  typedef base::Callback<void(bool)> NewWindowDecideCallback;

  // Allows delegate to asynchronously control whether a WebContents should be
  // created. The function should return true if the implementation intends to
  // use this functionality. In such case the synchronous version of this
  // function ShouldCreateWebContents will not be called. The embedder must
  // call the callback function at some point.
  virtual bool ShouldCreateWebContentsAsync(NewWindowDecideCallback,
                                            const GURL&) = 0;

  // Allows the delegate to control new web contents creation in an
  // asynchronous manner. The function is called with a callback object the
  // delegate may keep if it wants to delay creation of new web contents. In
  // such case the function should return true. Returning false means new web
  // contents creation should be handled in normal, synchronous manner.
  virtual bool WebContentsCreateAsync(WebContentsCreateCallback) = 0;

  // Make it possible to early hook native_view and its associated smart
  // parent.
  // This is needed when a window is created with window.open or target:_blank,
  // where RenderWidgetHostViewXXX objects are created earlier in the call chain.
  virtual void SetUpSmartObject(void* platform_data, void* native_view) = 0;
};

} // namespace content

#endif // CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_EFL_DELEGATE_H_
