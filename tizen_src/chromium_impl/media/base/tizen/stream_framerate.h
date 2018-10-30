// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @file    stream_framerate.h
 * @brief   This file implement method to calculate stream framerate.
 */

#ifndef MEDIA_BASE_STREAM_FRAMERATE_H_
#define MEDIA_BASE_STREAM_FRAMERATE_H_

#include <stddef.h>
#include <stdint.h>
#include "media/base/media_export.h"

typedef signed long long int int64_tz;
typedef unsigned long long int uint64_tz;

namespace media {

class MEDIA_EXPORT StreamFramerate {
 public:
  // Framerate struct.
  struct Framerate {
    int num;
    int den;
    double toDouble() const { return den ? static_cast<double>(num) / den : 0; }
  };

  StreamFramerate(const int64_tz base_num, const int64_tz base_den);

  // Do reduction of a fraction to get reduced framerate_num and framerate_den.
  bool Reduce(int* dst_num,
              int* dst_den,
              int64_tz num,
              int64_tz den,
              int64_tz max);

  bool Calculate(int* framerate_num, int* framerate_den);

 private:
  int64_tz framerate_num_base_;
  int64_tz framerate_den_base_;
};

}  // namespace media

#endif  // MEDIA_BASE_STREAM_FRAMERATE_H_
