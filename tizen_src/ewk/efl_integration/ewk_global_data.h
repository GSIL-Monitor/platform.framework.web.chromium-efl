// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_GLOBAL_DATA_H_
#define EWK_GLOBAL_DATA_H_

#include <base/macros.h>
#include <base/memory/ref_counted.h>
#include "url_request_context_getter_efl.h"

namespace content {
  class BrowserMainRunner;
  class ContentMainRunner;
  class ContentMainDelegateEfl;
}

class EwkGlobalData
{
 public:
  static EwkGlobalData* GetInstance();
  static void Delete();
  static bool IsInitialized();

  content::ContentMainDelegateEfl& GetContentMainDelegatEfl() const {
    return *content_main_delegate_efl_;
  }

  content::URLRequestContextGetterEfl* GetSystemRequestContextGetter();

 private:
  EwkGlobalData();
  ~EwkGlobalData();

  static EwkGlobalData* instance_;

  content::ContentMainRunner* content_main_runner_;
  content::BrowserMainRunner* browser_main_runner_;
  content::ContentMainDelegateEfl* content_main_delegate_efl_;
  scoped_refptr<content::URLRequestContextGetterEfl> system_request_context_;


  DISALLOW_COPY_AND_ASSIGN(EwkGlobalData);
};

#endif // EWK_GLOBAL_DATA_H_
