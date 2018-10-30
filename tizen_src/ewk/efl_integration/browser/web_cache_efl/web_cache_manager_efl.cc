// Copyright 2014 Samsung Electronics. All rights reseoved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/web_cache_efl/web_cache_manager_efl.h"

#include "base/logging.h"
#include "base/sys_info.h"
#include "common/render_messages_ewk.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"

WebCacheManagerEfl::WebCacheManagerEfl(content::BrowserContext* browser_context)
    : browser_context_(browser_context),
      cache_model_(EWK_CACHE_MODEL_DOCUMENT_VIEWER) {
  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_CREATED,
                 content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_TERMINATED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

WebCacheManagerEfl::~WebCacheManagerEfl() { }

void WebCacheManagerEfl::Observe(int type,
                                 const content::NotificationSource& source,
                                 const content::NotificationDetails& details) {
  content::RenderProcessHost* process =
      content::Source<content::RenderProcessHost>(source).ptr();
  DCHECK(process);
  if (process->GetBrowserContext() != browser_context_)
    return;  // LCOV_EXCL_LINE

  int renderer_id = process->GetID();
  switch (type) {
    case content::NOTIFICATION_RENDERER_PROCESS_CREATED: {
      renderers_.insert(renderer_id);
      SetRenderProcessCacheModel(cache_model_, renderer_id);
      break;
    }
    case content::NOTIFICATION_RENDERER_PROCESS_TERMINATED: {
      renderers_.erase(renderer_id);
      break;
    }
    default:
      NOTREACHED();
      break;
  }
}

void WebCacheManagerEfl::ClearCache() {
  for (std::set<int>::const_iterator iter = renderers_.begin();
       iter != renderers_.end(); ++iter) {
    /* LCOV_EXCL_START */
    content::RenderProcessHost* host =
        content::RenderProcessHost::FromID(*iter);
    if (host)
      host->Send(new EflViewMsg_ClearCache());
    /* LCOV_EXCL_STOP */
  }
}

void WebCacheManagerEfl::SetRenderProcessCacheModel(Ewk_Cache_Model model,
                                                    int render_process_id) {
  DCHECK(render_process_id);
  DCHECK(renderers_.find(render_process_id) != renderers_.end());
  content::RenderProcessHost* host =
      content::RenderProcessHost::FromID(render_process_id);
  if (host)
    host->Send(new EflViewMsg_SetCache(GetCacheTotalCapacity(model)));
}

void WebCacheManagerEfl::SetCacheModel(Ewk_Cache_Model model) {
  cache_model_ = model;
  int64_t cache_total_capacity = GetCacheTotalCapacity(model);
  for (std::set<int>::const_iterator iter = renderers_.begin();
       iter != renderers_.end(); ++iter) {
    /* LCOV_EXCL_START */
    content::RenderProcessHost* host =
        content::RenderProcessHost::FromID(*iter);
    if (host)
      host->Send(new EflViewMsg_SetCache(cache_total_capacity));
    /* LCOV_EXCL_STOP */
  }
}

int64_t WebCacheManagerEfl::GetCacheTotalCapacity(Ewk_Cache_Model cache_model) {
  int64_t memory_size = base::SysInfo::AmountOfPhysicalMemory();

  switch (cache_model) {
    case EWK_CACHE_MODEL_DOCUMENT_VIEWER:
      // Object cache capacities (in bytes)
      if (memory_size >= 2048)
        return 96 * 1024 * 1024;
      else if (memory_size >= 1536)
        return 64 * 1024 * 1024;
      else if (memory_size >= 1024)
        return 32 * 1024 * 1024;
      else if (memory_size >= 512)
        return 16 * 1024 * 1024;
      break;

    case EWK_CACHE_MODEL_DOCUMENT_BROWSER:
      // Object cache capacities (in bytes)
      if (memory_size >= 2048)
        return 96 * 1024 * 1024;
      else if (memory_size >= 1536)
        return 64 * 1024 * 1024;
      else if (memory_size >= 1024)
        return 32 * 1024 * 1024;
      else if (memory_size >= 512)
        return 16 * 1024 * 1024;
      break;

    case EWK_CACHE_MODEL_PRIMARY_WEBBROWSER:
      // Object cache capacities (in bytes)
      // (Testing indicates that value / MB depends heavily on content and
      // browsing pattern. Even growth above 128MB can have substantial
      // value / MB for some content / browsing patterns.)
      if (memory_size >= 2048)
        return 128 * 1024 * 1024;
      else if (memory_size >= 1536)
        return 96 * 1024 * 1024;
      else if (memory_size >= 1024)
        return 64 * 1024 * 1024;
      else if (memory_size >= 512)
        return 32 * 1024 * 1024;
      break;

    default:
      NOTREACHED();
      break;
  };

  return 0;
}

void WebCacheManagerEfl::SetBrowserContext(
    content::BrowserContext* browser_context) {
  DCHECK(!browser_context_);
  browser_context_ = browser_context;
}
/* LCOV_EXCL_STOP */
