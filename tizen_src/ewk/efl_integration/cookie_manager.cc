// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cookie_manager.h"

#include "utility"

#include "base/bind.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/synchronization/waitable_event.h"
#include "browser/scoped_allow_wait_for_legacy_web_view_api.h"
#include "content/public/browser/browser_thread.h"
#include "content/browser/storage_partition_impl.h"
#include "eweb_context.h"
#include "net/base/net_errors.h"
#include "net/base/static_cookie_policy.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/parsed_cookie.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_options.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

#include "net/cookies/cookie_store_test_callbacks.h"

#include <Eina.h>

using content::BrowserThread;
using net::CookieList;
using net::CookieMonster;
using base::AutoLock;

namespace {

void TriggerHostPolicyGetCallbackAsyncOnUIThread(Ewk_Cookie_Accept_Policy policy,
                                                 Ewk_Cookie_Manager_Policy_Async_Get_Cb callback,
                                                 void *data) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (callback)
    (*callback)(policy, data);
}

}

class CookieManager::EwkGetHostCallback {
 public:
  /* LCOV_EXCL_START */
  EwkGetHostCallback(AsyncHostnamesGetCb callback, void* user_data)
    : callback_(callback),
      user_data_(user_data) {}

  void TriggerCallback(const net::CookieList& cookies) {
    Eina_List* hostnames = 0;
    if (cookies.size()) {
      net::CookieList::const_iterator it = cookies.begin();
      while (it != cookies.end()) {
        hostnames = eina_list_append(hostnames, eina_stringshare_add(it->Name().c_str()));
        ++it;
      }
    }
    (*callback_)(hostnames, NULL, user_data_);
    void* item = 0;
    EINA_LIST_FREE(hostnames, item)
      eina_stringshare_del(static_cast<Eina_Stringshare*>(item));
  }
  /* LCOV_EXCL_STOP */

 private:
    AsyncHostnamesGetCb callback_;
    void* user_data_;
};

CookieManager::CookieManager()
    : cookie_policy_(EWK_COOKIE_ACCEPT_POLICY_ALWAYS),
      weak_ptr_factory_(this) {
}

CookieManager::~CookieManager() {
  DeleteSessionCookiesOnIOThread();
}

void CookieManager::SetRequestContextGetter(
    scoped_refptr<content::URLRequestContextGetterEfl> getter) {
  request_context_getter_ = getter;
}

void CookieManager::DeleteSessionCookiesOnIOThread() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

  net::CookieStore* cookie_store = GetCookieStore();
  if (cookie_store)
    cookie_store->DeleteSessionCookiesAsync(
        base::OnceCallback<void(uint32_t)>());
}

/* LCOV_EXCL_START */
void CookieManager::DeleteCookiesOnIOThread(const std::string& url,
                                            const std::string& cookie_name) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  net::CookieStore* cookie_store = GetCookieStore();
  if (!cookie_store)
    return;

  if (url.empty()) { // Delete all cookies.
    cookie_store->DeleteAllAsync(net::CookieMonster::DeleteCallback());
    return;
  }

  GURL gurl(url);
  if (!gurl.is_valid())
    return;

  if (cookie_name.empty()) {
    // Delete all matching host cookies.
    cookie_store->DeleteAllCreatedBetweenWithPredicateAsync(
        base::Time(),
        base::Time::Max(),
        content::StoragePartitionImpl::CreatePredicateForHostCookies(gurl),
        net::CookieMonster::DeleteCallback());
  } else {
    // Delete all matching host and domain cookies.
    cookie_store->DeleteCookieAsync(gurl, cookie_name, base::Closure());
  }
}

void CookieManager::DeleteCookiesAsync(const std::string& url,
                                       const std::string& cookie_name) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&CookieManager::DeleteCookiesOnIOThread,
                 base::Unretained(this), url, cookie_name));
}
/* LCOV_EXCL_STOP */

static void SetStoragePathOnIOThread(
    scoped_refptr<content::URLRequestContextGetterEfl> request_context_getter,
    const std::string& path,
    bool persist_session_cookies,
    Ewk_Cookie_Persistent_Storage file_storage_type) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (request_context_getter.get()) {
    base::FilePath storage_path(path);
    request_context_getter->SetCookieStoragePath(storage_path, persist_session_cookies,
                                                 file_storage_type);
  }
}

