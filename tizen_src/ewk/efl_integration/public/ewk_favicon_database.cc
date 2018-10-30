/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Copyright (C) 2014-2016 Samsung Electronics. All rights reserved.
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

#include "ewk_favicon_database_internal.h"

#include "private/ewk_favicon_database_private.h"
#include "private/ewk_private.h"

Evas_Object* ewk_favicon_database_icon_get(Ewk_Favicon_Database* ewkIconDatabase, const char* pageURL, Evas* evas)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewkIconDatabase, nullptr);
  EINA_SAFETY_ON_NULL_RETURN_VAL(pageURL, nullptr);
  EINA_SAFETY_ON_NULL_RETURN_VAL(evas, nullptr);

  return ewkIconDatabase->GetIcon(pageURL, evas);
}

void ewk_favicon_database_icon_change_callback_add(Ewk_Favicon_Database* ewkIconDatabase, Ewk_Favicon_Database_Icon_Change_Cb callback, void* userData)
{
  LOG_EWK_API_MOCKUP("for Tizen TV Browser");
}
