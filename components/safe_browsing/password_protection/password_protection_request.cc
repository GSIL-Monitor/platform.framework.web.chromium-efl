// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/password_protection/password_protection_request.h"

#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_macros.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/password_manager/core/browser/password_reuse_detector.h"
#include "components/safe_browsing/db/whitelist_checker_client.h"
#include "components/safe_browsing/password_protection/password_protection_navigation_throttle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/origin.h"

using content::BrowserThread;
using content::WebContents;

namespace safe_browsing {

namespace {

// Cap on how many reused domains can be included in a report, to limit
// the size of the report. UMA suggests 99.9% will have < 200 domains.
const int kMaxReusedDomains = 200;

}  // namespace

const char kPasswordOnFocusVerdictHistogram[] =
    "PasswordProtection.Verdict.PasswordFieldOnFocus";
const char kAnyPasswordEntryVerdictHistogram[] =
    "PasswordProtection.Verdict.AnyPasswordEntry";
const char kSyncPasswordEntryVerdictHistogram[] =
    "PasswordProtection.Verdict.SyncPasswordEntry";
const char kProtectedPasswordEntryVerdictHistogram[] =
    "PasswordProtection.Verdict.ProtectedPasswordEntry";

PasswordProtectionRequest::PasswordProtectionRequest(
    WebContents* web_contents,
    const GURL& main_frame_url,
    const GURL& password_form_action,
    const GURL& password_form_frame_url,
    bool matches_sync_password,
    const std::vector<std::string>& matching_domains,
    LoginReputationClientRequest::TriggerType type,
    bool password_field_exists,
    PasswordProtectionService* pps,
    int request_timeout_in_ms)
    : web_contents_(web_contents),
      main_frame_url_(main_frame_url),
      password_form_action_(password_form_action),
      password_form_frame_url_(password_form_frame_url),
      matches_sync_password_(matches_sync_password),
      matching_domains_(matching_domains),
      trigger_type_(type),
      password_field_exists_(password_field_exists),
      password_protection_service_(pps),
      request_timeout_in_ms_(request_timeout_in_ms),
      request_proto_(base::MakeUnique<LoginReputationClientRequest>()),
      is_modal_warning_showing_(false),
      weakptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  DCHECK(trigger_type_ == LoginReputationClientRequest::UNFAMILIAR_LOGIN_PAGE ||
         trigger_type_ == LoginReputationClientRequest::PASSWORD_REUSE_EVENT);
  DCHECK(trigger_type_ != LoginReputationClientRequest::PASSWORD_REUSE_EVENT ||
         matches_sync_password_ || matching_domains_.size() > 0);

  request_proto_->set_trigger_type(trigger_type_);
}

PasswordProtectionRequest::~PasswordProtectionRequest() {
  weakptr_factory_.InvalidateWeakPtrs();
}

void PasswordProtectionRequest::Start() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  CheckWhitelist();
}

void PasswordProtectionRequest::CheckWhitelist() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Start a task on the IO thread to check the whitelist. It may
  // callback immediately on the IO thread or take some time if a full-hash-
  // check is required.
  auto result_callback = base::Bind(&OnWhitelistCheckDoneOnIO, GetWeakPtr());
  tracker_.PostTask(
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO).get(), FROM_HERE,
      base::Bind(&WhitelistCheckerClient::StartCheckCsdWhitelist,
                 password_protection_service_->database_manager(),
                 main_frame_url_, result_callback));
}

// static
void PasswordProtectionRequest::OnWhitelistCheckDoneOnIO(
    base::WeakPtr<PasswordProtectionRequest> weak_request,
    bool match_whitelist) {
  // Don't access weak_request on IO thread. Move it back to UI thread first.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&PasswordProtectionRequest::OnWhitelistCheckDone, weak_request,
                 match_whitelist));
}

void PasswordProtectionRequest::OnWhitelistCheckDone(bool match_whitelist) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (match_whitelist)
    Finish(PasswordProtectionService::MATCHED_WHITELIST, nullptr);
  else
    CheckCachedVerdicts();
}

void PasswordProtectionRequest::CheckCachedVerdicts() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!password_protection_service_) {
    Finish(PasswordProtectionService::SERVICE_DESTROYED, nullptr);
    return;
  }

  std::unique_ptr<LoginReputationClientResponse> cached_response =
      base::MakeUnique<LoginReputationClientResponse>();
  auto verdict = password_protection_service_->GetCachedVerdict(
      main_frame_url_, trigger_type_, cached_response.get());
  if (verdict != LoginReputationClientResponse::VERDICT_TYPE_UNSPECIFIED)
    Finish(PasswordProtectionService::RESPONSE_ALREADY_CACHED,
           std::move(cached_response));
  else
    SendRequest();
}

