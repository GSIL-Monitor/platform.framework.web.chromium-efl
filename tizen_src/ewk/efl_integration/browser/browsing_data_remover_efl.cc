// Copyright 2014 Samsung Eaectronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/browsing_data_remover_efl.h"

#include "base/bind.h"
#include "base/logging.h"

#include "content/browser/appcache/appcache_service_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/local_storage_usage_info.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/session_storage_usage_info.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/completion_callback.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "storage/browser/quota/quota_manager.h"
#include "storage/common/quota/quota_types.h"

using content::BrowserThread;

// Static.
BrowsingDataRemoverEfl* BrowsingDataRemoverEfl::CreateForUnboundedRange(content::BrowserContext* profile) {
  return new BrowsingDataRemoverEfl(profile, base::Time(), base::Time::Max());
}

/* LCOV_EXCL_START */
BrowsingDataRemoverEfl* BrowsingDataRemoverEfl::CreateForRange(content::BrowserContext* browser_context,
  base::Time start, base::Time end) {
  return new BrowsingDataRemoverEfl(browser_context, start, end);
}

int BrowsingDataRemoverEfl::GenerateQuotaClientMask(int remove_mask) {
  int quota_client_mask = 0;
  if (remove_mask & BrowsingDataRemoverEfl::REMOVE_FILE_SYSTEMS)
    quota_client_mask |= storage::QuotaClient::kFileSystem;
  if (remove_mask & BrowsingDataRemoverEfl::REMOVE_WEBSQL)
    quota_client_mask |= storage::QuotaClient::kDatabase;
  if (remove_mask & BrowsingDataRemoverEfl::REMOVE_INDEXEDDB)
    quota_client_mask |= storage::QuotaClient::kIndexedDatabase;

  return quota_client_mask;
}
/* LCOV_EXCL_STOP */

BrowsingDataRemoverEfl::BrowsingDataRemoverEfl(
    content::BrowserContext* browser_context,
    base::Time delete_begin,
    base::Time delete_end)
    : browser_context_(browser_context),
      app_cache_service_(nullptr),
      quota_manager_(nullptr),
      dom_storage_context_(nullptr),
      delete_begin_(delete_begin),
      delete_end_(delete_end),
      next_cache_state_(STATE_NONE),
      cache_(nullptr),
      main_context_getter_(nullptr),
      media_context_getter_(nullptr),
      waiting_for_clear_cache_(false),
      waiting_for_clear_local_storage_(false),
      waiting_for_clear_session_storage_(false),
      waiting_for_clear_quota_managed_data_(false),
      quota_managed_origins_to_delete_count_(0),
      quota_managed_storage_types_to_delete_count_(0),
      remove_mask_(0) {
  if (browser_context_) {
    app_cache_service_ = browser_context->GetStoragePartition(browser_context_, NULL)->GetAppCacheService();
    main_context_getter_ =
      content::BrowserContext::GetDefaultStoragePartition(browser_context_)->GetURLRequestContext();
    media_context_getter_ = browser_context->CreateMediaRequestContext();
  }
}

/* LCOV_EXCL_START */
BrowsingDataRemoverEfl::~BrowsingDataRemoverEfl() {
  DCHECK(AllDone());
}
/* LCOV_EXCL_STOP */

void BrowsingDataRemoverEfl::ClearNetworkCache() {
  waiting_for_clear_cache_ = true;
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  content::BrowserThread::PostTask(
    content::BrowserThread::IO, FROM_HERE,
    base::Bind(&BrowsingDataRemoverEfl::ClearNetworkCacheOnIOThread,
      base::Unretained(this)));
}

void BrowsingDataRemoverEfl::ClearNetworkCacheOnIOThread() {
  // This function should be called on the IO thread.
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  DCHECK_EQ(STATE_NONE, next_cache_state_);
  DCHECK(main_context_getter_.get());
  DCHECK(media_context_getter_.get());

  next_cache_state_ = STATE_CREATE_MAIN;
  DoClearCache(net::OK);
}

