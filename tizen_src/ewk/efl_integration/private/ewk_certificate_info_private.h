// Copyright 2016-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_certificate_info_private_h
#define ewk_certificate_info_private_h

struct _Ewk_Certificate_Info {
  const char* pem;
  bool is_context_secure;

  _Ewk_Certificate_Info(const char* p, bool is_secure)
      : pem(p),
        is_context_secure(is_secure) {
  }
  ~_Ewk_Certificate_Info() { }
};

#endif /* ewk_certificate_info_private_h */