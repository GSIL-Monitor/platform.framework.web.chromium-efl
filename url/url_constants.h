// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef URL_URL_CONSTANTS_H_
#define URL_URL_CONSTANTS_H_

#include <stddef.h>

#include "url/url_export.h"

namespace url {

URL_EXPORT extern const char kAboutBlankURL[];

URL_EXPORT extern const char kAboutBlankPath[];
URL_EXPORT extern const char kAboutBlankWithHashPath[];

URL_EXPORT extern const char kAboutScheme[];
URL_EXPORT extern const char kBlobScheme[];
// The content scheme is specific to Android for identifying a stored file.
URL_EXPORT extern const char kContentScheme[];
URL_EXPORT extern const char kContentIDScheme[];
URL_EXPORT extern const char kDataScheme[];
URL_EXPORT extern const char kFileScheme[];
URL_EXPORT extern const char kFileSystemScheme[];
URL_EXPORT extern const char kFtpScheme[];
URL_EXPORT extern const char kGopherScheme[];
URL_EXPORT extern const char kHttpScheme[];
URL_EXPORT extern const char kHttpsScheme[];
URL_EXPORT extern const char kJavaScriptScheme[];
URL_EXPORT extern const char kMailToScheme[];
URL_EXPORT extern const char kWsScheme[];
URL_EXPORT extern const char kWssScheme[];
#if defined(OS_TIZEN_TV_PRODUCT)
URL_EXPORT extern const char kDvbScheme[];
URL_EXPORT extern const char kMmfScheme[];
URL_EXPORT extern const char kTVKeyScheme[];
URL_EXPORT extern const char kOpAppScheme[];
URL_EXPORT extern const char kHbbTVCarouselScheme[];
URL_EXPORT extern const char kCIScheme[];
#ifndef URL_URL_UTIL_H_
URL_EXPORT extern size_t GetMaxURLChars();
#endif
#endif

// Special HTTP and HTTPS schemes for serialization of suborigins. See
// https://w3c.github.io/webappsec-suborigins/.
URL_EXPORT extern const char kHttpSuboriginScheme[];
URL_EXPORT extern const char kHttpsSuboriginScheme[];

// Used to separate a standard scheme and the hostname: "://".
URL_EXPORT extern const char kStandardSchemeSeparator[];

#if defined(OS_TIZEN_TV_PRODUCT)
#define kMaxURLChars GetMaxURLChars()
#else
constexpr int kMaxURLChars = 2 * 1024 * 1024;
#endif
}  // namespace url

#endif  // URL_URL_CONSTANTS_H_
