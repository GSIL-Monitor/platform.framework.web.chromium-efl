/*
 * Copyright (C) 2016 Samsung Electronics. All rights reserved.
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

#include "ewk_highcontrast_product.h"
#include "private/ewk_private.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "content/browser/high_contrast/high_contrast_controller.h"
#endif
void ewk_highcontrast_enabled_set(Eina_Bool enabled)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  content::HighContrastController::Shared().SetUseHighContrast(!!enabled);
#else
  LOG_EWK_API_MOCKUP("This is only available on Tizen TV Product");
#endif
}

Eina_Bool ewk_highcontrast_enabled_get()
{
#if defined(OS_TIZEN_TV_PRODUCT)
  return content::HighContrastController::Shared().UseHighContrast();
#else
  LOG_EWK_API_MOCKUP("This is only available on Tizen TV Product");
  return EINA_FALSE;
#endif
}

Eina_Bool ewk_highcontrast_forbidden_url_add(const char* url)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  return url ? content::HighContrastController::Shared().AddForbiddenUrl(url) : EINA_FALSE;
#else
  LOG_EWK_API_MOCKUP("This is only available on Tizen TV Product");
  return EINA_FALSE;
#endif
}

Eina_Bool ewk_highcontrast_forbidden_url_remove(const char* url)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  return url ? content::HighContrastController::Shared().RemoveForbiddenUrl(url) : EINA_FALSE;
#else
  LOG_EWK_API_MOCKUP("This is only available on Tizen TV Product");
  return EINA_FALSE;
#endif
}
