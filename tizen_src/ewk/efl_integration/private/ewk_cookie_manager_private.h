// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_cookie_manager_private_h
#define ewk_cookie_manager_private_h

#include "cookie_manager.h"

class Ewk_Cookie_Manager {
 public:
  static Ewk_Cookie_Manager* create() {
    return new Ewk_Cookie_Manager;
  }

  ~Ewk_Cookie_Manager() { }

  scoped_refptr<CookieManager> cookieManager() const {
    return cookie_manager_;
  }

 private:
  Ewk_Cookie_Manager()
    : cookie_manager_(new CookieManager) {
  }

  scoped_refptr<CookieManager> cookie_manager_;
};

#endif // ewk_cookie_manager_private_h
