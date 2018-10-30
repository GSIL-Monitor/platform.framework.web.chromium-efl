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

#include "ewk_content_screening_detection_internal.h"
#include "private/ewk_private.h"

void ewk_content_screening_detection_confirmed_set(Ewk_Content_Screening_Detection* content_screening_detection, Eina_Bool confirmed)
{
  // API is postponed. Possibly chromium's mechanism may be reused.
  LOG_EWK_API_MOCKUP("Not Supported yet");
}

void ewk_content_screening_detection_suspend(Ewk_Content_Screening_Detection* content_screening_detection)
{
  // API is postponed. Possibly chromium's mechanism may be reused.
  LOG_EWK_API_MOCKUP("Not Supperted yet");
}

Ewk_Error* ewk_content_screening_detection_error_get(Ewk_Content_Screening_Detection* content_screening_detection)
{
  // API is postponed. Possibly chromium's mechanism may be reused.
  LOG_EWK_API_MOCKUP("Not Supported yet");
  return NULL;
}

int ewk_content_screening_detection_level_get(Ewk_Content_Screening_Detection* content_screening_detection)
{
  // API is postponed. Possibly chromium's mechanism may be reused.
  LOG_EWK_API_MOCKUP("Not Supported yet");
  return 0;
}

const char* ewk_content_screening_detection_name_get(Ewk_Content_Screening_Detection* content_screening_detection)
{
  // API is postponed. Possibly chromium's mechanism may be reused.
  LOG_EWK_API_MOCKUP("Not Supported yet");
  return NULL;
}

const char* ewk_content_screening_detection_url_get(Ewk_Content_Screening_Detection* content_screening_detection)
{
  // API is postponed. Possibly chromium's mechanism may be reused.
  LOG_EWK_API_MOCKUP("Not Supported yet");
  return NULL;
}