void CookieManager::SetStoragePath(const std::string& path,
    bool persist_session_cookies,
    Ewk_Cookie_Persistent_Storage file_storage_type) {
 DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

 scoped_refptr<content::URLRequestContextGetterEfl> context_getter = GetContextGetter();
 if (context_getter.get()) {
   BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                           base::Bind(SetStoragePathOnIOThread,
                                      context_getter,
                                      path,
                                      persist_session_cookies,
                                      file_storage_type));
 }
}

void CookieManager::GetAcceptPolicyAsync(Ewk_Cookie_Manager_Policy_Async_Get_Cb callback, void *data) {
  AutoLock lock(lock_);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                                  base::Bind(&TriggerHostPolicyGetCallbackAsyncOnUIThread,
                                             cookie_policy_,
                                             callback,
                                             data));
}

/* LCOV_EXCL_START */
void CookieManager::GetHostNamesWithCookiesAsync(AsyncHostnamesGetCb callback, void *data) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  host_callback_queue_.push(new EwkGetHostCallback(callback, data));
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&CookieManager::FetchCookiesOnIOThread, GetSharedThis()));
}

void CookieManager::FetchCookiesOnIOThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  net::CookieStore* cookie_store = GetCookieStore();
  if (cookie_store) {
    cookie_store->GetAllCookiesAsync(base::Bind(&CookieManager::OnFetchComplete,
                                                GetSharedThis()));
  } else {
    OnFetchComplete(net::CookieList());
  }
}

void CookieManager::OnFetchComplete(const net::CookieList& cookies) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                                    base::Bind(&CookieManager::OnFetchComplete,
                                               GetSharedThis(),
                                               cookies));
    return;
  }
  if (!host_callback_queue_.empty()) {
    EwkGetHostCallback* host_callback = host_callback_queue_.front();
    if (host_callback) {
      host_callback->TriggerCallback(cookies);
      delete host_callback;
    }
    host_callback_queue_.pop();
  }
}
/* LCOV_EXCL_STOP */

bool CookieManager::GetGlobalAllowAccess() {
  AutoLock lock(lock_);
  if (EWK_COOKIE_ACCEPT_POLICY_ALWAYS == cookie_policy_)
    return true;
  else
    return false;  // LCOV_EXCL_LINE
}

void CookieManager::SetCookiePolicy(Ewk_Cookie_Accept_Policy policy) {
  AutoLock lock(lock_);
  cookie_policy_ = policy;
}

/* LCOV_EXCL_START */
bool CookieManager::ShouldBlockThirdPartyCookies() {
  AutoLock lock(lock_);
  if (EWK_COOKIE_ACCEPT_POLICY_NO_THIRD_PARTY == cookie_policy_)
    return true;
  else
    return false;
}
/* LCOV_EXCL_STOP */

bool CookieManager::AllowCookies(const GURL& url,
                                 const GURL& first_party_url,
                                 bool setting_cookie) {
  if (GetGlobalAllowAccess())
    return true;

  /* LCOV_EXCL_START */
  if (ShouldBlockThirdPartyCookies()) {
    net::StaticCookiePolicy policy(net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES);
    return (policy.CanAccessCookies(url, first_party_url) == net::OK);
  }
  /* LCOV_EXCL_STOP */

  return false;
}

/* LCOV_EXCL_START */
bool CookieManager::OnCanGetCookies(const net::URLRequest& request,
                                    const net::CookieList& cookie_list) {
  return AllowCookies(request.url(), request.site_for_cookies(), false);
}
/* LCOV_EXCL_STOP */

bool CookieManager::OnCanSetCookie(const net::URLRequest& request,
                                   const std::string& cookie_line,
                                   net::CookieOptions* options) {
  return AllowCookies(request.url(), request.site_for_cookies(), true);
}

