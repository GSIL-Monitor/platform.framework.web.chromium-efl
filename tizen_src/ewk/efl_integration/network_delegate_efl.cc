// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "network_delegate_efl.h"

#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "common/web_contents_utils.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/web_contents.h"
#include "eweb_context.h"
#include "ewk/efl_integration/browser_context_efl.h"
#endif

namespace net {

NetworkDelegateEfl::NetworkDelegateEfl(
    base::WeakPtr<CookieManager> cookie_manager)
    : intercept_request_callback_with_data_{nullptr, nullptr, nullptr},
#if defined(OS_TIZEN_TV_PRODUCT)
      intercept_request_cancel_callback_with_data_{nullptr, nullptr, nullptr},
#endif
      cookie_manager_(cookie_manager) {}

/* LCOV_EXCL_START */
NetworkDelegate::AuthRequiredResponse NetworkDelegateEfl::OnAuthRequired(
    URLRequest* request,
    const AuthChallengeInfo& auth_info,
    const AuthCallback& callback,
    AuthCredentials* credentials) {
  return AUTH_REQUIRED_RESPONSE_NO_ACTION;
}
/* LCOV_EXCL_STOP */

bool NetworkDelegateEfl::OnCanGetCookies(const URLRequest& request,
                                         const CookieList& cookie_list) {
  if (!cookie_manager_.get())
    return false;
  return cookie_manager_->OnCanGetCookies(request, cookie_list);
}

/* LCOV_EXCL_START */
bool NetworkDelegateEfl::OnCanSetCookie(
    const URLRequest& request,
    const std::string& cookie_line,
    CookieOptions* options) {
  if (!cookie_manager_.get())
    return false;
  return cookie_manager_->OnCanSetCookie(request, cookie_line,
                                         options);
}
/* LCOV_EXCL_STOP */

bool NetworkDelegateEfl::OnCanAccessFile(
    const URLRequest& request,
    const base::FilePath& original_path,
    const base::FilePath& absolute_path) const {
#if defined(OS_TIZEN_TV_PRODUCT)
  int render_process_id = -1;
  int render_frame_id = -1;
  if (!content::ResourceRequestInfo::GetRenderFrameForRequest(
          &request, &render_process_id, &render_frame_id)) {
    return true;
  }

  content::WebContents* web_contents =
      web_contents_utils::WebContentsFromFrameID(render_process_id,
                                                 render_frame_id);
  if (!web_contents)
    return true;

  auto* browser_context = static_cast<content::BrowserContextEfl*>(
      web_contents->GetBrowserContext());
  if (!browser_context)
    return true;
  return browser_context->WebContext()->ShouldAllowRequest(request);
#else
  return true;
#endif
}
void NetworkDelegateEfl::SetInterceptRequestCallback(  // LCOV_EXCL_LINE
    Ewk_Context* ewk_context,
    Ewk_Context_Intercept_Request_Callback callback,
    void* user_data) {
  intercept_request_callback_with_data_ = {ewk_context, callback,
                                           user_data};  // LCOV_EXCL_LINE
}  // LCOV_EXCL_LINE

bool NetworkDelegateEfl::HasInterceptRequestCallback() const {
  return !!intercept_request_callback_with_data_.callback;
}

/* LCOV_EXCL_START */
void NetworkDelegateEfl::RunInterceptRequestCallback(
    _Ewk_Intercept_Request* intercept_request) const {
  if (intercept_request && intercept_request_callback_with_data_.callback)
    intercept_request_callback_with_data_.Run(intercept_request);
}
/* LCOV_EXCL_STOP */

#if defined(OS_TIZEN_TV_PRODUCT)
void NetworkDelegateEfl::SetInterceptRequestCancelCallback(
    Ewk_Context* ewk_context,
    Ewk_Context_Intercept_Request_Callback callback,
    void* user_data) {
  intercept_request_cancel_callback_with_data_ = {ewk_context, callback,
                                                  user_data};
}

bool NetworkDelegateEfl::HasInterceptRequestCancelCallback() const {
  return !!intercept_request_cancel_callback_with_data_.callback;
}

void NetworkDelegateEfl::RunInterceptRequestCancelCallback(
    _Ewk_Intercept_Request* intercept_request) const {
  if (intercept_request &&
      intercept_request_cancel_callback_with_data_.callback)
    intercept_request_cancel_callback_with_data_.Run(intercept_request);
}
#endif

};  // namespace net
