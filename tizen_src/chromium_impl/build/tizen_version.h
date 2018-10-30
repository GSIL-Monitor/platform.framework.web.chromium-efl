// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Tizen_Version_h
#define Tizen_Version_h

#if defined(OS_TIZEN)

#define TIZEN_VERSION (TIZEN_VERSION_MAJOR * 10000 + TIZEN_VERSION_MINOR * 100 + TIZEN_VERSION_PATCH)
#define TIZEN_VERSION_EQ(major, minor, patch) \
  (TIZEN_VERSION == (major * 10000 + minor * 100 + patch))
#define TIZEN_VERSION_AT_LEAST(major, minor, patch) \
  (TIZEN_VERSION >= (major * 10000 + minor * 100 + patch))

#else

#define TIZEN_VERSION 0
#define TIZEN_VERSION_EQ(major, minor, patch) 0
#define TIZEN_VERSION_AT_LEAST(major, minor, patch) 0

#endif // defined(OS_TIZEN)

#endif // Tizen_Version_h
