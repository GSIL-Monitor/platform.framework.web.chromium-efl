// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_security_origin_private_h
#define ewk_security_origin_private_h

#include <Eina.h>
#include <Evas.h>
#include <assert.h>
#include <sstream>

#include "base/callback.h"
#include "url/gurl.h"

// chromium-efl implementation for _Ewk_Security_Origin
struct _Ewk_Security_Origin
{
 public:
  _Ewk_Security_Origin(const GURL& url);
  ~_Ewk_Security_Origin();

  GURL GetURL() const;
  Eina_Stringshare* GetHost() const;
  Eina_Stringshare* GetProtocol() const;
  int GetPort() const;

  static _Ewk_Security_Origin* CreateFromString(const char* url);

 private:
  const GURL url_;
  const Eina_Stringshare* host_;
  const Eina_Stringshare* protocol_;
  const int port_;

  DISALLOW_COPY_AND_ASSIGN(_Ewk_Security_Origin);
};

#endif // ewk_security_origin_private_h