// The expected state sequence is STATE_NONE --> STATE_CREATE_MAIN -->
// STATE_DELETE_MAIN --> STATE_CREATE_MEDIA --> STATE_DELETE_MEDIA -->
// STATE_DONE, and any errors are ignored.
void BrowsingDataRemoverEfl::DoClearCache(int rv)  {
  DCHECK_NE(STATE_NONE, next_cache_state_);

  while (rv != net::ERR_IO_PENDING && next_cache_state_ != STATE_NONE) {
    switch (next_cache_state_) {
      case STATE_CREATE_MAIN:
      case STATE_CREATE_MEDIA: {
        // Get a pointer to the cache.
        net::URLRequestContextGetter* getter = nullptr;
        if (next_cache_state_ == STATE_CREATE_MAIN) {
          if (main_context_getter_)
            getter = main_context_getter_.get();
        } else {
          if (media_context_getter_)
            getter = media_context_getter_.get();
        }
        if (getter && getter->GetURLRequestContext()) {
          net::HttpTransactionFactory* factory =
              getter->GetURLRequestContext()->http_transaction_factory();
          if (factory) {
            next_cache_state_ = (next_cache_state_ == STATE_CREATE_MAIN)
                                    ? STATE_DELETE_MAIN
                                    : STATE_DELETE_MEDIA;
            rv = factory->GetCache()->GetBackend(
                &cache_, base::Bind(&BrowsingDataRemoverEfl::DoClearCache,
                                    base::Unretained(this)));
          } else {
            LOG(ERROR)
                << "Could not get HttpTransactionFactory.";  // LCOV_EXCL_LINE
            next_cache_state_ = STATE_NONE;                  // LCOV_EXCL_LINE
          }
        } else {
          /* LCOV_EXCL_START */
          LOG(ERROR) << "Could not get URLRequestContext.";
          next_cache_state_ = STATE_NONE;
          /* LCOV_EXCL_STOP */
        }
        break;
      }
      case STATE_DELETE_MAIN:
      case STATE_DELETE_MEDIA: {
        next_cache_state_ = (next_cache_state_ == STATE_DELETE_MAIN) ?
                            STATE_CREATE_MEDIA : STATE_DONE;

        // |cache_| can be null if it cannot be initialized.
        if (cache_) {
          if (delete_begin_.is_null()) {
            rv = cache_->DoomAllEntries(
                base::Bind(&BrowsingDataRemoverEfl::DoClearCache,
                           base::Unretained(this)));
          } else {
            rv = cache_->DoomEntriesBetween(
                delete_begin_, delete_end_,
                base::Bind(&BrowsingDataRemoverEfl::DoClearCache,
                           base::Unretained(this)));  // LCOV_EXCL_LINE
          }
          cache_ = NULL;
        }
        break;
      }
      /* LCOV_EXCL_START */
      case STATE_DONE: {
        cache_ = NULL;
        next_cache_state_ = STATE_NONE;

        // Notify the UI thread that we are done.
        content::BrowserThread::PostTask(
            content::BrowserThread::UI, FROM_HERE,
            base::Bind(&BrowsingDataRemoverEfl::ClearedCache,
                       base::Unretained(this)));
        return;
      }
      default: {
        NOTREACHED() << "bad state";
        next_cache_state_ = STATE_NONE;  // Stop looping.
        return;
      }
      /* LCOV_EXCL_STOP */
    }
  }
}

/* LCOV_EXCL_START */
void BrowsingDataRemoverEfl::ClearedCache() {
  waiting_for_clear_cache_ = false;
  DeleteIfDone();
}

// just to keep same overall structure of Chrome::BrowsingDataRemover
bool BrowsingDataRemoverEfl::AllDone() {
  return !waiting_for_clear_cache_ &&
         !waiting_for_clear_local_storage_ &&
         !waiting_for_clear_session_storage_ &&
         !waiting_for_clear_quota_managed_data_;
}

