// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "url/url_constants.h"

namespace url {

const char kAboutBlankURL[] = "about:blank";

const char kAboutBlankPath[] = "blank";
const char kAboutBlankWithHashPath[] = "blank/";

const char kAboutScheme[] = "about";
const char kBlobScheme[] = "blob";
const char kContentScheme[] = "content";
const char kContentIDScheme[] = "cid";
const char kDataScheme[] = "data";
const char kFileScheme[] = "file";
const char kFileSystemScheme[] = "filesystem";
const char kFtpScheme[] = "ftp";
const char kGopherScheme[] = "gopher";
const char kHttpScheme[] = "http";
const char kHttpsScheme[] = "https";
const char kJavaScriptScheme[] = "javascript";
const char kMailToScheme[] = "mailto";
const char kWsScheme[] = "ws";
const char kWssScheme[] = "wss";
#if defined(OS_TIZEN_TV_PRODUCT)
const char kDvbScheme[] = "dvb";
const char kMmfScheme[] = "mmf";
const char kTVKeyScheme[] = "tvkey-apps";
const char kOpAppScheme[] = "hbbtv-package";
const char kHbbTVCarouselScheme[] = "hbbtv-carousel";
const char kCIScheme[] = "ci";
#endif

const char kHttpSuboriginScheme[] = "http-so";
const char kHttpsSuboriginScheme[] = "https-so";

const char kStandardSchemeSeparator[] = "://";

}  // namespace url
