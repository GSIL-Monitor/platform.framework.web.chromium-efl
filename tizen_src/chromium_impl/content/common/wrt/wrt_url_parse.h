// Copyright (c) 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WRT_URL_PARSE_H
#define WRT_URL_PARSE_H

#include "url/gurl.h"

namespace content {

class WrtUrlParseBase {
 public:
  virtual GURL parseUrl(const GURL& url) const = 0;
  virtual ~WrtUrlParseBase() {}
};

} // namespace content

#endif // WRT_URL_PARSE_H
