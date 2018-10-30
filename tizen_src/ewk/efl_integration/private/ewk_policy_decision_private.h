// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_policy_decision_private_h
#define ewk_policy_decision_private_h

#include <Eina.h>

#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "browser/navigation_policy_handler_efl.h"
#include "browser/policy_response_delegate_efl.h"
#include "content/public/browser/web_contents_efl_delegate.h"
#include "content/public/common/resource_type.h"
#include "public/ewk_policy_decision.h"
// FIXME: removed on m63
//#include "third_party/WebKit/public/web/WebViewModeEnums.h"

// Basic type of authorization - 'type username:password'
#define BASIC_AUTHORIZATION "BASIC"

namespace net {
class URLRequest;
}

class EWebView;
class GURL;
struct NavigationPolicyParams;
class PolicyResponseDelegateEfl;
class _Ewk_Frame;

namespace content {
class RenderViewHost;
} // namespace content

class _Ewk_Policy_Decision {
 public:
 /**
  * Constructs _Ewk_Policy_Decision with navigation type POLICY_RESPONSE
  */
  explicit _Ewk_Policy_Decision(const GURL& request_url,
                                net::URLRequest* request,
                                PolicyResponseDelegateEfl* delegate);

 /**
  * Constructs _Ewk_Policy_Decision with navigation type POLICY_NAVIGATION
  */
  explicit _Ewk_Policy_Decision(const NavigationPolicyParams& params);

  /**
   * Constructs _Ewk_Policy_Decision with navigation type POLICY_NEWWINDOW
   */
  explicit _Ewk_Policy_Decision(EWebView* view,
      content::WebContentsEflDelegate::NewWindowDecideCallback);

  ~_Ewk_Policy_Decision();

  void Use();
  void Ignore();
  bool Suspend();

  bool isDecided() const { return is_decided_; }
  bool isSuspended() const { return is_suspended_; }
  Ewk_Policy_Navigation_Type GetNavigationType() const { return navigation_type_; }
  const char* GetCookie() const;
  const char* GetAuthUser() const;
  const char* GetAuthPassword() const;
  const char* GetUrl() const;
  const char* GetHttpMethod() const;
  const char* GetHttpBody() const;
  const char* GetScheme() const;
  const char* GetHost() const;
  const char* GetResponseMime() const;
  void SetResponseMime(const std::string& response_mime);
  Ewk_Policy_Decision_Type GetDecisionType() const { return decision_type_; }
  Eina_Hash* GetResponseHeaders() const { return response_headers_; }
  int GetResponseStatusCode() const { return response_status_code_; }

  NavigationPolicyHandlerEfl* GetNavigationPolicyHandler() const { return navigation_policy_handler_.get(); }

  _Ewk_Frame* GetFrameRef() const;

  void InitializeOnUIThread();
  void ParseUrl(const GURL& url);

private:
  /**
   * @brief sets userID and password
   * @param request_url requested url - user:password@address
   */
  void SetAuthorizationIfNecessary(const GURL& request_url);

  /**
   * @brief sets userID and password
   * @param request contents of Authorization HTTP header
   */
  void SetAuthorizationIfNecessary(const std::string request);

  enum PolicyType {
    POLICY_RESPONSE,
    POLICY_NAVIGATION,
    POLICY_NEWWINDOW,
  };

  EWebView* web_view_;
  base::WeakPtr<PolicyResponseDelegateEfl> policy_response_delegate_;
  std::unique_ptr<NavigationPolicyHandlerEfl> navigation_policy_handler_;
  std::unique_ptr<_Ewk_Frame> frame_;
  content::WebContentsEflDelegate::NewWindowDecideCallback window_create_callback_;
  std::string cookie_;
  std::string url_;
  std::string http_method_;
  std::string http_body_;
  std::string host_;
  std::string scheme_;
  std::string response_mime_;
  Eina_Hash* response_headers_;
  Ewk_Policy_Decision_Type decision_type_;
  Ewk_Policy_Navigation_Type navigation_type_;
  bool is_decided_;
  bool is_suspended_;
  int response_status_code_;
  std::string auth_user_;
  std::string auth_password_;
  PolicyType type_;
};

#endif // ewk_policy_decision_private_h
