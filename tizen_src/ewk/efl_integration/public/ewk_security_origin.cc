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

#include "ewk_security_origin_internal.h"
#include "private/ewk_security_origin_private.h"

/*
 *
 * @param origin security origin
 *
 * @return host of security origin
 */
Eina_Stringshare* ewk_security_origin_host_get(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);
    return origin->GetHost();  // LCOV_EXCL_LINE
}

/**
 * Requests for getting host of security origin.
 *
 * @param origin security origin
 *
 * @return protocol of security origin
 */
Eina_Stringshare* ewk_security_origin_protocol_get(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);
    return origin->GetProtocol();  // LCOV_EXCL_LINE
}

/**
 * Requests for getting host of security origin.
 *
 * @param origin security origin
 *
 * @return port of security origin
 */
/* LCOV_EXCL_START */
uint16_t ewk_security_origin_port_get(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);
    return static_cast<uint16_t>(origin->GetPort());
}

void ewk_security_origin_free(Ewk_Security_Origin *o)
{
  delete o;
}

Ewk_Security_Origin *ewk_security_origin_new_from_string(const char *url)
{
  return static_cast<Ewk_Security_Origin *>(_Ewk_Security_Origin::CreateFromString(url));
}
/* LCOV_EXCL_STOP */
