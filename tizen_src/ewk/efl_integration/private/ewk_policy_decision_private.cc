// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_policy_decision_private.h"

#include <algorithm>

#include "common/navigation_policy_params.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "eweb_view.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/http/http_response_headers.h"
#include "private/ewk_frame_private.h"
#include "third_party/WebKit/common/mime_util/mime_util.h"
#include "url/url_constants.h"

using content::BrowserThread;
using content::RenderFrameHost;
using content::RenderViewHost;
using content::ResourceRequestInfo;

namespace {
void FreeStringShare(void* data) {
  eina_stringshare_del(static_cast<char*>(data));
}
}

_Ewk_Policy_Decision::_Ewk_Policy_Decision(const GURL& request_url,
                                           net::URLRequest* request,
                                           PolicyResponseDelegateEfl* delegate)
    : web_view_(NULL),
      policy_response_delegate_(AsWeakPtr(delegate)),
      response_headers_(NULL),
      decision_type_(EWK_POLICY_DECISION_USE),
      navigation_type_(EWK_POLICY_NAVIGATION_TYPE_OTHER),
      is_decided_(false),
      is_suspended_(false),
      response_status_code_(0),
      type_(POLICY_RESPONSE) {
  DCHECK(delegate);
  DCHECK(request);

  net::HttpResponseHeaders* response_headers = request->response_headers();

  ParseUrl(request_url);
  if (response_headers) {
    response_status_code_ = response_headers->response_code();

    request->GetMimeType(&response_mime_);

    const ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(request);
    /* LCOV_EXCL_START */
    if (info && info->IsDownload())
      decision_type_ = EWK_POLICY_DECISION_DOWNLOAD;

    if (request_url.has_password() && request_url.has_username())
      SetAuthorizationIfNecessary(request_url);
    /* LCOV_EXCL_STOP */

    std::string set_cookie_;

    /* LCOV_EXCL_START */
    if (response_headers->EnumerateHeader(NULL, "Set-Cookie", &set_cookie_))
      cookie_ = set_cookie_;
    /* LCOV_EXCL_STOP */

    size_t iter = 0;
    std::string name;
    std::string value;
    response_headers_ = eina_hash_string_small_new(FreeStringShare);
    while (response_headers->EnumerateHeaderLines(&iter, &name, &value))
      eina_hash_add(response_headers_, name.c_str(),
                    eina_stringshare_add(value.c_str()));

    if (request->method() == "POST" || request->method() == "PUT") {
      /* LCOV_EXCL_START */
      const net::UploadDataStream* stream = request->get_upload();
      if (stream && stream->size() == 1u) {
        const net::UploadBytesElementReader* reader =
            (*stream->GetElementReaders())[0]->AsBytesReader();
        std::string request_body(reader->bytes());
        http_body_ = request_body;
      }
      /* LCOV_EXCL_STOP */
    }
  } else if (scheme_ == url::kFileScheme) {
    // There are no headers for file:// protocol, but we still get mime type
    // because of mime type detection.
    request->GetMimeType(&response_mime_);
  }
}

_Ewk_Policy_Decision::_Ewk_Policy_Decision(const NavigationPolicyParams& params)
    : web_view_(NULL),
      navigation_policy_handler_(new NavigationPolicyHandlerEfl()),
      frame_(new _Ewk_Frame(params)),
      cookie_(params.cookie),
      http_method_(params.httpMethod),
      response_headers_(NULL),
      decision_type_(EWK_POLICY_DECISION_USE),
      navigation_type_(static_cast<Ewk_Policy_Navigation_Type>(params.type)),
      is_decided_(false),
      is_suspended_(false),
      response_status_code_(0),
      type_(POLICY_NAVIGATION) {
  ParseUrl(params.url);
  if (!params.auth.IsEmpty())
    SetAuthorizationIfNecessary(params.auth.Utf8());  // LCOV_EXCL_LINE
  else if (params.url.has_password() && params.url.has_username())
    SetAuthorizationIfNecessary(params.url);  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
_Ewk_Policy_Decision::_Ewk_Policy_Decision(
    EWebView* view,
    content::WebContentsEflDelegate::NewWindowDecideCallback callback)
    : web_view_(view),
      window_create_callback_(callback),
      response_headers_(NULL),
      decision_type_(EWK_POLICY_DECISION_USE),
      navigation_type_(EWK_POLICY_NAVIGATION_TYPE_OTHER),
      is_decided_(false),
      is_suspended_(false),
      response_status_code_(0),
      type_(POLICY_NEWWINDOW) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(view);

  RenderFrameHost* rfh = NULL;
  // we can use main frame here
  if (view) {
    view->web_contents().GetMainFrame();
  }

  frame_.reset(new _Ewk_Frame(rfh));
}
/* LCOV_EXCL_STOP */

_Ewk_Policy_Decision::~_Ewk_Policy_Decision() {
  eina_hash_free(response_headers_);
}

void _Ewk_Policy_Decision::Use() {
  DCHECK(policy_response_delegate_.get());
  is_decided_ = true;
  switch (type_) {
    case POLICY_RESPONSE:
      policy_response_delegate_->UseResponse();
      break;
    case POLICY_NAVIGATION:
      navigation_policy_handler_->SetDecision(
          NavigationPolicyHandlerEfl::Unhandled);
      break;
    case POLICY_NEWWINDOW:
      window_create_callback_.Run(true);
      break;
    default:
      NOTREACHED();
      break;
  }

  if (isSuspended()) {
    BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
  }
}

