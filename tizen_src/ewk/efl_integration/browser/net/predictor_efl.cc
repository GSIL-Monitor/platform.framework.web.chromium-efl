// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/net/predictor_efl.h"

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "browser_context_efl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_hints.h"
#include "net/base/address_list.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_network_session.h"
#include "net/http/http_request_info.h"
#include "net/http/http_stream_factory.h"
#include "net/http/http_transaction_factory.h"
#include "net/http/transport_security_state.h"
#include "net/log/net_log.h"
#include "net/ssl/ssl_config_service.h"
#include "net/url_request/http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "url_request_context_getter_efl.h"

using content::BrowserThread;

const static size_t kMaxSpeculativeParallelResolves = 3;
const static int kExpectedResolutionTimeMs = 500;
const static int kTypicalSpeculativeGroupSize = 8;
static int kMaxSpeculativeResolveQueueDelayMs =
    (kExpectedResolutionTimeMs * kTypicalSpeculativeGroupSize) /
    kMaxSpeculativeParallelResolves;

namespace browser_net {

namespace {

const base::Feature kNetworkPrediction{"NetworkPrediction",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace

PredictorEfl::HostNameQueue::HostNameQueue() {}

PredictorEfl::HostNameQueue::~HostNameQueue() {}

void PredictorEfl::HostNameQueue::Push(
    const GURL& url,
    chrome_browser_net::UrlInfo::ResolutionMotivation motivation) {
  switch (motivation) {
    case chrome_browser_net::UrlInfo::STATIC_REFERAL_MOTIVATED:
    case chrome_browser_net::UrlInfo::LEARNED_REFERAL_MOTIVATED:
    case chrome_browser_net::UrlInfo::MOUSE_OVER_MOTIVATED:
      rush_queue_.push(url);
      break;

    default:
      background_queue_.push(url);
      break;
  }
}

bool PredictorEfl::HostNameQueue::IsEmpty() const {
  return rush_queue_.empty() && background_queue_.empty();
}

GURL PredictorEfl::HostNameQueue::Pop() {
  DCHECK(!IsEmpty());
  std::queue<GURL>* queue(rush_queue_.empty() ? &background_queue_
                                              : &rush_queue_);
  GURL url(queue->front());
  queue->pop();
  return url;
}

PredictorEfl::PredictorEfl()
    : browser_context_efl_(NULL),
      predictor_enabled_(true),
      num_pending_lookups_(0),
      max_dns_queue_delay_(base::TimeDelta::FromMilliseconds(
          kMaxSpeculativeResolveQueueDelayMs)) {}

PredictorEfl::~PredictorEfl() {}

void PredictorEfl::InitNetworkPredictor(
    content::BrowserContextEfl* browser_context_efl) {
  browser_context_efl_ = browser_context_efl;
  predictor_enabled_ = base::FeatureList::IsEnabled(kNetworkPrediction);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&PredictorEfl::FinalizeInitializationOnIOThread,
                 base::Unretained(this)));
}

void PredictorEfl::FinalizeInitializationOnIOThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  io_weak_factory_.reset(new base::WeakPtrFactory<PredictorEfl>(this));
}

// This API is only used in the browser process.
// It is called from an IPC message originating in the renderer.  It currently
// includes both Page-Scan, and Link-Hover prefetching.
// TODO(jar): Separate out link-hover prefetching, and page-scan results.
void PredictorEfl::DnsPrefetchList(const std::vector<std::string>& hostnames) {
  // TODO(jar): Push GURL transport further back into renderer, but this will
  // require a Webkit change in the observer :-/.
  std::vector<GURL> urls;
  for (std::vector<std::string>::const_iterator it = hostnames.begin();
       it < hostnames.end(); ++it) {
    urls.push_back(GURL("http://" + *it + ":80"));
  }

  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DnsPrefetchMotivatedList(urls,
                           chrome_browser_net::UrlInfo::PAGE_SCAN_MOTIVATED);
}

void PredictorEfl::DnsPrefetchMotivatedList(
    const std::vector<GURL>& urls,
    chrome_browser_net::UrlInfo::ResolutionMotivation motivation) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) ||
         BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!PredictorEnabled())
    return;

  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    ResolveList(urls, motivation);
  } else {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&PredictorEfl::ResolveList, base::Unretained(this), urls,
                   motivation));
  }
}

// Overloaded Resolve() to take a vector of names.
void PredictorEfl::ResolveList(
    const std::vector<GURL>& urls,
    chrome_browser_net::UrlInfo::ResolutionMotivation motivation) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (std::vector<GURL>::const_iterator it = urls.begin(); it < urls.end();
       ++it) {
    AppendToResolutionQueue(*it, motivation);
  }
}

chrome_browser_net::UrlInfo* PredictorEfl::AppendToResolutionQueue(
    const GURL& url,
    chrome_browser_net::UrlInfo::ResolutionMotivation motivation) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(url.has_host());

  chrome_browser_net::UrlInfo* info = &results_[url];
  info->SetUrl(url);  // Initialize or DCHECK.
  // TODO(jar):  I need to discard names that have long since expired.
  // Currently we only add to the domain map :-/

  DCHECK(info->HasUrl(url));

  if (!info->NeedsDnsUpdate()) {
    info->DLogResultsStats("DNS PrefetchNotUpdated");
    return nullptr;
  }

  info->SetQueuedState(motivation);
  work_queue_.Push(url, motivation);

  StartSomeQueuedResolutions();
  return info;
}

