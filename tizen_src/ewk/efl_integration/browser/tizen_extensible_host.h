// Copyright 2016 Samsung Electronics. All rights reseoved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_TIZEN_EXTENSIBLE_HOST_H
#define EWK_EFL_INTEGRATION_BROWSER_TIZEN_EXTENSIBLE_HOST_H

#include <map>
#include <set>
#include <string>

#include "base/compiler_specific.h"
#include "base/memory/singleton.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "public/ewk_context.h"

class TizenExtensibleHost : public content::NotificationObserver {
 public:
  static TizenExtensibleHost* GetInstance();
  void Initialize();

  // content::NotificationObserver implementation:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;
  void UpdateTizenExtensible(int render_process_id);

  // Tizen Extensible API
  bool SetExtensibleAPI(const std::string& api_name, bool enable);
  bool GetExtensibleAPI(const std::string& api_name);

 private:
  TizenExtensibleHost();
  ~TizenExtensibleHost() override;

  content::NotificationRegistrar registrar_;
  std::set<int> renderers_;
  std::map<std::string, bool> extensible_api_table_;
  friend struct base::DefaultSingletonTraits<TizenExtensibleHost>;

  DISALLOW_COPY_AND_ASSIGN(TizenExtensibleHost);
};

#endif  // EWK_EFL_INTEGRATION_BROWSER_TIZEN_EXTENSIBLE_HOST_H
