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

#include "ewk_geolocation_internal.h"

#include "ewk_security_origin.h"
#include "private/ewk_security_origin_private.h"
#include "private/ewk_geolocation_private.h"

const Ewk_Security_Origin* ewk_geolocation_permission_request_origin_get(const Ewk_Geolocation_Permission_Request* permissionRequest)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(permissionRequest, 0);
    return static_cast<const Ewk_Security_Origin*>(permissionRequest->GetOrigin());
}

Eina_Bool ewk_geolocation_permission_request_set(Ewk_Geolocation_Permission_Request* permissionRequest, Eina_Bool allow)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(permissionRequest, EINA_FALSE);
    return permissionRequest->SetDecision(allow == EINA_TRUE);
}

Eina_Bool ewk_geolocation_permission_reply(Ewk_Geolocation_Permission_Request* permissionRequest, Eina_Bool allow)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(permissionRequest, EINA_FALSE);
    return permissionRequest->SetDecision(allow == EINA_TRUE); // the same as ewk_geolocation_permission_request_set
}

void ewk_geolocation_permission_request_suspend(Ewk_Geolocation_Permission_Request* permissionRequest)
{
    EINA_SAFETY_ON_NULL_RETURN(permissionRequest);
    permissionRequest->Suspend();
}

