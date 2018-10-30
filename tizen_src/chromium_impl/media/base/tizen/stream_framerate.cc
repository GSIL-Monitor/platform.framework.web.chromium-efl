// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @file    stream_framerate.cc
 * @brief   This file implement method to calculate stream framerate.
 */

#include "tizen_src/chromium_impl/media/base/tizen/stream_framerate.h"

const int64_tz kMaxFramerateReduceNum = 30000;

#define FFABS(a) ((a) >= 0 ? (a) : (-(a)))
#define FFMIN(a, b) ((a) > (b) ? (b) : (a))

static int64_tz inline av_gcd(int64_tz a, int64_tz b) {
  if (b)
    return av_gcd(b, a % b);
  else
    return a;
}

namespace media {

StreamFramerate::StreamFramerate(const int64_tz base_num,
                                 const int64_tz base_den) {
  framerate_num_base_ = base_num;
  framerate_den_base_ = base_den;
}

bool StreamFramerate::Calculate(int* framerate_num, int* framerate_den) {
  // Calculate reduced framerate_num and framerate_den.
  return Reduce(framerate_num, framerate_den, framerate_num_base_,
                framerate_den_base_, kMaxFramerateReduceNum);
}

bool StreamFramerate::Reduce(int* dst_num,
                             int* dst_den,
                             int64_tz num,
                             int64_tz den,
                             int64_tz max) {
  // Do reduction of a fraction to get reduced framerate_num and framerate_den.
  Framerate a0 = {0, 1}, a1 = {1, 0};
  int sign = (num < 0) ^ (den < 0);
  int64_tz gcd = av_gcd(FFABS(num), FFABS(den));

  if (gcd) {
    num = FFABS(num) / gcd;
    den = FFABS(den) / gcd;
  }
  if (num <= max && den <= max) {
    a1 = (Framerate){num, den};
    den = 0;
  }

  while (den) {
    uint64_tz x = num / den;
    int64_tz next_den = num - den * x;
    int64_tz a2n = x * a1.num + a0.num;
    int64_tz a2d = x * a1.den + a0.den;

    if (a2n > max || a2d > max) {
      if (a1.num)
        x = (max - a0.num) / a1.num;
      if (a1.den)
        x = FFMIN((int64_tz)x, (max - a0.den) / a1.den);

      if (den * (2 * (int64_tz)x * a1.den + a0.den) > num * a1.den)
        a1 = (Framerate){x * a1.num + a0.num, x * a1.den + a0.den};
      break;
    }

    a0 = a1;
    a1 = (Framerate){a2n, a2d};
    num = den;
    den = next_den;
  }

  *dst_num = sign ? -a1.num : a1.num;
  *dst_den = a1.den;

  return den == 0;
}

}  // namespace media