bool PredictorEfl::CongestionControlPerformed(
    chrome_browser_net::UrlInfo* info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Note: queue_duration is ONLY valid after we go to assigned state.
  if (info->queue_duration() < max_dns_queue_delay_)
    return false;
  // We need to discard all entries in our queue, as we're keeping them waiting
  // too long.  By doing this, we'll have a chance to quickly service urgent
  // resolutions, and not have a bogged down system.
  while (true) {
    info->RemoveFromQueue();
    if (work_queue_.IsEmpty())
      break;
    info = &results_[work_queue_.Pop()];
    info->SetAssignedState();
  }
  return true;
}

void PredictorEfl::StartSomeQueuedResolutions() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  while (!work_queue_.IsEmpty() &&
         num_pending_lookups_ < kMaxSpeculativeParallelResolves) {
    const GURL url(work_queue_.Pop());
    chrome_browser_net::UrlInfo* info = &results_[url];
    DCHECK(info->HasUrl(url));
    info->SetAssignedState();

    if (CongestionControlPerformed(info)) {
      DCHECK(work_queue_.IsEmpty());
      return;
    }

    if (!browser_context_efl_)
      return;

    int status = content::PreresolveUrl(
        browser_context_efl_->GetRequestContextEfl(), url,
        base::Bind(&PredictorEfl::OnLookupFinished,
                   io_weak_factory_->GetWeakPtr(), url));
    if (status == net::ERR_IO_PENDING) {
      // Will complete asynchronously.
      num_pending_lookups_++;
    } else {
      // Completed synchronously (was already cached by HostResolver), or else
      // there was (equivalently) some network error that prevents us from
      // finding the name.  Status net::OK means it was "found."
      LookupFinished(url, status == net::OK);
    }
  }
}

void PredictorEfl::OnLookupFinished(const GURL& url, int result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  LookupFinished(url, result == net::OK);
  DCHECK_GT(num_pending_lookups_, 0u);
  num_pending_lookups_--;
  StartSomeQueuedResolutions();
}

void PredictorEfl::LookupFinished(const GURL& url, bool found) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  chrome_browser_net::UrlInfo* info = &results_[url];
  DCHECK(info->HasUrl(url));
  if (info->is_marked_to_delete()) {
    results_.erase(url);
  } else {
    if (found)
      info->SetFoundState();
    else
      info->SetNoSuchNameState();
  }
}

void PredictorEfl::PreconnectUrl(
    const GURL& url,
    const GURL& first_party_for_cookies,
    chrome_browser_net::UrlInfo::ResolutionMotivation motivation,
    bool allow_credentials,
    int count) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) ||
         BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    PreconnectUrlOnIOThread(url, first_party_for_cookies, motivation,
                            allow_credentials, count);
  } else {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&PredictorEfl::PreconnectUrlOnIOThread,
                   base::Unretained(this), url, first_party_for_cookies,
                   motivation, allow_credentials, count));
  }
}

void PredictorEfl::PreconnectUrlOnIOThread(
    const GURL& original_url,
    const GURL& first_party_for_cookies,
    chrome_browser_net::UrlInfo::ResolutionMotivation motivation,
    bool allow_credentials,
    int count) {
  // Skip the HSTS redirect.
  GURL url = GetHSTSRedirectOnIOThread(original_url);

  if (!browser_context_efl_)
    return;

  // Translate the motivation from UrlRequest motivations to HttpRequest
  // motivations.
  net::HttpRequestInfo::RequestMotivation request_motivation =
      net::HttpRequestInfo::NORMAL_MOTIVATION;
  switch (motivation) {
    case chrome_browser_net::UrlInfo::OMNIBOX_MOTIVATED:
      request_motivation = net::HttpRequestInfo::OMNIBOX_MOTIVATED;
      break;
    case chrome_browser_net::UrlInfo::LEARNED_REFERAL_MOTIVATED:
      request_motivation = net::HttpRequestInfo::PRECONNECT_MOTIVATED;
      break;
    case chrome_browser_net::UrlInfo::MOUSE_OVER_MOTIVATED:
    case chrome_browser_net::UrlInfo::SELF_REFERAL_MOTIVATED:
    case chrome_browser_net::UrlInfo::EARLY_LOAD_MOTIVATED:
      request_motivation = net::HttpRequestInfo::EARLY_LOAD_MOTIVATED;
      break;
    default:
      // Other motivations should never happen here.
      NOTREACHED();
      break;
  }

  content::PreconnectUrl(browser_context_efl_->GetRequestContextEfl(), url,
                         first_party_for_cookies, count, allow_credentials,
                         request_motivation);
}

GURL PredictorEfl::GetHSTSRedirectOnIOThread(const GURL& url) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  net::TransportSecurityState* transport_security_state_ = nullptr;
  if (!browser_context_efl_)
    return url;

  net::URLRequestContextGetter* getter =
      browser_context_efl_->GetRequestContextEfl();

  net::URLRequestContext* context = nullptr;
  if (getter)
    context = getter->GetURLRequestContext();

  if (context)
    transport_security_state_ = context->transport_security_state();

  if (!transport_security_state_)
    return url;
  if (!url.SchemeIs("http"))
    return url;
  if (!transport_security_state_->ShouldUpgradeToSSL(url.host()))
    return url;

  url::Replacements<char> replacements;
  const char kNewScheme[] = "https";
  replacements.SetScheme(kNewScheme, url::Component(0, strlen(kNewScheme)));
  return url.ReplaceComponents(replacements);
}

bool PredictorEfl::PredictorEnabled() const {
  base::AutoLock lock(predictor_enabled_lock_);
  return predictor_enabled_;
}

}  // namespace browser_net
