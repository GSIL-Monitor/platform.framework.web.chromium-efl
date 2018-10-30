/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ewk_error_internal.h"

#include "private/ewk_error_private.h"
#include "private/ewk_private.h"

/* LCOV_EXCL_START */
Ewk_Error_Type ewk_error_type_get(const Ewk_Error* error)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(error, EWK_ERROR_TYPE_NONE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(error->domain, EWK_ERROR_TYPE_NONE);

  // TODO: right now no other Ewk_Error domain is used
  if (strcmp(error->domain, net::kErrorDomain) == 0) {
    return EWK_ERROR_TYPE_NETWORK;
  }

  LOG(ERROR) << "Unknown error domain: " << error->domain;
  return EWK_ERROR_TYPE_NONE;
}
/* LCOV_EXCL_STOP */

const char* ewk_error_url_get(const Ewk_Error* error)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(error, nullptr);
  return error->url;  // LCOV_EXCL_LINE
}

int ewk_error_code_get(const Ewk_Error* error)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(error, 0);
  return error->error_code;  // LCOV_EXCL_LINE
}

const char* ewk_error_description_get(const Ewk_Error* error)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(error, nullptr);
  return error->description;  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
const char* ewk_error_domain_get(const Ewk_Error* error)
{
  LOG_EWK_API_MOCKUP();
  return NULL;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_error_cancellation_get(const Ewk_Error* error)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(error, EINA_FALSE);
  return error->is_cancellation;  // LCOV_EXCL_LINE
}

