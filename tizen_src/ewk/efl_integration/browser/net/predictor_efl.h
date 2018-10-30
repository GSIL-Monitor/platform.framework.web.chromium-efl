// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_SRC_EWK_EFL_INTEGRATION_PREDICTOR_EFL_H_
#define TIZEN_SRC_EWK_EFL_INTEGRATION_PREDICTOR_EFL_H_

#include <map>
#include <queue>
#include <set>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "browser/net/url_info.h"
#include "components/network_hints/common/network_hints_common.h"
#include "url/gurl.h"

namespace net {
class HostResolver;
class TransportSecurityState;
class URLRequestContextGetter;
}  // namespace net

namespace content {
class BrowserContextEfl;
}

typedef std::map<GURL, chrome_browser_net::UrlInfo> Results;

namespace browser_net {

class PredictorEfl {
 public:
  explicit PredictorEfl();
  virtual ~PredictorEfl();

  virtual void InitNetworkPredictor(
      content::BrowserContextEfl* browser_context_efl);
  void FinalizeInitializationOnIOThread();
  void SetBrowserContext(content::BrowserContextEfl* browser_context_efl);

  // Renderer bundles up list and sends to this browser API via IPC.
  void DnsPrefetchList(const std::vector<std::string>& hostnames);

  // May be called from either the IO or UI thread and will PostTask
  // to the IO thread if necessary.
  void PreconnectUrl(
      const GURL& url,
      const GURL& first_party_for_cookies,
      chrome_browser_net::UrlInfo::ResolutionMotivation motivation,
      bool allow_credentials,
      int count);

  const Results& GetResults() const { return results_; }

  bool PredictorEnabled() const;

 private:
  // A simple priority queue for handling host names.
  // Some names that are queued up have |motivation| that requires very rapid
  // handling.  For example, a sub-resource name lookup MUST be done before the
  // actual sub-resource is fetched.  In contrast, a name that was speculatively
  // noted in a page has to be resolved before the user "gets around to"
  // clicking on a link.  By tagging (with a motivation) each push we make into
  // this FIFO queue, the queue can re-order the more important names to service
  // them sooner (relative to some low priority background resolutions).
  class HostNameQueue {
   public:
    HostNameQueue();
    ~HostNameQueue();
    void Push(const GURL& url,
              chrome_browser_net::UrlInfo::ResolutionMotivation motivation);
    bool IsEmpty() const;
    GURL Pop();

   private:
    // The names in the queue that should be serviced (popped) ASAP.
    std::queue<GURL> rush_queue_;
    // The names in the queue that should only be serviced when rush_queue is
    // empty.
    std::queue<GURL> background_queue_;

    DISALLOW_COPY_AND_ASSIGN(HostNameQueue);
  };

  // May be called from either the IO or UI thread and will PostTask
  // to the IO thread if necessary.
  void DnsPrefetchMotivatedList(
      const std::vector<GURL>& urls,
      chrome_browser_net::UrlInfo::ResolutionMotivation motivation);

  // Add hostname(s) to the queue for processing.
  void ResolveList(
      const std::vector<GURL>& urls,
      chrome_browser_net::UrlInfo::ResolutionMotivation motivation);

  // Access method for use by async lookup request to pass resolution result.
  void OnLookupFinished(const GURL& url, int found);

  // Underlying method for both async and synchronous lookup to update state.
  void LookupFinished(const GURL& url, bool found);

  // Queue hostname for resolution.  If queueing was done, return the pointer
  // to the queued instance, otherwise return NULL. If the proxy advisor is
  // enabled, and |url| is likely to be proxied, the hostname will not be
  // queued as the browser is not expected to fetch it directly.
  chrome_browser_net::UrlInfo* AppendToResolutionQueue(
      const GURL& url,
      chrome_browser_net::UrlInfo::ResolutionMotivation motivation);

  // Check to see if too much queuing delay has been noted for the given info,
  // which indicates that there is "congestion" or growing delay in handling the
  // resolution of names.  Rather than letting this congestion potentially grow
  // without bounds, we abandon our queued efforts at pre-resolutions in such a
  // case.
  // To do this, we will recycle |info|, as well as all queued items, back to
  // the state they had before they were queued up.  We can't do anything about
  // the resolutions we've already sent off for processing on another thread, so
  // we just let them complete.  On a slow system, subject to congestion, this
  // will greatly reduce the number of resolutions done, but it will assure that
  // any resolutions that are done, are in a timely and hence potentially
  // helpful manner.
  bool CongestionControlPerformed(chrome_browser_net::UrlInfo* info);

  // Take lookup requests from work_queue_ and tell HostResolver to look them up
  // asynchronously, provided we don't exceed concurrent resolution limit.
  void StartSomeQueuedResolutions();

  void PreconnectUrlOnIOThread(
      const GURL& original_url,
      const GURL& first_party_for_cookies,
      chrome_browser_net::UrlInfo::ResolutionMotivation motivation,
      bool allow_credentials,
      int count);

  GURL GetHSTSRedirectOnIOThread(const GURL& url);

  // Reference to URLRequestContextGetter from the Profile which owns the
  // predictor. Used by Preconnect.
  content::BrowserContextEfl* browser_context_efl_;

  bool predictor_enabled_;

  size_t num_pending_lookups_;

  // TODO(csharrison): It is not great that two weak pointer factories are
  // needed in this class. Let's split it into two classes that live on each
  // thread.
  //
  // Weak factory for weak pointers that should be dereferenced on the IO
  // thread.
  std::unique_ptr<base::WeakPtrFactory<PredictorEfl>> io_weak_factory_;

  // The maximum queueing delay that is acceptable before we enter congestion
  // reduction mode, and discard all queued (but not yet assigned) resolutions.
  const base::TimeDelta max_dns_queue_delay_;

  // work_queue_ holds a list of names we need to look up.
  HostNameQueue work_queue_;

  // results_ contains information for existing/prior prefetches.
  Results results_;

  // Protects |predictor_enabled_|.
  mutable base::Lock predictor_enabled_lock_;

  DISALLOW_COPY_AND_ASSIGN(PredictorEfl);
};

}  // namespace browser_net

#endif  // TIZEN_SRC_EWK_EFL_INTEGRATION_PREDICTOR_EFL_H_
