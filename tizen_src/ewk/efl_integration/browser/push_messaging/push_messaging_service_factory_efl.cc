// Copyright 2015 The Chromium Authors. All rights reserved.
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/push_messaging/push_messaging_service_factory_efl.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/browser/push_messaging/push_messaging_service_impl_efl.h"
#include "ewk/efl_integration/browser_context_efl.h"

// static
PushMessagingServiceImplEfl* PushMessagingServiceFactoryEfl::GetForContext(
    content::BrowserContext* context) {
  // The Push API is not currently supported in incognito mode.
  // See https://crbug.com/401439.
  if (context->IsOffTheRecord())
    return nullptr;

  return static_cast<PushMessagingServiceImplEfl*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

// static
PushMessagingServiceFactoryEfl* PushMessagingServiceFactoryEfl::GetInstance() {
  return base::Singleton<PushMessagingServiceFactoryEfl>::get();
}

PushMessagingServiceFactoryEfl::PushMessagingServiceFactoryEfl()
    : BrowserContextKeyedServiceFactory(
          "PushMessagingProfileServiceEfl",
          BrowserContextDependencyManager::GetInstance()) {}

PushMessagingServiceFactoryEfl::~PushMessagingServiceFactoryEfl() {}

KeyedService* PushMessagingServiceFactoryEfl::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new PushMessagingServiceImplEfl(context);
}

content::BrowserContext* PushMessagingServiceFactoryEfl::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;  // FIXME probably something to do in incognito mode
}
