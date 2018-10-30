/*
 * Copyright (C) 2013-2016 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SAMSUNG ELECTRONICS. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SAMSUNG ELECTRONICS. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ewk_intercept_request_internal.h"

#include "private/ewk_intercept_request_private.h"
#include "private/ewk_private.h"

/* LCOV_EXCL_START */
const char* ewk_intercept_request_url_get(
    Ewk_Intercept_Request* intercept_request) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, NULL);
  return intercept_request->request_url_get();
}

const char* ewk_intercept_request_http_method_get(
    Ewk_Intercept_Request* intercept_request) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, NULL);
  return intercept_request->request_http_method_get();
}

const Eina_Hash* ewk_intercept_request_headers_get(
    Ewk_Intercept_Request* intercept_request) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, NULL);
  return intercept_request->request_headers_get();
}

Eina_Bool ewk_intercept_request_ignore(
    Ewk_Intercept_Request* intercept_request) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, EINA_FALSE);
  return intercept_request->request_ignore();
}

Eina_Bool ewk_intercept_request_response_set(
    Ewk_Intercept_Request* intercept_request,
    const char* headers,
    const char* body,
    size_t length) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(headers, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(body, EINA_FALSE);
  return intercept_request->response_set(headers, body, length);
}

Eina_Bool ewk_intercept_request_response_status_set(
    Ewk_Intercept_Request* intercept_request,
    int status_code,
    const char* custom_status_text) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, EINA_FALSE);
  return intercept_request->response_status_set(status_code,
                                                custom_status_text);
}

Eina_Bool ewk_intercept_request_response_header_add(
    Ewk_Intercept_Request* intercept_request,
    const char* field_name,
    const char* field_value) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(field_name, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(field_value, EINA_FALSE);
  return intercept_request->response_header_add(field_name, field_value);
}

Eina_Bool ewk_intercept_request_response_header_map_add(
    Ewk_Intercept_Request* intercept_request,
    const Eina_Hash* headers) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(headers, EINA_FALSE);
  return intercept_request->response_header_map_add(headers);
}

Eina_Bool ewk_intercept_request_response_body_set(
    Ewk_Intercept_Request* intercept_request,
    const char* body,
    size_t length) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(body, EINA_FALSE);
  return intercept_request->response_body_set(body, length);
}

Eina_Bool ewk_intercept_request_response_write_chunk(
    Ewk_Intercept_Request* intercept_request,
    const char* chunk,
    size_t length) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, EINA_FALSE);
  // (chunk == NULL && length == 0) means to end of data, handled inside
  // |response_write_chunk|.
  if (chunk != NULL || length != 0) {
    EINA_SAFETY_ON_NULL_RETURN_VAL(chunk, EINA_FALSE);
    EINA_SAFETY_ON_TRUE_RETURN_VAL(length == 0, EINA_FALSE);
  }
  return intercept_request->response_write_chunk(chunk, length);
}

const char* ewk_intercept_request_scheme_get(
    Ewk_Intercept_Request* intercept_request) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, NULL);
  return intercept_request->request_scheme_get();
}

const char* ewk_intercept_request_body_get(
    Ewk_Intercept_Request* intercept_request) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, NULL);
  return intercept_request->request_body_get();
}

int64_t ewk_intercept_request_body_length_get(
    Ewk_Intercept_Request* intercept_request) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(intercept_request, -1);
  return intercept_request->request_body_length_get();
}
/* LCOV_EXCL_STOP */
