// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_security_origin_private.h"

#include <sstream>

/* LCOV_EXCL_START */
_Ewk_Security_Origin::_Ewk_Security_Origin(const GURL& url)
  : url_(url.GetOrigin()),
    host_(eina_stringshare_add(url.host().c_str())),
    protocol_(eina_stringshare_add(url.scheme().c_str())),
    port_((url.IntPort() == url::PORT_UNSPECIFIED)? 0: url.IntPort())
{
}

_Ewk_Security_Origin::~_Ewk_Security_Origin() {
  eina_stringshare_del(host_);
  eina_stringshare_del(protocol_);
}

GURL _Ewk_Security_Origin::GetURL() const {
  return url_;
}

Eina_Stringshare* _Ewk_Security_Origin::GetHost() const {
  return host_;
}

Eina_Stringshare* _Ewk_Security_Origin::GetProtocol() const {
  return protocol_;
}

int _Ewk_Security_Origin::GetPort() const {
  return port_;
}

_Ewk_Security_Origin* _Ewk_Security_Origin::CreateFromString(const char *url) {
  if (!url)
    return 0;

  GURL gurl(url);
  if (gurl.is_empty() || !gurl.is_valid())
    return 0;

  return new _Ewk_Security_Origin(gurl);
}
/* LCOV_EXCL_STOP */
