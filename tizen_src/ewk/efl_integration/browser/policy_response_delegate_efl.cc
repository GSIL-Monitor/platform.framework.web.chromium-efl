// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/policy_response_delegate_efl.h"

#include "browser/mime_override_manager_efl.h"
#include "browser/resource_throttle_efl.h"
#include "common/web_contents_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"
#include "eweb_view.h"
#include "private/ewk_policy_decision_private.h"

using content::BrowserThread;
using content::RenderViewHost;
using content::RenderFrameHost;
using content::ResourceDispatcherHost;
using content::ResourceRequestInfo;
using content::WebContents;
using content::ResourceController;

using web_contents_utils::WebContentsFromFrameID;
using web_contents_utils::WebContentsFromViewID;

using web_contents_utils::WebViewFromWebContents;

PolicyResponseDelegateEfl::PolicyResponseDelegateEfl(
    net::URLRequest* request,
    ResourceThrottleEfl* throttle)
    : policy_decision_(new _Ewk_Policy_Decision(request->url(),
                                               request,
                                               this)),
      throttle_(throttle),
      render_process_id_(0),
      render_frame_id_(0),
      render_view_id_(0) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  const ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(request);

  if (info) {
    info->GetAssociatedRenderFrame(&render_process_id_, &render_frame_id_);
    render_view_id_ = info->GetRouteID();

  } else {
    ResourceRequestInfo::GetRenderFrameForRequest(request, &render_process_id_, &render_frame_id_);
  }

  // Chromium internal downloads are not associated with any frame or view, we should
  // accept them without EWK-specific logic. For example notification icon is internal
  // chromium download
  if (render_process_id_ > 0 && (render_frame_id_ > 0 || render_view_id_ > 0)) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
        base::Bind(&PolicyResponseDelegateEfl::HandlePolicyResponseOnUIThread, this));
  } else {
    // Async call required!
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
        base::Bind(&PolicyResponseDelegateEfl::UseResponseOnIOThread, this));
  }
}

void PolicyResponseDelegateEfl::HandlePolicyResponseOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(policy_decision_.get());

  WebContents* web_contents = NULL;

  DCHECK(render_process_id_ > 0);
  DCHECK(render_frame_id_ > 0 || render_view_id_ > 0);

  if (render_frame_id_ > 0) {
    web_contents = WebContentsFromFrameID(
      render_process_id_, render_frame_id_);
  } else {
    web_contents = WebContentsFromViewID(render_process_id_, render_view_id_);
  }

  if (!web_contents) {
    // this is a situation where we had frame/view info on IO thread but it
    // does not exist now in UI. We'll ignore such responses
    IgnoreResponse();
    return;
  }

  content::BrowserContextEfl* browser_context =
      static_cast<content::BrowserContextEfl*>(
          web_contents->GetBrowserContext());
  if (browser_context) {
    std::string mime_type;
    const char* mime_type_chars = policy_decision_->GetResponseMime();
    if (mime_type_chars)
      mime_type.assign(mime_type_chars);

    std::string url_spec;
    const char* url_spec_chars = policy_decision_->GetUrl();
    if (url_spec_chars)
      url_spec.assign(url_spec_chars);

    std::string new_mime_type;
    if (browser_context->WebContext()->OverrideMimeForURL(
        url_spec, mime_type, new_mime_type)) {
      MimeOverrideManagerEfl* mime_override_manager_efl =
          MimeOverrideManagerEfl::GetInstance();
      mime_override_manager_efl->PushOverriddenMime(url_spec, new_mime_type);
      // We have to set overridden mime type for policy_decision_, because
      // "policy,response,decide" callback would receive old mime type.
      policy_decision_->SetResponseMime(new_mime_type);
    }
  }

  // Delegate may be retrieved ONLY on UI thread
  policy_decision_->InitializeOnUIThread();

  // web_view_ takes owenership of Ewk_Policy_Decision. This is same as WK2/Tizen
  WebViewFromWebContents(web_contents)->
      InvokePolicyResponseCallback(policy_decision_.release());
}

void PolicyResponseDelegateEfl::UseResponse() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&PolicyResponseDelegateEfl::UseResponseOnIOThread, this));
}

/* LCOV_EXCL_START */
void PolicyResponseDelegateEfl::IgnoreResponse() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&PolicyResponseDelegateEfl::IgnoreResponseOnIOThread, this));
}
/* LCOV_EXCL_STOP */

void PolicyResponseDelegateEfl::UseResponseOnIOThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (throttle_) {
    throttle_->Resume();
    // decision is already taken so there is no need to use throttle anymore.
    throttle_ = NULL;
  }
}

/* LCOV_EXCL_START */
void PolicyResponseDelegateEfl::IgnoreResponseOnIOThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (throttle_) {
    throttle_->Ignore();
    throttle_ = NULL;

  }
}
/* LCOV_EXCL_STOP */

void PolicyResponseDelegateEfl::ThrottleDestroyed(){
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
   // The throttle has gone so don't try to do anything about it further.
  throttle_ = NULL;
}
