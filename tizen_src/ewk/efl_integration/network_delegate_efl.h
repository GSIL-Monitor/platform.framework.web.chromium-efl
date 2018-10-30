// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reseoved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _NETWORK_DELEGATE_EFL_H_
#define _NETWORK_DELEGATE_EFL_H_

#include "base/compiler_specific.h"
#include "cookie_manager.h"
#include "net/base/network_delegate_impl.h"
#include "public/ewk_context.h"

namespace net {

class NetworkDelegateEfl : public NetworkDelegateImpl {
 public:
  NetworkDelegateEfl(base::WeakPtr<CookieManager> cookie_manager);

  void SetInterceptRequestCallback(
      Ewk_Context* ewk_context,
      Ewk_Context_Intercept_Request_Callback callback,
      void* user_data);
  bool HasInterceptRequestCallback() const;
  void RunInterceptRequestCallback(
      _Ewk_Intercept_Request* intercept_request) const;

#if defined(OS_TIZEN_TV_PRODUCT)
  void SetInterceptRequestCancelCallback(
      Ewk_Context* ewk_context,
      Ewk_Context_Intercept_Request_Callback callback,
      void* user_data);
  bool HasInterceptRequestCancelCallback() const;
  void RunInterceptRequestCancelCallback(
      _Ewk_Intercept_Request* intercept_request) const;
  void NotifyInterceptRequestDestroy(
      _Ewk_Intercept_Request* intercept_request) const;
#endif

 private:
  // NetworkDelegate implementation.
  virtual AuthRequiredResponse OnAuthRequired(
      URLRequest* request,
      const AuthChallengeInfo& auth_info,
      const AuthCallback& callback,
      AuthCredentials* credentials) override;
  bool OnCanGetCookies(const URLRequest& request,
                       const CookieList& cookie_list) override;
  bool OnCanSetCookie(const URLRequest& request,
                      const std::string& cookie_line,
                      CookieOptions* options) override;
  bool OnCanAccessFile(const URLRequest& request,
                       const base::FilePath& original_path,
                       const base::FilePath& absolute_path) const override;

  struct InterceptRequestCallbackWithData {
    Ewk_Context* context;
    Ewk_Context_Intercept_Request_Callback callback;
    void* user_data;

    /* LCOV_EXCL_START */
    void Run(_Ewk_Intercept_Request* intercept_request) const {
      callback(context, intercept_request, user_data);
    }
    /* LCOV_EXCL_STOP */
  };

  InterceptRequestCallbackWithData intercept_request_callback_with_data_;

#if defined(OS_TIZEN_TV_PRODUCT)
  InterceptRequestCallbackWithData intercept_request_cancel_callback_with_data_;
#endif

  base::WeakPtr<CookieManager> cookie_manager_;
};

}  // namespace net

#endif  // _NETWORK_DELEGATE_EFL_H_