void PasswordProtectionRequest::FillRequestProto() {
  request_proto_->set_page_url(main_frame_url_.spec());
  password_protection_service_->FillUserPopulation(trigger_type_,
                                                   request_proto_.get());
  request_proto_->set_stored_verdict_cnt(
      password_protection_service_->GetStoredVerdictCount(trigger_type_));
  LoginReputationClientRequest::Frame* main_frame =
      request_proto_->add_frames();
  main_frame->set_url(main_frame_url_.spec());
  main_frame->set_frame_index(0 /* main frame */);
  password_protection_service_->FillReferrerChain(
      main_frame_url_, -1 /* tab id not available */, main_frame);
  bool clicked_through_interstitial =
      password_protection_service_->UserClickedThroughSBInterstitial(
          web_contents_);
  request_proto_->set_clicked_through_interstitial(
      clicked_through_interstitial);

  switch (trigger_type_) {
    case LoginReputationClientRequest::UNFAMILIAR_LOGIN_PAGE: {
      UMA_HISTOGRAM_BOOLEAN(
          "PasswordProtection.UserClickedThroughSBInterstitial."
          "PasswordFieldOnFocus",
          clicked_through_interstitial);
      LoginReputationClientRequest::Frame::Form* password_form;
      if (password_form_frame_url_ == main_frame_url_) {
        main_frame->set_has_password_field(true);
        password_form = main_frame->add_forms();
      } else {
        LoginReputationClientRequest::Frame* password_frame =
            request_proto_->add_frames();
        password_frame->set_url(password_form_frame_url_.spec());
        password_frame->set_has_password_field(true);
        // TODO(jialiul): Add referrer chain for subframes later.
        password_form = password_frame->add_forms();
      }
      password_form->set_action_url(password_form_action_.spec());
      // TODO(jialiul): Fill more frame specific info when Safe Browsing backend
      // is ready to handle these pieces of information.
      break;
    }
    case LoginReputationClientRequest::PASSWORD_REUSE_EVENT: {
      UMA_HISTOGRAM_BOOLEAN(
          "PasswordProtection.UserClickedThroughSBInterstitial."
          "AnyPasswordEntry",
          clicked_through_interstitial);
      main_frame->set_has_password_field(password_field_exists_);
      LoginReputationClientRequest::PasswordReuseEvent* reuse_event =
          request_proto_->mutable_password_reuse_event();
      reuse_event->set_is_chrome_signin_password(matches_sync_password_);
      if (matches_sync_password_) {
        UMA_HISTOGRAM_BOOLEAN(
            "PasswordProtection.UserClickedThroughSBInterstitial."
            "SyncPasswordEntry",
            clicked_through_interstitial);
        reuse_event->set_sync_account_type(
            password_protection_service_->GetSyncAccountType());
        UMA_HISTOGRAM_ENUMERATION(
            "PasswordProtection.PasswordReuseSyncAccountType",
            reuse_event->sync_account_type(),
            LoginReputationClientRequest::PasswordReuseEvent::
                    SyncAccountType_MAX +
                1);
      } else {
        UMA_HISTOGRAM_BOOLEAN(
            "PasswordProtection.UserClickedThroughSBInterstitial."
            "ProtectedPasswordEntry",
            clicked_through_interstitial);
      }
      if (password_protection_service_->IsExtendedReporting() &&
          !password_protection_service_->IsIncognito()) {
        for (const auto& domain : matching_domains_) {
          reuse_event->add_domains_matching_password(domain);
          if (reuse_event->domains_matching_password_size() >=
              kMaxReusedDomains)
            break;
        }
      }
      break;
    }
    default:
      NOTREACHED();
  }
}

void PasswordProtectionRequest::SendRequest() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  FillRequestProto();

  std::string serialized_request;
  if (!request_proto_->SerializeToString(&serialized_request)) {
    Finish(PasswordProtectionService::REQUEST_MALFORMED, nullptr);
    return;
  }

  // In case the request take too long, we set a timer to cancel this request.
  StartTimeout();
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("password_protection_request", R"(
        semantics {
          sender: "Safe Browsing"
          description:
            "When the user is about to log in to a new, uncommon site, Chrome "
            "will send a request to Safe Browsing to determine if the page is "
            "phishing. It'll then show a warning if the page poses a risk of "
            "phishing."
          trigger:
            "When a user focuses on a password field on a page that they "
            "haven't visited before and that isn't popular or known to be safe."
          data:
            "URL and referrer of the current page, password form action, and "
            "iframe structure."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "Safe Browsing Cookie Store"
          setting:
            "Users can control this feature via 'Protect you and your device "
            "from dangerous sites' or 'Automatically report details of "
            "possible security incidents to Google' setting under 'Privacy'. "
            "By default, the first setting is enabled and the second is not."
          chrome_policy {
            SafeBrowsingExtendedReportingOptInAllowed {
              policy_options {mode: MANDATORY}
              SafeBrowsingExtendedReportingOptInAllowed: false
            }
          }
        })");
  fetcher_ = net::URLFetcher::Create(
      0, PasswordProtectionService::GetPasswordProtectionRequestUrl(),
      net::URLFetcher::POST, this, traffic_annotation);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher_.get(), data_use_measurement::DataUseUserData::SAFE_BROWSING);
  fetcher_->SetLoadFlags(net::LOAD_DISABLE_CACHE);
  fetcher_->SetAutomaticallyRetryOn5xx(false);
  fetcher_->SetRequestContext(
      password_protection_service_->request_context_getter().get());
  fetcher_->SetUploadData("application/octet-stream", serialized_request);
  request_start_time_ = base::TimeTicks::Now();
  fetcher_->Start();
}

