// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/web_contents_utils.h"

#include <string.h>

#include "browser_context_efl.h"
#include "content/browser/web_contents/web_contents_impl_efl.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "eweb_view.h"

using content::WebContents;
using content::RenderFrameHost;
using content::RenderViewHost;

namespace web_contents_utils {

WebContents* WebContentsFromViewID(int render_process_id, int render_view_id) {
  RenderViewHost* render_view_host =
      RenderViewHost::FromID(render_process_id, render_view_id);

  if (!render_view_host)
    return NULL;

  return WebContents::FromRenderViewHost(render_view_host);
}

WebContents* WebContentsFromFrameID(int render_process_id,
                                    int render_frame_id) {
  RenderFrameHost* render_frame_host =
      RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (!render_frame_host)
    return NULL;

  RenderViewHost* render_view_host = render_frame_host->GetRenderViewHost();

  if (!render_view_host)
    return NULL;

  return WebContents::FromRenderViewHost(render_view_host);
}

/* LCOV_EXCL_START */
EWebView* WebViewFromFrameId(int render_process_id, int render_frame_id) {
  content::WebContents* web_contents =
      web_contents_utils::WebContentsFromFrameID(render_process_id,
                                                 render_frame_id);
  if (!web_contents)
    return NULL;
  return WebViewFromWebContents(web_contents);
}

EWebView* WebViewFromViewId(int render_process_id, int render_view_id) {
  content::WebContents* web_contents =
      web_contents_utils::WebContentsFromViewID(render_process_id,
                                                render_view_id);
  if (!web_contents)
    return NULL;
  return WebViewFromWebContents(web_contents);
}
/* LCOV_EXCL_STOP */

EWebView* WebViewFromWebContents(const content::WebContents* web_contents) {
  CHECK(web_contents);
  const content::WebContentsImplEfl* web_contents_efl =
      static_cast<const content::WebContentsImplEfl*>(web_contents);
  EWebView* wv = static_cast<EWebView*>(web_contents_efl->GetPlatformData());
  // Each WebContents in EFL port should always have EWebView hosting it.
  CHECK(wv);
  return wv;
}

/* LCOV_EXCL_START */
bool MapsToHWBackKey(const char* keyname) {
  if (!keyname)
    return false;

  // Escape key is associated with HW Back Key. Everything else is not.
  // In order to protect against not "\0"-terminated strings, check first
  // 6 characters (lenght of "Escape").
  return strncmp(keyname, "Escape", 6) == 0;
}
/* LCOV_EXCL_STOP */

}  // namespace web_contents_utils
