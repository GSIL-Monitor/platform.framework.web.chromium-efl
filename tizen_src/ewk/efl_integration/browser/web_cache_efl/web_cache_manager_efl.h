// Copyright 2014 Samsung Electronics. All rights reseoved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_CACHE_MANAGER_EFL_H
#define WEB_CACHE_MANAGER_EFL_H

#include <set>

#include "base/compiler_specific.h"
#include "base/memory/singleton.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "public/ewk_context.h"

namespace content {
class BrowserContext;
}

class WebCacheManagerEfl : public content::NotificationObserver {
 public:
  explicit WebCacheManagerEfl(content::BrowserContext* browser_context);
  virtual ~WebCacheManagerEfl();

  // content::NotificationObserver implementation:
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) override;
  void ClearCache();
  void SetCacheModel(Ewk_Cache_Model model);
  Ewk_Cache_Model GetCacheModel() const { return cache_model_; }
  void SetBrowserContext(content::BrowserContext* browser_context);

 private:
  int64_t GetCacheTotalCapacity(Ewk_Cache_Model);
  void SetRenderProcessCacheModel(Ewk_Cache_Model model, int render_process_id);

  content::NotificationRegistrar registrar_;
  std::set<int> renderers_;
  content::BrowserContext* browser_context_;
  Ewk_Cache_Model cache_model_;
  DISALLOW_COPY_AND_ASSIGN(WebCacheManagerEfl);
};

#endif