bool CookieManager::AllowGetCookie(const GURL& url,
                                   const GURL& first_party,
                                   const net::CookieList& cookie_list,
                                   content::ResourceContext* context,
                                   int render_process_id,
                                   int render_frame_id) {
  return AllowCookies(url, first_party, false);
}

bool CookieManager::AllowSetCookie(const GURL& url,
                                   const GURL& first_party,
                                   const std::string& cookie_line,
                                   content::ResourceContext* context,
                                   int render_process_id,
                                   int render_frame_id,
                                   const net::CookieOptions& options) {
  return AllowCookies(url, first_party, true);
}

bool CookieManager::IsFileSchemeCookiesAllowed() {
  auto cookie_monster = static_cast<net::CookieMonster*>(GetCookieStore());
  if (!cookie_monster)
    return false;

  AutoLock lock(file_scheme_lock_);
  return cookie_monster->IsCookieableScheme(url::kFileScheme);
}

void CookieManager::SetAcceptFileSchemeCookies(bool accept) {
  auto cookie_monster = static_cast<net::CookieMonster*>(GetCookieStore());
  if (!cookie_monster)
    return;  // LCOV_EXCL_LINE

  std::vector<std::string> schemes(
      CookieMonster::kDefaultCookieableSchemes,
      CookieMonster::kDefaultCookieableSchemes +
          CookieMonster::kDefaultCookieableSchemesCount);
  if (accept)
    schemes.push_back(url::kFileScheme);

  AutoLock lock(file_scheme_lock_);
  cookie_monster->SetCookieableSchemes(schemes);
}

/* LCOV_EXCL_START */
static void SignalGetCookieValueCompleted(base::WaitableEvent* completion,
                                          std::string* result,
                                          const std::string& value) {
  DCHECK(completion);

  *result = value;
  completion->Signal();
}

static void GetCookieValueOnIOThread(net::CookieStore* cookie_store,
                                     const GURL& host,
                                     std::string* result,
                                     base::WaitableEvent* completion) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  net::CookieOptions options;
  options.set_include_httponly();

  if (cookie_store) {
    cookie_store->GetCookiesWithOptionsAsync(host,
                                             options,
                                             base::Bind(SignalGetCookieValueCompleted,
                                                        completion,
                                                        result));
  } else {
    DCHECK(completion);
    completion->Signal();
  }
}

std::string CookieManager::GetCookiesForURL(const std::string& url) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  net::CookieStore* cookie_store = GetCookieStore();
  if (!cookie_store)
    return std::string();

  std::string cookie_value;
  base::WaitableEvent completion(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                                 base::WaitableEvent::InitialState::NOT_SIGNALED);
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(GetCookieValueOnIOThread, cookie_store,
                                     GURL(url), &cookie_value, &completion));
  // allow wait temporarily.
  ScopedAllowWaitForLegacyWebViewApi allow_wait;
  completion.Wait();
  return cookie_value;
}

scoped_refptr<CookieManager> CookieManager::GetSharedThis() {
  return scoped_refptr<CookieManager>(this);
}
/* LCOV_EXCL_STOP */

scoped_refptr<content::URLRequestContextGetterEfl> CookieManager::GetContextGetter() const {
  return scoped_refptr<content::URLRequestContextGetterEfl>(request_context_getter_.get());
}

net::CookieStore* CookieManager::GetCookieStore() const {
  return request_context_getter_ ?
      request_context_getter_->GetURLRequestContext()->cookie_store() : nullptr;
}

#if defined(OS_TIZEN_TV_PRODUCT) || defined(TIZEN_DESKTOP_SUPPORT)
void CookieManager::SetCookiesChangedCallbackOnIOThread(
    Ewk_Cookie_Manager_Changes_Watch_Cb callback,
    void* data) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  auto cookie_monster = static_cast<net::CookieMonster*>(GetCookieStore());
  if (cookie_monster)
    cookie_monster->RegisterCookiesChangedCallback(callback, data);
}

void CookieManager::SetCookiesChangedCallback(
    Ewk_Cookie_Manager_Changes_Watch_Cb callback,
    void* data) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&CookieManager::SetCookiesChangedCallbackOnIOThread,
                 GetSharedThis(), callback, data));
}
#endif
