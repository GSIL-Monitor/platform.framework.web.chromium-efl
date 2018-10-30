// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_intercept_request_private.h"

#include <cstring>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/upload_data_stream.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request.h"
#include "url_request_job_efl.h"

using content::BrowserThread;

namespace {
/* LCOV_EXCL_START */
void _headers_entry_free_cb(void* data) {
  free(data);
}
/* LCOV_EXCL_STOP */
}  // namespace

/* LCOV_EXCL_START */
_Ewk_Intercept_Request::_Ewk_Intercept_Request(net::URLRequest* request)
    // API user might want to receive even invalid urls
    : job_(nullptr),
      request_(request),
      request_scheme_(request->url().scheme()),
      request_url_(request->url().possibly_invalid_spec()),
      request_http_method_(request->method()),
      request_body_length_(-1),
      response_body_length_(0),
      response_status_code_(-1),
      ignored_(false),
      chunked_write_(false) {
  // Code for getting request headers ported from android webview impl, see
  // android_webview/native/aw_contents_io_thread_client_impl.cc.
  // Headers for http requests we get here will not be the same as headers
  // which would be sent if request have been executed normally.
  //
  // Getting the same http headers would require creating URLRequestHttpJob,
  // which fills http headers. See HttpNetworkTransaction::BuildRequestHeaders
  // in net/http/http_network_transaction.cc.

  net::HttpRequestHeaders headers;
  if (!request->GetFullRequestHeaders(&headers))
    headers = request->extra_request_headers();
  request_headers_ = eina_hash_string_small_new(_headers_entry_free_cb);
  net::HttpRequestHeaders::Iterator current_header(headers);
  while (current_header.GetNext()) {
    if (!eina_hash_add(request_headers_, current_header.name().c_str(),
                       strdup(current_header.value().c_str()))) {
      LOG(ERROR) << "Failed to add header to Eina_Hash";
    }
  }
}

_Ewk_Intercept_Request::~_Ewk_Intercept_Request() {
  eina_hash_free(request_headers_);
}

Eina_Bool _Ewk_Intercept_Request::response_status_set(
    int status_code,
    const char* custom_status_text) {
  response_status_code_ = status_code;
  if (custom_status_text) {
    response_status_text_ = custom_status_text;
  } else {
    response_status_text_ =
        net::GetHttpReasonPhrase(net::HttpStatusCode(status_code));
  }
  return EINA_TRUE;
}

Eina_Bool _Ewk_Intercept_Request::response_header_add(const char* field_name,
                                                      const char* field_value) {
  response_headers_map_.push_back(
      std::pair<std::string, std::string>(field_name, field_value));
  return EINA_TRUE;
}

Eina_Bool _Ewk_Intercept_Request::response_header_map_add(
    const Eina_Hash* headers) {
  Eina_Iterator* it = eina_hash_iterator_tuple_new(headers);
  if (!it)
    return EINA_FALSE;
  void* data;
  while (eina_iterator_next(it, &data)) {
    Eina_Hash_Tuple* tuple = static_cast<Eina_Hash_Tuple*>(data);
    std::string field_name = static_cast<const char*>(tuple->key);
    std::string field_value = static_cast<const char*>(tuple->data);
    response_headers_map_.push_back(std::pair<std::string, std::string>(
        std::move(field_name), std::move(field_value)));
  }
  eina_iterator_free(it);
  return EINA_TRUE;
}

void _Ewk_Intercept_Request::headers_generate() {
  response_headers_.append("HTTP/1.1 ");
  response_headers_.append(base::IntToString(response_status_code_));
  response_headers_.append(" ");
  response_headers_.append(response_status_text_);
  response_headers_.append("\r\n");
  for (const auto& header : response_headers_map_) {
    response_headers_.append(header.first);
    response_headers_.append(": ");
    response_headers_.append(header.second);
    response_headers_.append("\r\n");
  }
  response_headers_.append("\r\n");
}

Eina_Bool _Ewk_Intercept_Request::response_body_set(const void* data,
                                                    size_t length) {
  headers_generate();
  set_data(data, length);
  post_response_decided();
  return EINA_TRUE;
}

Eina_Bool _Ewk_Intercept_Request::response_set(const char* headers,
                                               const void* data,
                                               size_t length) {
  response_headers_ = headers;
  set_data(data, length);
  post_response_decided();
  return EINA_TRUE;
}

void _Ewk_Intercept_Request::set_data(const void* data, size_t length) {
  response_body_.reset(new char[length]);
  memcpy(response_body_.get(), data, length);
  response_body_length_ = length;
}

Eina_Bool _Ewk_Intercept_Request::request_ignore() {
  ignored_ = true;
  return EINA_TRUE;
}

Eina_Bool _Ewk_Intercept_Request::response_write_chunk(const void* data,
                                                       size_t length) {
  // On first chunked write generate headers and signal job.
  if (!chunked_write_) {
    chunked_write_ = true;
    headers_generate();
    post_response_decided();
  }

  // Zero length data means end of response body. Also there is possibility of
  // early finish if engine doesn't want response to the request anymore.
  if (length == 0 || chunked_write_early_exit_.IsSet()) {
    post_chunked_read_done();
    return EINA_FALSE;
  }

  char* data_copy = new char[length];
  memcpy(data_copy, data, length);
  post_put_chunk(data_copy, length);
  return EINA_TRUE;
}

void _Ewk_Intercept_Request::response_decided() {
  job_->ResponseDecided();
}

void _Ewk_Intercept_Request::post_response_decided() {
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&_Ewk_Intercept_Request::response_decided,
                                     base::Unretained(this)));
}

void _Ewk_Intercept_Request::put_chunk(const char* data, size_t length) {
  job_->PutChunk(data, length);
}

void _Ewk_Intercept_Request::post_put_chunk(const char* data, size_t length) {
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&_Ewk_Intercept_Request::put_chunk,
                                     base::Unretained(this), data, length));
}

void _Ewk_Intercept_Request::chunked_read_done() {
  job_->ChunkedReadDone();
}

void _Ewk_Intercept_Request::post_chunked_read_done() {
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&_Ewk_Intercept_Request::chunked_read_done,
                                     base::Unretained(this)));
}

const char* _Ewk_Intercept_Request::request_body_get() const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!request_body_.empty())
    return request_body_.data();

  if (!request_) {
    LOG(ERROR) << "Trying to get request body outside of "
                  "Ewk_Context_Intercept_Request_Callback";
    return nullptr;
  }

  const net::UploadDataStream* stream = request_->get_upload();
  if (!stream)
    return nullptr;

  std::string http_body;
  if (!stream->DumpUploadData(http_body))
    return nullptr;

  request_body_ = http_body;
  return request_body_.data();
}

int64_t _Ewk_Intercept_Request::request_body_length_get() const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (request_body_length_ >= 0)
    return request_body_length_;

  if (!request_) {
    LOG(ERROR) << "Trying to get request body length outside of "
                  "Ewk_Context_Intercept_Request_Callback";
    return -1;
  }

  const net::UploadDataStream* stream = request_->get_upload();
  if (!stream)
    return -1;

  request_body_length_ = stream->GetSizeSync();
  return request_body_length_;
}
/* LCOV_EXCL_STOP */
