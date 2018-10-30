// Copyright (C) 2012 Intel Corporation.
// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_error_private_h
#define ewk_error_private_h

#include <Eina.h>
#include "net/base/net_errors.h"

int ConvertErrorCode(int error_code);
const char* GetDescriptionFromErrorCode(int error_code);

struct _Ewk_Error {
  int error_code;
  bool is_cancellation;
  Eina_Stringshare* url;
  Eina_Stringshare* description;
  Eina_Stringshare* domain;

  /* LCOV_EXCL_START */
  _Ewk_Error(int error_code_in, bool is_cancellation_in, const char* url_in)
      : error_code(ConvertErrorCode(error_code_in)),
        is_cancellation(is_cancellation_in),
        url(eina_stringshare_add(url_in)),
        description(
            eina_stringshare_add(GetDescriptionFromErrorCode(error_code_in))),
        // Chromium always reports "net" as error domain anyways,
        // so we just hardcode it.
        domain(eina_stringshare_add("net")) {
  }

  ~_Ewk_Error() {
    eina_stringshare_del(url);
    eina_stringshare_del(description);
    eina_stringshare_del(domain);
  }
  /* LCOV_EXCL_STOP */
};

#endif // ewk_error_private_h
