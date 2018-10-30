// Copyright 2015 The Chromium Authors. All rights reserved.
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_SERVICE_FACTORY_EFL_H_
#define EWK_EFL_INTEGRATION_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_SERVICE_FACTORY_EFL_H_

#include "base/compiler_specific.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class PushMessagingServiceImplEfl;

class PushMessagingServiceFactoryEfl
    : public BrowserContextKeyedServiceFactory {
 public:
  static PushMessagingServiceImplEfl* GetForContext(
      content::BrowserContext* context);
  static PushMessagingServiceFactoryEfl* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<PushMessagingServiceFactoryEfl>;

  PushMessagingServiceFactoryEfl();
  ~PushMessagingServiceFactoryEfl() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(PushMessagingServiceFactoryEfl);
};

#endif  // EWK_EFL_INTEGRATION_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_SERVICE_FACTORY_EFL_H_