void PasswordProtectionRequest::StartTimeout() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // If request is not done withing 10 seconds, we cancel this request.
  // The weak pointer used for the timeout will be invalidated (and
  // hence would prevent the timeout) if the check completes on time and
  // execution reaches Finish().
  BrowserThread::PostDelayedTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&PasswordProtectionRequest::Cancel, GetWeakPtr(), true),
      base::TimeDelta::FromMilliseconds(request_timeout_in_ms_));
}

void PasswordProtectionRequest::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  net::URLRequestStatus status = source->GetStatus();
  const bool is_success = status.is_success();
  const int response_code = source->GetResponseCode();

  UMA_HISTOGRAM_SPARSE_SLOWLY(
      "PasswordProtection.PasswordProtectionResponseOrErrorCode",
      is_success ? response_code : status.error());

  if (!is_success || net::HTTP_OK != response_code) {
    Finish(PasswordProtectionService::FETCH_FAILED, nullptr);
    return;
  }

  std::unique_ptr<LoginReputationClientResponse> response =
      base::MakeUnique<LoginReputationClientResponse>();
  std::string response_body;
  bool received_data = source->GetResponseAsString(&response_body);
  DCHECK(received_data);
  fetcher_.reset();  // We don't need it anymore.
  UMA_HISTOGRAM_TIMES("PasswordProtection.RequestNetworkDuration",
                      base::TimeTicks::Now() - request_start_time_);
  if (response->ParseFromString(response_body))
    Finish(PasswordProtectionService::SUCCEEDED, std::move(response));
  else
    Finish(PasswordProtectionService::RESPONSE_MALFORMED, nullptr);
}

void PasswordProtectionRequest::Finish(
    PasswordProtectionService::RequestOutcome outcome,
    std::unique_ptr<LoginReputationClientResponse> response) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  tracker_.TryCancelAll();
  if (trigger_type_ == LoginReputationClientRequest::UNFAMILIAR_LOGIN_PAGE) {
    UMA_HISTOGRAM_ENUMERATION(kPasswordOnFocusRequestOutcomeHistogram, outcome,
                              PasswordProtectionService::MAX_OUTCOME);
  } else {
    PasswordProtectionService::LogPasswordEntryRequestOutcome(
        outcome, matches_sync_password_);
    if (matches_sync_password_) {
      password_protection_service_->MaybeLogPasswordReuseLookupEvent(
          web_contents_, outcome, response.get());
    }
  }

  if (outcome == PasswordProtectionService::SUCCEEDED && response) {
    switch (trigger_type_) {
      case LoginReputationClientRequest::UNFAMILIAR_LOGIN_PAGE:
        UMA_HISTOGRAM_ENUMERATION(
            kPasswordOnFocusVerdictHistogram, response->verdict_type(),
            LoginReputationClientResponse_VerdictType_VerdictType_MAX + 1);
        break;
      case LoginReputationClientRequest::PASSWORD_REUSE_EVENT:
        UMA_HISTOGRAM_ENUMERATION(
            kAnyPasswordEntryVerdictHistogram, response->verdict_type(),
            LoginReputationClientResponse_VerdictType_VerdictType_MAX + 1);
        if (matches_sync_password_) {
          UMA_HISTOGRAM_ENUMERATION(
              kSyncPasswordEntryVerdictHistogram, response->verdict_type(),
              LoginReputationClientResponse_VerdictType_VerdictType_MAX + 1);
        } else {
          UMA_HISTOGRAM_ENUMERATION(
              kProtectedPasswordEntryVerdictHistogram, response->verdict_type(),
              LoginReputationClientResponse_VerdictType_VerdictType_MAX + 1);
        }
        break;
      default:
        NOTREACHED();
    }
  }

  password_protection_service_->RequestFinished(
      this, outcome == PasswordProtectionService::RESPONSE_ALREADY_CACHED,
      std::move(response));
}

void PasswordProtectionRequest::Cancel(bool timed_out) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  fetcher_.reset();
  // If request is canceled because |password_protection_service_| is shutting
  // down, ignore all these deferred navigations.
  if (!timed_out)
    throttles_.clear();

  Finish(timed_out ? PasswordProtectionService::TIMEDOUT
                   : PasswordProtectionService::CANCELED,
         nullptr);
}

void PasswordProtectionRequest::HandleDeferredNavigations() {
  for (auto* throttle : throttles_) {
    if (is_modal_warning_showing_)
      throttle->CancelNavigation(content::NavigationThrottle::CANCEL);
    else
      throttle->ResumeNavigation();
  }
  throttles_.clear();
}

}  // namespace safe_browsing
