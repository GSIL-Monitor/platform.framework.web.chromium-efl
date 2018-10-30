// Copyright (C) 2012 Intel Corporation. All rights reserved.
// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_private_h
#define ewk_private_h

#include <tizen.h>

#include "base/logging.h"

#define COMPILE_ASSERT_MATCHING_ENUM(ewkName, webcoreName) \
        COMPILE_ASSERT(int(ewkName) == int(webcoreName), mismatchingEnums)

#define COMPILE_ASSERT_MATCHING_ENUM(ewkName, webcoreName) \
        COMPILE_ASSERT(int(ewkName) == int(webcoreName), mismatchingEnums)

// Temporarily added in order to track not-yet-implemented ewk api calls.
#define LOG_EWK_API_MOCKUP(msg)                                          \
  LOG(INFO) << "[EWK_API_MOCKUP] "                                       \
            << " " << __FUNCTION__ << std::string(msg);

#define LOG_EWK_API_DEPRECATED(msg)                                       \
  LOG(WARNING) << "DEPRECATION WARNING: " << __FUNCTION__                 \
               << " is deprecated and will be removed from next release." \
               << std::string(msg);
#endif
