// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_favicon_database_private_h
#define ewk_favicon_database_private_h

#include "eweb_context.h"

class EWebContext;

class EwkFaviconDatabase {
 public:
  explicit EwkFaviconDatabase(EWebContext* eweb_context)
    : eweb_context_(eweb_context) {}

  /* LCOV_EXCL_START */
  Evas_Object* GetIcon(const char* page_url, Evas* evas) const {
    return eweb_context_->AddFaviconObject(page_url, evas);
  }
  /* LCOV_EXCL_STOP */

 private:
  EWebContext* eweb_context_;
};
#endif // ewk_favicon_database_private_h
