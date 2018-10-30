// Copyright 2012 The Chromium Authors. All rights reserved.
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "url_request_job_efl.h"

#include <algorithm>
#include <cstring>

#include "base/callback.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "network_delegate_efl.h"
#endif

using content::BrowserThread;

/* LCOV_EXCL_START */
URLRequestJobEFL::URLRequestJobEFL(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    _Ewk_Intercept_Request* intercept_request)
    : URLRequestJob(request, network_delegate),
      intercept_request_(intercept_request),
      response_info_(),
      bytes_read_(0),
      response_body_length_(0),
      killed_(false),
      waiting_done_(false),
      communication_done_(false),
      started_(false),
      available_data_(0),
      buf_(nullptr),
      buf_size_(0) {
  intercept_request_->set_job(this);
}

URLRequestJobEFL::~URLRequestJobEFL() {
#if defined(OS_TIZEN_TV_PRODUCT)
  auto network_delegate_efl =
      static_cast<net::NetworkDelegateEfl*>(network_delegate());

  if (network_delegate_efl &&
      network_delegate_efl->HasInterceptRequestCancelCallback()) {
    network_delegate_efl->RunInterceptRequestCancelCallback(
        intercept_request_.get());
  }
#endif
}

URLRequestJobEFL::Chunk::Chunk(const char* data, size_t length)
    : data_(data), data_pos_(data), length_(length) {}

URLRequestJobEFL::Chunk::~Chunk() {}

void URLRequestJobEFL::HeadersComplete() {
  // Converts \r\n separated headers to \0 separated as expected by
  // net::HttpResponseHeaders. We can't really expect \0 separated headers from
  // EWK C API.
  base::ReplaceSubstringsAfterOffset(&response_headers_, 0, "\r\n",
                                     std::string("\0", 1));
  response_info_.headers = new net::HttpResponseHeaders(response_headers_);

  NotifyHeadersComplete();
}

void URLRequestJobEFL::TakeResponseData() {
  response_headers_ = intercept_request_->response_headers_take();
  if (!intercept_request_->is_chunked_write()) {
    response_body_ = intercept_request_->response_body_take();
    response_body_length_ = intercept_request_->response_body_length_get();
  }
}

void URLRequestJobEFL::Start() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  started_ = true;

  // User have already sent all data or started chunked writing.
  if (waiting_done_)
    HeadersComplete();
}

int URLRequestJobEFL::ReadRawData(net::IOBuffer* buf,
                                  int buf_size) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // It is safe to call is_chunked_write here on IO thread, despite it being
  // set in UI/user thread. Due to URLRequestJob contract we won't enter
  // ReadRawData until we call NotifyHeadersComplete, which is called in
  // response to ResponseDecided task posted on IO thread by
  // Ewk_Intercept_Request. is_chunked_write internal state is always decided
  // before ResponseDecided is posted. The same reasoning applies to call to
  // is_chunked_write in ResponseDecided task.
  if (intercept_request_->is_chunked_write()) {
    // If user signaled end of writing before all data was read by engine
    // we couldn't call ::NotifyDone() in ::ChunkedReadDone(), so we can signal
    // end of data here by setting |bytes_read| to 0 and returning true.
    if (communication_done_ && !available_data_)
      return 0;

    if (available_data_) {
      // If user wrote some chunks of data before ::ReadRawData() we can read
      // it already here.
      return static_cast<int>(
          ReadChunkedData(buf, buf_size));
    } else {
      // If there is no chunk of data we save |buf| and wait for a write.
      buf_ = buf;
      buf_size_ = buf_size;
      return net::ERR_IO_PENDING;
    }
  } else {
    return static_cast<int>(ReadData(buf, buf_size));
  }
}

void URLRequestJobEFL::Kill() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (killed_)
    return;
  killed_ = true;
  buf_ = nullptr;
  buf_size_ = -1;
  intercept_request_->request_early_exit();
  URLRequestJob::Kill();
}

void URLRequestJobEFL::ResponseDecided() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  waiting_done_ = true;
  TakeResponseData();
  if (started_ && !killed_) {
    // No point in notifing if job is dead.
    HeadersComplete();
  }
}

void URLRequestJobEFL::PutChunk(const char* data,
                                size_t length) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (killed_) {
    // No point doing any work if job is already killed, but events are still
    // scheduled.
    delete data;
    return;
  }
  chunks_.emplace(data, length);
  available_data_ += length;
  if (buf_) {
    // A buffer is ready, we can write now.
    int result = static_cast<int>(ReadChunkedData(buf_, buf_size_));
    buf_ = nullptr;
    buf_size_ = -1;
    ReadRawDataComplete(result);
  }
}

void URLRequestJobEFL::ChunkedReadDone() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  communication_done_ = true;
  // If user spammed engine with data, and called ::ChunkedReadDone() before
  // all chunks have been read, then we might not be done yet. End of reading
  // will be signaled from ::ReadRawData().
  if (chunks_.empty() && !killed_)
    ReadRawDataComplete(0);
}

size_t URLRequestJobEFL::ReadChunkedData(net::IOBuffer* buf, int buf_size) {
  size_t to_read = std::min(static_cast<size_t>(buf_size), available_data_);
  available_data_ -= to_read;
  char* buf_ptr = buf->data();
  while (!chunks_.empty()) {
    if (chunks_.front().Available() > to_read) {
      memcpy(buf_ptr, chunks_.front().Data(), to_read);
      buf_ptr += to_read;
      chunks_.front().Advance(to_read);
      to_read = 0;
      break;
    } else {
      memcpy(buf_ptr, chunks_.front().Data(), chunks_.front().Available());
      buf_ptr += chunks_.front().Available();
      to_read -= chunks_.front().Available();
      chunks_.front().Advance(chunks_.front().Available());
      chunks_.pop();
      if (!to_read)
        break;
    }
  }
  return buf_ptr - buf->data();
}

size_t URLRequestJobEFL::ReadData(net::IOBuffer* buf, int buf_size) {
  size_t to_read = std::min(static_cast<size_t>(buf_size),
                            response_body_length_ - bytes_read_);
  if (to_read) {
    memcpy(buf->data(), response_body_.get() + bytes_read_, to_read);
    bytes_read_ += to_read;
  }

  return to_read;
}

bool URLRequestJobEFL::GetMimeType(std::string* mime_type) const {
  return response_info_.headers->GetMimeType(mime_type);
}

bool URLRequestJobEFL::GetCharset(std::string* charset) {
#if defined(OS_TIZEN_TV_PRODUCT)
  if (!response_info_.headers->GetCharset(charset))
#endif
    *charset = std::string("utf8");
  return true;
}

int URLRequestJobEFL::GetResponseCode() const {
  return response_info_.headers->response_code();
}

void URLRequestJobEFL::GetResponseInfo(net::HttpResponseInfo* info) {
  *info = response_info_;
}
/* LCOV_EXCL_STOP */