void BrowsingDataRemoverEfl::DeleteIfDone() {
  if (!AllDone())
    return;

  // we can delete ourselves, but following chromium we do delete later.
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void BrowsingDataRemoverEfl::OnGotOriginsWithApplicationCache(
    scoped_refptr<content::AppCacheInfoCollection> collection,
    int /*result*/) {
  typedef std::map<GURL, content::AppCacheInfoVector> InfoByOrigin;
  InfoByOrigin& origin_map = collection->infos_by_origin;
  for (InfoByOrigin::iterator iter = origin_map.begin();
       iter != origin_map.end(); ++iter) {
    DeleteAppCachesForOrigin(iter->first);
  }
}
/* LCOV_EXCL_STOP */

void BrowsingDataRemoverEfl::RemoveImpl(int remove_mask,
                                        const GURL& origin) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  remove_mask_ = remove_mask;
  remove_origin_ = origin;

  if (remove_mask & REMOVE_LOCAL_STORAGE) {
    waiting_for_clear_local_storage_ = true;
    if (!dom_storage_context_) {
      dom_storage_context_ = content::BrowserContext::GetStoragePartition(browser_context_, NULL)->GetDOMStorageContext();
    }
    ClearLocalStorageOnUIThread();
  }

  if (remove_mask & REMOVE_SESSION_STORAGE) {
    waiting_for_clear_session_storage_ = true;
    if (!dom_storage_context_) {
      dom_storage_context_ =
          content::BrowserContext::GetStoragePartition(
              browser_context_, nullptr)->GetDOMStorageContext();
    }
    ClearSessionStorageOnUIThread();
  }

  if (remove_mask & REMOVE_INDEXEDDB || remove_mask & REMOVE_WEBSQL ||
      remove_mask & REMOVE_FILE_SYSTEMS) {
    if (!quota_manager_) {
      quota_manager_ = content::BrowserContext::GetStoragePartition(browser_context_, NULL)->GetQuotaManager();
    }
    waiting_for_clear_quota_managed_data_ = true;
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(&BrowsingDataRemoverEfl::ClearQuotaManagedDataOnIOThread,
                   base::Unretained(this)));
  }
  /* LCOV_EXCL_START */
  if (remove_mask & REMOVE_APPCACHE) {
    if (origin.is_valid())
      DeleteAppCachesForOrigin(origin);
    else
      DeleteAllAppCaches();
  }
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
void BrowsingDataRemoverEfl::DeleteAllAppCaches() {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    // TODO: Using base::Unretained is not thread safe
    // It may happen that on IO thread this ptr will be already deleted
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&BrowsingDataRemoverEfl::DeleteAllAppCaches,
                   base::Unretained(this)));
    return;
  }

  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!app_cache_service_)
    return;

  // Actual deletion in OnGotOriginsWithApplicationCache
  scoped_refptr<content::AppCacheInfoCollection> collection =
      new content::AppCacheInfoCollection();
  app_cache_service_->GetAllAppCacheInfo(
      collection.get(),
      base::Bind(&BrowsingDataRemoverEfl::OnGotOriginsWithApplicationCache,
                 base::Unretained(this), collection));
}

void BrowsingDataRemoverEfl::DeleteAppCachesForOrigin(const GURL& origin) {
  if (BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    // TODO: Using base::Unretained is not thread safe
    // It may happen that on IO thread this ptr will be already deleted
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&BrowsingDataRemoverEfl::DeleteAppCachesForOrigin,
                   base::Unretained(this), origin));
    return;
  }

  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!app_cache_service_)
    return;

  static_cast<content::AppCacheServiceImpl*>(app_cache_service_)->
      DeleteAppCachesForOrigin(origin, net::CompletionCallback());
}
/* LCOV_EXCL_STOP */

void BrowsingDataRemoverEfl::ClearLocalStorageOnUIThread() {
  DCHECK(waiting_for_clear_local_storage_);
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  dom_storage_context_->GetLocalStorageUsage(
    base::Bind(&BrowsingDataRemoverEfl::OnGotLocalStorageUsageInfo,
               base::Unretained(this)));
}

/* LCOV_EXCL_START */
void BrowsingDataRemoverEfl::OnGotLocalStorageUsageInfo(
  const std::vector<content::LocalStorageUsageInfo>& infos) {
  DCHECK(waiting_for_clear_local_storage_);
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  for (size_t i = 0; i < infos.size(); ++i) {
    if (infos[i].last_modified >= delete_begin_
        && infos[i].last_modified <= delete_end_)
      dom_storage_context_->DeleteLocalStorage(infos[i].origin);
  }
  waiting_for_clear_local_storage_ = false;
  DeleteIfDone();
}
/* LCOV_EXCL_STOP */

void BrowsingDataRemoverEfl::ClearSessionStorageOnUIThread() {
  DCHECK(waiting_for_clear_session_storage_);
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  dom_storage_context_->GetSessionStorageUsage(
      base::Bind(&BrowsingDataRemoverEfl::OnGotSessionStorageUsageInfo,
                 base::Unretained(this)));
}

