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

#include "ewk_quota_permission_request_internal.h"
#include "private/ewk_quota_permission_request_private.h"

EXPORT_API Eina_Stringshare* ewk_quota_permission_request_origin_protocol_get(const Ewk_Quota_Permission_Request* request)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(request, 0);
  return request->GetProtocol();
}

EXPORT_API Eina_Stringshare* ewk_quota_permission_request_origin_host_get(const Ewk_Quota_Permission_Request* request)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(request, 0);
  return request->GetHost();
}

EXPORT_API uint16_t ewk_quota_permission_request_origin_port_get(const Ewk_Quota_Permission_Request* request)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(request, 0);
  return static_cast<uint16_t>(request->GetPort());
}

EXPORT_API int64_t ewk_quota_permission_request_quota_get(const Ewk_Quota_Permission_Request* request)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(request, 0);
  return request->GetQuota();
}

EXPORT_API Eina_Bool ewk_quota_permission_request_is_persistent_get(const Ewk_Quota_Permission_Request* request)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(request, EINA_FALSE);
  return request->IsPersistent() ? EINA_TRUE : EINA_FALSE;
}