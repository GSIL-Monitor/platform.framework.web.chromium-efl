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

#include "ewk_user_media_internal.h"

#include "private/ewk_private.h"
#include "private/ewk_security_origin_private.h"
#include "private/ewk_user_media_private.h"

Eina_Bool ewk_user_media_permission_request_suspend(
    Ewk_User_Media_Permission_Request* request) {  // LCOV_EXCL_LINE
  /* LCOV_EXCL_START */
  EINA_SAFETY_ON_NULL_RETURN_VAL(request, false);
  request->Suspend();
  return true;
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
void ewk_user_media_permission_request_set(
    Ewk_User_Media_Permission_Request* request, Eina_Bool allowed) {
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  if(request && request->WebContents())
    request->ProceedPermissionCallback(allowed == EINA_TRUE);
#endif
}
/* LCOV_EXCL_STOP */

void ewk_user_media_permission_reply(  // LCOV_EXCL_LINE
    Ewk_User_Media_Permission_Request* request,
    Eina_Bool allowed) {
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  /* LCOV_EXCL_START */
  if(request && request->WebContents())
    request->ProceedPermissionCallback(allowed == EINA_TRUE);
/* LCOV_EXCL_STOP */
#endif
}  // LCOV_EXCL_LINE

const Ewk_Security_Origin*
ewk_user_media_permission_request_origin_get(  // LCOV_EXCL_LINE
    const Ewk_User_Media_Permission_Request* request) {
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(request, 0);  // LCOV_EXCL_LINE

  return static_cast<const Ewk_Security_Origin*>(
      request->Origin());  // LCOV_EXCL_LINE
#else
  return nullptr;
#endif
}

/* LCOV_EXCL_START */
const char* ewk_user_media_permission_request_message_get(
    const Ewk_User_Media_Permission_Request* request) {
  LOG_EWK_API_MOCKUP("This API is deprecated");
  return NULL;
}

Ewk_User_Media_Device_Type ewk_user_media_permission_request_device_type_get(
    const Ewk_User_Media_Permission_Request* request) {
#if defined(OS_TIZEN_TV_PRODUCT)
  if (!request)
    return EWK_USER_MEDIA_DEVICE_TYPE_NONE;

  int deviceType = 0;
  if(request->IsAudioRequested())
    deviceType |= EWK_USER_MEDIA_DEVICE_TYPE_MICROPHONE;

  if (request->IsVideoRequested())
    deviceType |= EWK_USER_MEDIA_DEVICE_TYPE_CAMERA;

  return static_cast<Ewk_User_Media_Device_Type>(deviceType);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
  return EWK_USER_MEDIA_DEVICE_TYPE_NONE;
#endif
}
/* LCOV_EXCL_STOP */