void BrowsingDataRemoverEfl::OnGotSessionStorageUsageInfo(
  const std::vector<content::SessionStorageUsageInfo>& infos) {
  DCHECK(waiting_for_clear_session_storage_);
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  for (size_t i = 0; i < infos.size(); ++i)
    dom_storage_context_->DeleteSessionStorage(infos[i]);

  waiting_for_clear_session_storage_ = false;
  DeleteIfDone();
}

void BrowsingDataRemoverEfl::ClearQuotaManagedDataOnIOThread() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

  // Ask the QuotaManager for all origins with temporary quota modified within
  // the user-specified timeframe, and deal with the resulting set in
  // OnGotQuotaManagedOrigins().
  quota_managed_origins_to_delete_count_ = 0;
  quota_managed_storage_types_to_delete_count_ = 0;

  if (delete_begin_ == base::Time()) {
    ++quota_managed_storage_types_to_delete_count_;
    quota_manager_->GetOriginsModifiedSince(
      storage::kStorageTypePersistent, delete_begin_,
      base::Bind(&BrowsingDataRemoverEfl::OnGotQuotaManagedOrigins,
                 base::Unretained(this)));
  }

  // Do the same for temporary quota.
  ++quota_managed_storage_types_to_delete_count_;
  quota_manager_->GetOriginsModifiedSince(
    storage::kStorageTypeTemporary, delete_begin_,
    base::Bind(&BrowsingDataRemoverEfl::OnGotQuotaManagedOrigins,
               base::Unretained(this)));

  // Do the same for syncable quota.
  ++quota_managed_storage_types_to_delete_count_;
  quota_manager_->GetOriginsModifiedSince(
    storage::kStorageTypeSyncable, delete_begin_,
    base::Bind(&BrowsingDataRemoverEfl::OnGotQuotaManagedOrigins,
               base::Unretained(this)));
}

/* LCOV_EXCL_START */
void BrowsingDataRemoverEfl::OnGotQuotaManagedOrigins(
  const std::set<GURL>& origins, storage::StorageType type) {
  DCHECK_GT(quota_managed_storage_types_to_delete_count_, 0);

  // Walk through the origins passed in, delete quota of |type| from each that
  // matches the |origin_set_mask_|.
  std::set<GURL>::const_iterator origin;
  for (origin = origins.begin(); origin != origins.end(); ++origin) {
    if (!remove_origin_.is_empty()) { // delete all origins if remove_origin is empty
      if (remove_origin_ != origin->GetOrigin())
        continue;
    }
    ++quota_managed_origins_to_delete_count_;
    quota_manager_->DeleteOriginData(
      origin->GetOrigin(), type,
      BrowsingDataRemoverEfl::GenerateQuotaClientMask(remove_mask_),
      base::Bind(&BrowsingDataRemoverEfl::OnQuotaManagedOriginDeletion,
                 base::Unretained(this), origin->GetOrigin(), type));
  }

  --quota_managed_storage_types_to_delete_count_;
  CheckQuotaManagedDataDeletionStatus();
}

void BrowsingDataRemoverEfl::OnQuotaManagedOriginDeletion(
  const GURL& origin,
  storage::StorageType type,
  storage::QuotaStatusCode status) {
  DCHECK_GT(quota_managed_origins_to_delete_count_, 0);

  if (status != storage::kQuotaStatusOk)
    DLOG(ERROR) << "Couldn't remove data of type " << type << " for origin "
                << origin << ". Status: " << status;

  --quota_managed_origins_to_delete_count_;  // LCOV_EXCL_LINE
  CheckQuotaManagedDataDeletionStatus();     // LCOV_EXCL_LINE
}

void BrowsingDataRemoverEfl::CheckQuotaManagedDataDeletionStatus() {
  if (quota_managed_storage_types_to_delete_count_ != 0 ||
      quota_managed_origins_to_delete_count_ != 0)
  return;

  content::BrowserThread::PostTask(
    content::BrowserThread::UI, FROM_HERE,
    base::Bind(&BrowsingDataRemoverEfl::OnQuotaManagedDataDeleted,
               base::Unretained(this)));
}

void BrowsingDataRemoverEfl::OnQuotaManagedDataDeleted() {
  DCHECK(waiting_for_clear_quota_managed_data_);

  waiting_for_clear_quota_managed_data_ = false;
  DeleteIfDone();
}
/* LCOV_EXCL_STOP */
