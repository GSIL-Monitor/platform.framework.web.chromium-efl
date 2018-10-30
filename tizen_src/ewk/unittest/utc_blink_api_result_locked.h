// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UTC_BLINK_API_RESULT_LOCKED_H_
#define UTC_BLINK_API_RESULT_LOCKED_H_

#include <mutex>

template <typename T>
class ApiResultLocked {
 public:
  ApiResultLocked() : is_set_(false) {}

  void operator=(const T& t) { Set(t); }
  void Set(const T& t) {
    lock_.lock();
    result_ = t;
    is_set_ = true;
    lock_.unlock();
  }
  T Get() {
    lock_.lock();
    auto t = result_;
    lock_.unlock();
    return t;
  }
  bool IsSet() {
    lock_.lock();
    auto is_set = is_set_;
    lock_.unlock();
    return is_set;
  }

 private:
  bool is_set_;
  T result_;
  std::mutex lock_;
};

#endif  // UTC_BLINK_API_RESULT_LOCKED_H_
