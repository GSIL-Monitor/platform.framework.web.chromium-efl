/*
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

#include "ewk_autofill_profile.h"
#include "ewk_autofill_profile_product.h"

#include "private/ewk_autofill_profile_private.h"
#include "private/ewk_private.h"

Ewk_Autofill_Profile* ewk_autofill_profile_new()
{
  return new(std::nothrow) Ewk_Autofill_Profile();
}

void ewk_autofill_profile_delete(Ewk_Autofill_Profile* profile)
{
  EINA_SAFETY_ON_NULL_RETURN(profile);
  delete profile;
}

void ewk_autofill_profile_data_set(Ewk_Autofill_Profile* profile,
    Ewk_Autofill_Profile_Data_Type name, const char* value)
{
  EINA_SAFETY_ON_NULL_RETURN(profile);
  EINA_SAFETY_ON_NULL_RETURN(value);
  profile->setData(DataType(name), value);
}

unsigned ewk_autofill_profile_id_get(Ewk_Autofill_Profile* profile)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(profile, 0);
  return profile->getProfileID();
}

const char* ewk_autofill_profile_data_get(Ewk_Autofill_Profile* profile,
    Ewk_Autofill_Profile_Data_Type name)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(profile, 0);
  std::string retVal = profile->getData(DataType(name));
  return (retVal.empty()) ?
          NULL : strdup(retVal.c_str());
}

Ewk_Form_Type ewk_autofill_profile_form_type_get(Ewk_Form_Info* info)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(info, EWK_FORM_NONE);
  LOG(INFO) << "[SPASS], type:" << info->GetFormType();
  return info->GetFormType();
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
  return EWK_FORM_NONE;
#endif
}

const char* ewk_autofill_profile_form_user_name_get(Ewk_Form_Info* info)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(info, nullptr);
  LOG(INFO) << "[SPASS], id:" << info->GetFormId();
  return info->GetFormId();
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
  return nullptr;
#endif
}

const char* ewk_autofill_profile_form_password_get(Ewk_Form_Info* info)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(info, nullptr);
  LOG(INFO) << "[SPASS], password:" << info->GetFormPassword();
  return info->GetFormPassword();
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
  return nullptr;
#endif
}

const char* ewk_autofill_profile_form_username_element_get(Ewk_Form_Info* info)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(info, nullptr);
  LOG(INFO) << "[SPASS], user name element:" << info->GetFormUsernameElement();
  return info->GetFormUsernameElement();
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
  return nullptr;
#endif
}

const char* ewk_autofill_profile_form_password_element_get(Ewk_Form_Info* info)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(info, nullptr);
  LOG(INFO) << "[SPASS], password element:" << info->GetFormPasswordElement();
  return info->GetFormPasswordElement();
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
  return nullptr;
#endif
}

const char* ewk_autofill_profile_form_action_url_get(Ewk_Form_Info* info)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(info, nullptr);
  LOG(INFO) << "[SPASS], action url:" << info->GetFormActionUrl();
  return info->GetFormActionUrl();
#else
  LOG_EWK_API_MOCKUP("This API is only available in Tizen TV product.");
  return nullptr;
#endif
}
