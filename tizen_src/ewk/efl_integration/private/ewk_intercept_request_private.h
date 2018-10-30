// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_PRIVATE_EWK_INTERCEPT_REQUEST_PRIVATE_H_
#define EWK_EFL_INTEGRATION_PRIVATE_EWK_INTERCEPT_REQUEST_PRIVATE_H_

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <Eina.h>

#include "base/synchronization/cancellation_flag.h"

namespace net {
class URLRequest;
}

class URLRequestJobEFL;

struct _Ewk_Intercept_Request {
 public:
  explicit _Ewk_Intercept_Request(net::URLRequest* request);
  ~_Ewk_Intercept_Request();

  /* LCOV_EXCL_START */
  // functions so user can use EWK API to get info about intercepted request
  // to make a decision
  const char* request_scheme_get() const { return request_scheme_.c_str(); }
  const char* request_url_get() const { return request_url_.c_str(); }
  const char* request_http_method_get() const {
    return request_http_method_.c_str();
  }
  /* LCOV_EXCL_STOP */

  const Eina_Hash* request_headers_get() const { return request_headers_; }
  const char* request_body_get() const;
  int64_t request_body_length_get() const;

  // functions so user can use EWK API to write response for intercepted
  // request
  Eina_Bool response_set(const char* headers, const void* data, size_t length);
  Eina_Bool request_ignore();
  Eina_Bool response_status_set(int status_code,
                                const char* custom_status_text);
  Eina_Bool response_header_add(const char* field_name,
                                const char* field_value);
  Eina_Bool response_header_map_add(const Eina_Hash* headers);
  Eina_Bool response_body_set(const void* data, size_t length);
  Eina_Bool response_write_chunk(const void* data, size_t length);

  /* LCOV_EXCL_START */
  // functions so URLRequestJobEFL can get info about headers and body of
  // custom response
  std::string&& response_headers_take() { return std::move(response_headers_); }
  std::unique_ptr<char[]> response_body_take() {
    return std::move(response_body_);
  }
  /* LCOV_EXCL_STOP */
  size_t response_body_length_get() const { return response_body_length_; }
  bool is_ignored() const { return ignored_; }
  bool is_chunked_write() const { return chunked_write_; }
  /* LCOV_EXCL_START */
  void request_early_exit() { chunked_write_early_exit_.Set(); }
  void set_job(URLRequestJobEFL* job) { job_ = job; }
  void callback_ended() { request_ = nullptr; }
  /* LCOV_EXCL_STOP */

 private:
  void set_data(const void* data, size_t length);
  void headers_generate();
  void response_decided();
  void post_response_decided();
  void put_chunk(const char* data, size_t length);
  void post_put_chunk(const char* data, size_t length);
  void chunked_read_done();
  void post_chunked_read_done();

  URLRequestJobEFL* job_;

  // request_ is valid only during the callback. It is saved to allow
  // retrieving request body. We don't save request's body every time in
  // constructor (like other data), because data uploaded in request body may
  // have excessive size.
  net::URLRequest* request_;

  std::string request_scheme_;
  std::string request_url_;
  std::string request_http_method_;
  Eina_Hash* request_headers_;
  // mutable because request_body_ is lazily loaded in its const getter
  mutable std::string request_body_;
  // mutable because request_body_size_ is lazily loaded in its const getter
  mutable int64_t request_body_length_;

  std::string response_headers_;
  std::unique_ptr<char[]> response_body_;
  size_t response_body_length_;
  int response_status_code_;
  std::string response_status_text_;
  std::vector<std::pair<std::string, std::string>> response_headers_map_;

  bool ignored_;
  bool chunked_write_;
  base::CancellationFlag chunked_write_early_exit_;
};

#endif  // EWK_EFL_INTEGRATION_PRIVATE_EWK_INTERCEPT_REQUEST_PRIVATE_H_