/* LCOV_EXCL_START */
void _Ewk_Policy_Decision::Ignore() {
  DCHECK(policy_response_delegate_.get());
  is_decided_ = true;
  switch (type_) {
    case _Ewk_Policy_Decision::POLICY_RESPONSE:
      policy_response_delegate_->IgnoreResponse();
      break;
    case _Ewk_Policy_Decision::POLICY_NAVIGATION:
      navigation_policy_handler_->SetDecision(
          NavigationPolicyHandlerEfl::Handled);
      break;
    case _Ewk_Policy_Decision::POLICY_NEWWINDOW:
      window_create_callback_.Run(false);  // LCOV_EXCL_LINE
      break;                               // LCOV_EXCL_LINE
    default:
      NOTREACHED();
      break;
  }

  if (isSuspended()) {
    BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
  }
}

bool _Ewk_Policy_Decision::Suspend() {
  if (!is_decided_ && type_ != _Ewk_Policy_Decision::POLICY_NAVIGATION) {
    // Policy navigation API is currently synchronous - we can't suspend it
    is_suspended_ = true;
    return true;
  }

  return false;
}
/* LCOV_EXCL_STOP */

void _Ewk_Policy_Decision::InitializeOnUIThread() {
  DCHECK(type_ == _Ewk_Policy_Decision::POLICY_RESPONSE);
  DCHECK(policy_response_delegate_.get());

  if (policy_response_delegate_.get()) {
    RenderFrameHost* host =
        RenderFrameHost::FromID(policy_response_delegate_->GetRenderProcessId(),
                                policy_response_delegate_->GetRenderFrameId());

    // Download request has no render frame id, they're detached. We override it
    // with main frame from render view id
    if (!host) {
      /* LCOV_EXCL_START */
      RenderViewHost* viewhost = RenderViewHost::FromID(
          policy_response_delegate_->GetRenderProcessId(),
          policy_response_delegate_->GetRenderViewId());

      // DCHECK(viewhost);
      if (viewhost) {
        host = viewhost->GetMainFrame();
      }
      /* LCOV_EXCL_STOP */
    }

    if (host) {
      /*
       * In some situations there is no renderer associated to the response
       * Such case can be observed while running TC
       * utc_blink_ewk_geolocation_permission_request_suspend_func
       */
      frame_.reset(new _Ewk_Frame(host));
    }
  }
}

/* LCOV_EXCL_START */
_Ewk_Frame* _Ewk_Policy_Decision::GetFrameRef() const {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Ups, forgot to initialize something?
  DCHECK(frame_.get());
  return frame_.get();
}
/* LCOV_EXCL_STOP */

void _Ewk_Policy_Decision::ParseUrl(const GURL& url) {
  url_ = url.spec();
  scheme_ = url.scheme();
  host_ = url.host();
}

/* LCOV_EXCL_START */
void _Ewk_Policy_Decision::SetAuthorizationIfNecessary(
    const GURL& request_url) {
  // There is no need to check if username or password is empty.
  // It was checked befor in constructor
  auth_password_ = request_url.password();
  auth_user_ = request_url.username();
}

void _Ewk_Policy_Decision::SetAuthorizationIfNecessary(
    const std::string request) {
  std::string type = request.substr(0, request.find_first_of(' '));
  std::transform(type.begin(), type.end(), type.begin(), ::toupper);

  if (type.compare(BASIC_AUTHORIZATION)) {
    auth_user_.clear();
    auth_password_.clear();
    return;
  }

  std::size_t space = request.find(' ');
  std::size_t colon = request.find(':');

  DCHECK(space != std::string::npos && colon != std::string::npos &&
         colon != request.length());
  if (space == std::string::npos || colon == std::string::npos ||
      colon == request.length())
    return;

  auth_user_ = request.substr(space + 1, request.length() - colon - 1);
  auth_password_ = request.substr(colon + 1);
}

const char* _Ewk_Policy_Decision::GetCookie() const {
  return cookie_.c_str();
}

const char* _Ewk_Policy_Decision::GetAuthUser() const {
  return auth_user_.c_str();
}

const char* _Ewk_Policy_Decision::GetAuthPassword() const {
  return auth_password_.c_str();
}
/* LCOV_EXCL_STOP */

const char* _Ewk_Policy_Decision::GetUrl() const {
  return url_.c_str();
}

const char* _Ewk_Policy_Decision::GetHttpMethod() const {
  return http_method_.c_str();
}

/* LCOV_EXCL_START */
const char* _Ewk_Policy_Decision::GetHttpBody() const {
  return http_body_.c_str();
}

const char* _Ewk_Policy_Decision::GetScheme() const {
  return scheme_.c_str();
}

const char* _Ewk_Policy_Decision::GetHost() const {
  return host_.c_str();
}
/* LCOV_EXCL_STOP */

const char* _Ewk_Policy_Decision::GetResponseMime() const {
  return response_mime_.c_str();
}

/* LCOV_EXCL_START */
void _Ewk_Policy_Decision::SetResponseMime(const std::string& response_mime) {
  response_mime_ = response_mime;
}
/* LCOV_EXCL_STOP */
