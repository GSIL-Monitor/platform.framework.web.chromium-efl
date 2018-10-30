// Copyright 2012 The Chromium Authors. All rights reserved.
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_URL_REQUEST_JOB_EFL_H_
#define EWK_EFL_INTEGRATION_URL_REQUEST_JOB_EFL_H_

#include <cstddef>
#include <queue>
#include <string>

#include "net/http/http_response_info.h"
#include "net/url_request/url_request_job.h"

#include "private/ewk_intercept_request_private.h"

namespace net {
class URLRequest;
class IOBuffer;
}  // namespace net

class URLRequestJobEFL : public net::URLRequestJob {
 public:
  URLRequestJobEFL(net::URLRequest* request,
                   net::NetworkDelegate* network_delegate,
                   _Ewk_Intercept_Request* intercept_request);

  // URLRequestJob:
  void Start() override;
  void Kill() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  bool GetMimeType(std::string* mime_type) const override;
  bool GetCharset(std::string* charset) override;
  int GetResponseCode() const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;

  // for _Ewk_Intercept_Request:
  void ResponseDecided();
  void PutChunk(const char* data, size_t length);
  void ChunkedReadDone();

  _Ewk_Intercept_Request* GetInterceptRequestHandle() const {
    return intercept_request_.get();
  }

 private:
  class Chunk {
   public:
    Chunk(const char* data, size_t length);
    ~Chunk();

    /* LCOV_EXCL_START */
    size_t Available() {
      return length_ - (data_pos_ - data_.get());
    }
    const char* Data() { return data_pos_; }
    void Advance(size_t count) { data_pos_ += count; }
    /* LCOV_EXCL_STOP */

   private:
    std::unique_ptr<const char[]> data_;
    const char* data_pos_;
    size_t length_;
  };

  ~URLRequestJobEFL() override;

  void HeadersComplete();
  void TakeResponseData();
  size_t ReadData(net::IOBuffer* buf, int buf_size);
  size_t ReadChunkedData(net::IOBuffer* buf, int buf_size);

  std::unique_ptr<_Ewk_Intercept_Request> intercept_request_;
  net::HttpResponseInfo response_info_;

  size_t bytes_read_;
  size_t response_body_length_;
  bool killed_;
  bool waiting_done_;
  bool communication_done_;
  bool started_;

  std::string response_headers_;
  std::unique_ptr<char[]> response_body_;

  size_t available_data_;
  std::queue<Chunk> chunks_;
  // |buf_| lifetime is managed by outer scope, it is alive as long as job uses
  // it - until ::ReadRawData returns (with value true), or until
  // ::NotifyReadComplete call in async case.
  net::IOBuffer* buf_;
  int buf_size_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestJobEFL);
};

#endif  // EWK_EFL_INTEGRATION_URL_REQUEST_JOB_EFL_H_
