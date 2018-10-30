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

#include "ewk_context_product.h"

#include "base/memory/ref_counted.h"
#include "browser/favicon/favicon_database_p.h"
#include "browser/notification/notification_controller_efl.h"
#include "browser/vibration/vibration_provider_client.h"
#include "browser_context_efl.h"
#include "content_browser_client_efl.h"
#include "content_main_delegate_efl.h"
#include "ewk_global_data.h"
#include "ewk_security_origin.h"
#include "private/ewk_context_private.h"
#include "private/ewk_private.h"
#include "private/ewk_security_origin_private.h"
#include "private/ewk_favicon_database_private.h"
#include "private/ewk_cookie_manager_private.h"
#include "private/ewk_context_form_autofill_credit_card_private.h"
#include "private/ewk_context_form_autofill_profile_private.h"

#if defined(OS_TIZEN)
#include <app_control.h>
#endif

#if defined(TIZEN_PEPPER_EXTENSIONS)
#include "common/trusted_pepper_plugin_util.h"
#include "ewk_extension_system_delegate.h"
#endif

using content::ContentBrowserClientEfl;
using content::ContentMainDelegateEfl;

namespace {
/* LCOV_EXCL_START */
ContentBrowserClientEfl* GetContentBrowserClient() {
  if (!EwkGlobalData::IsInitialized()) {
    return nullptr;
  }

  ContentMainDelegateEfl& cmde =
      EwkGlobalData::GetInstance()->GetContentMainDelegatEfl();
  ContentBrowserClientEfl* cbce =
      static_cast<ContentBrowserClientEfl*>(cmde.GetContentBrowserClient());

  return cbce;
}
/* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
Ewk_Context *ewk_context_ref(Ewk_Context *context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);
  context->AddRef();
  return context;
}
/* LCOV_EXCL_STOP */

/* LCOV_EXCL_START */
void ewk_context_unref(Ewk_Context *context)
{
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->Release();
}
/* LCOV_EXCL_STOP */

Ewk_Cookie_Manager* ewk_context_cookie_manager_get(const Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, 0);
  return context->ewkCookieManager();
}

/* LCOV_EXCL_START */
Ewk_Context* ewk_context_default_get()
{
  return Ewk_Context::DefaultContext();
}

Ewk_Context* ewk_context_new()
{
  return Ewk_Context::Create();
}

Ewk_Context *ewk_context_new_with_injected_bundle_path(const char *path)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
  // To create new Ewk_Context with default incognito = false.
  return Ewk_Context::Create(false, std::string(path));
}

Ewk_Context* ewk_context_new_with_injected_bundle_path_in_incognito_mode(const char *path)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
  return Ewk_Context::Create(true, std::string(path));
}

void ewk_context_delete(Ewk_Context* context)
{
  if (context)
    context->Release();
}

void ewk_context_proxy_uri_set(Ewk_Context* context, const char* proxy)
{
  ewk_context_proxy_set(context, proxy, ewk_context_proxy_bypass_rule_get(context));
}

const char* ewk_context_proxy_uri_get(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);
  return context->GetProxyUri();
}

Eina_Bool ewk_context_notify_low_memory(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, false);
  context->NotifyLowMemory();
  return true;
}

Eina_Bool ewk_context_origins_free(Eina_List* origins)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(origins, false);

  void* current_origin;
  EINA_LIST_FREE(origins, current_origin) {
    Ewk_Security_Origin* origin = static_cast<Ewk_Security_Origin*>(current_origin);
    delete origin;
  }
  return true;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_context_application_cache_delete_all(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  /* LCOV_EXCL_START */
  context->DeleteAllApplicationCache();
  return true;
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
Eina_Bool ewk_context_application_cache_delete(Ewk_Context* context, Ewk_Security_Origin* origin)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(origin, EINA_FALSE);
  context->DeleteApplicationCacheForSite(origin->GetURL());
  return true;
}

Eina_Bool ewk_context_application_cache_origins_get(Ewk_Context* context, Ewk_Web_Application_Cache_Origins_Get_Callback callback, void* user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);

  context->GetAllOriginsWithApplicationCache(callback, user_data);
  return true;
}

Eina_Bool ewk_context_application_cache_usage_for_origin_get(Ewk_Context* context, const Ewk_Security_Origin* origin, Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback callback, void* user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(origin, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->GetApplicationCacheUsage(origin->GetURL(), callback, user_data);
  return true;
}

Eina_Bool ewk_context_application_cache_quota_for_origin_set(Ewk_Context* context, const Ewk_Security_Origin* origin, int64_t quota)
{
  NOTIMPLEMENTED() << "Chromium does not support quota management for individual features.";
  return false;
}

Eina_Bool ewk_context_icon_database_path_set(Ewk_Context* context, const char* path)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);
  return context->SetFaviconDatabasePath(base::FilePath(path));
}
/* LCOV_EXCL_STOP */

Evas_Object* ewk_context_icon_database_icon_object_add(Ewk_Context* context, const char* uri, Evas* canvas)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(uri, NULL);
  /* LCOV_EXCL_START */
  EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, NULL);
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);
  return context->AddFaviconObject(uri, canvas);
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
Eina_Bool ewk_context_local_file_system_all_delete(Ewk_Context *context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->FileSystemDelete(GURL(""));
  return true;
}

Eina_Bool ewk_context_local_file_system_delete(Ewk_Context *context, Ewk_Security_Origin *origin)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(origin, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  std::ostringstream ss;
  ss << ewk_security_origin_protocol_get(origin) << "://" << ewk_security_origin_host_get(origin);
  context->FileSystemDelete(GURL(ss.str()));
  return true;
}

Eina_Bool ewk_context_local_file_system_origins_get(const Ewk_Context *context,
                                                    Ewk_Local_File_System_Origins_Get_Callback callback,
                                                    void *user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->GetAllOriginsWithFileSystem(callback, user_data);
  return true;
}

Eina_Bool ewk_context_web_database_delete_all(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->WebDBDelete(GURL());
  return true;
}

Eina_Bool ewk_context_web_database_delete(Ewk_Context* context, Ewk_Security_Origin* origin)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(origin, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  std::ostringstream ss;
  ss << ewk_security_origin_protocol_get(origin) << "://" << ewk_security_origin_host_get(origin);
  context->WebDBDelete(GURL(ss.str()));
  return true;
}

Eina_Bool ewk_context_web_database_origins_get(Ewk_Context* context,
                                               Ewk_Web_Database_Origins_Get_Callback callback,
                                               void* user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->GetAllOriginsWithWebDB(callback, user_data);
  return true;
}

Eina_Bool ewk_context_web_database_quota_for_origin_get(Ewk_Context* context, Ewk_Web_Database_Quota_Get_Callback callback, void* userData, Ewk_Security_Origin* origin)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Eina_Bool ewk_context_web_database_quota_for_origin_set(Ewk_Context* context, Ewk_Security_Origin* origin, uint64_t quota)
{
  LOG_EWK_API_MOCKUP();
  return false;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_context_web_indexed_database_delete_all(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->IndexedDBDelete();
  return true;
}

Eina_Bool ewk_context_web_storage_delete_all(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->WebStorageDelete();
  return true;
}

/* LCOV_EXCL_START */
Eina_Bool ewk_context_web_storage_origin_delete(Ewk_Context* context, Ewk_Security_Origin* origin)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(origin, EINA_FALSE);

  context->WebStorageDelete(origin->GetURL());
  return EINA_TRUE;
}

Eina_Bool ewk_context_web_storage_origins_get(Ewk_Context* context, Ewk_Web_Storage_Origins_Get_Callback callback, void* user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);

  context->WebStorageOriginsAllGet(callback, user_data);
  return EINA_TRUE;
}

Eina_Bool ewk_context_web_storage_path_set(Ewk_Context* /*context*/, const char* /*path*/)
{
  //With chromium engine no separate path for web storage supported.
  //This API not implemented
  NOTIMPLEMENTED();
  return EINA_FALSE;
}

Eina_Bool ewk_context_web_storage_usage_for_origin_get(Ewk_Context* context, Ewk_Security_Origin* origin, Ewk_Web_Storage_Usage_Get_Callback callback, void* userData)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(origin, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->GetLocalStorageUsageForOrigin(origin->GetURL(), callback, userData);
  return EINA_TRUE;
}

Eina_Bool ewk_context_soup_data_directory_set(Ewk_Context* /*context*/, const char* /*path*/)
{
  //chomium engine does not use libsoup hence this API will not be implemented
  NOTIMPLEMENTED();
  return EINA_FALSE;
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_context_cache_model_set(Ewk_Context* context, Ewk_Cache_Model model)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->SetCacheModel(model);
  return true;
}

Ewk_Cache_Model ewk_context_cache_model_get(const Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EWK_CACHE_MODEL_DOCUMENT_VIEWER);
  return context->GetCacheModel();
}

/* LCOV_EXCL_START */
Eina_Bool ewk_context_cache_disabled_set(Ewk_Context* context, Eina_Bool cacheDisabled)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->SetNetworkCacheEnable(!cacheDisabled);
  return true;
}

Eina_Bool ewk_context_cache_disabled_get(const Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return !context->GetNetworkCacheEnable();
}

Eina_Bool ewk_context_certificate_file_set(Ewk_Context *context, const char *certificate_path)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(certificate_path, EINA_FALSE);
  context->SetCertificatePath(certificate_path);
  return true;
}

const char* ewk_context_certificate_file_get(const Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);
  return context->GetCertificatePath();
}

Eina_Bool ewk_context_cache_clear(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->ClearNetworkCache();
  context->ClearWebkitCache();
  return EINA_TRUE;
}

void ewk_context_did_start_download_callback_set(Ewk_Context* context,
                                                 Ewk_Context_Did_Start_Download_Callback callback,
                                                 void* userData)
{
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetDidStartDownloadCallback(callback, userData);
}

void ewk_context_mime_override_callback_set(Ewk_Context* context, Ewk_Context_Override_Mime_For_Url_Callback callback, void* user_data)
{
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetMimeOverrideCallback(callback, user_data);
}

void ewk_context_memory_sampler_start(Ewk_Context* context, double timerInterval)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_context_memory_sampler_stop(Ewk_Context* context)
{
  LOG_EWK_API_MOCKUP();
}

Eina_Bool ewk_context_additional_plugin_path_set(Ewk_Context *context, const char *path)
{
  LOG_EWK_API_MOCKUP("Not Supported by chromium. Currently Deprecated");
  return EINA_FALSE;
}

void ewk_context_memory_saving_mode_set(Ewk_Context* context, Eina_Bool mode)
{
  //memory saving mode should be handled in chromium in different way
  //than it is handled in WebKit, so implementation of this feature
  //is postponed, for now dummy implementation is enough
  NOTIMPLEMENTED();
  return;
}
/* LCOV_EXCL_STOP */

void ewk_context_form_password_data_delete_all(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->ClearPasswordData();
}

/* LCOV_EXCL_START */
void ewk_context_form_password_data_delete(Ewk_Context* ewkContext, const char* url)
{
  EINA_SAFETY_ON_NULL_RETURN(ewkContext);
  EINA_SAFETY_ON_NULL_RETURN(url);
  ewkContext->ClearPasswordDataForUrl(url);

}

void ewk_context_form_password_data_list_get(
    Ewk_Context* ewkContext,
    Ewk_Context_Form_Password_Data_List_Get_Callback callback,
    void* user_data)
{
  EINA_SAFETY_ON_NULL_RETURN(ewkContext);
  EINA_SAFETY_ON_NULL_RETURN(callback);
  ewkContext->GetPasswordDataList(callback, user_data);
}

void ewk_context_form_password_data_list_free(Ewk_Context* ewkContext, Eina_List* list)
{
  EINA_SAFETY_ON_NULL_RETURN(ewkContext);
  EINA_SAFETY_ON_NULL_RETURN(list);

  void* item;
  EINA_LIST_FREE(list, item) {
    delete [] (static_cast<Ewk_Password_Data*>(item))->url;
    delete static_cast<Ewk_Password_Data*>(item);
  }
}
/* LCOV_EXCL_STOP */

void ewk_context_form_candidate_data_delete_all(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->ClearCandidateData();
}

/* LCOV_EXCL_START */
void ewk_context_form_autofill_profile_changed_callback_set(
    Ewk_Context_Form_Autofill_Profile_Changed_Callback callback,
    void* user_data)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EwkContextFormAutofillProfileManager::priv_form_autofill_profile_changed_callback_set(
      callback, user_data);
#endif
}
/* LCOV_EXCL_STOP */

Eina_List* ewk_context_form_autofill_profile_get_all(Ewk_Context* context)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);
  return EwkContextFormAutofillProfileManager::priv_form_autofill_profile_get_all(context);
#else
  return NULL;
#endif
}

Ewk_Autofill_Profile* ewk_context_form_autofill_profile_get(Ewk_Context* context, unsigned id)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);
  return EwkContextFormAutofillProfileManager::priv_form_autofill_profile_get(context, id);
#else
  return NULL;
#endif
}

Eina_Bool ewk_context_form_autofill_profile_set(Ewk_Context* context, unsigned id, Ewk_Autofill_Profile* profile)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(profile, EINA_FALSE);
  return EwkContextFormAutofillProfileManager::priv_form_autofill_profile_set(context, id, profile);
#else
  return EINA_FALSE;
#endif

}

Eina_Bool ewk_context_form_autofill_profile_add(Ewk_Context* context, Ewk_Autofill_Profile* profile)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(profile, EINA_FALSE);
  return EwkContextFormAutofillProfileManager::priv_form_autofill_profile_add(context, profile);
#else
  return EINA_FALSE;
#endif

}

Eina_Bool ewk_context_form_autofill_profile_remove(Ewk_Context* context, unsigned id)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return EwkContextFormAutofillProfileManager::priv_form_autofill_profile_remove(context, id);
#else
  return EINA_FALSE;
#endif
}

/* LCOV_EXCL_START */
void ewk_context_form_autofill_credit_card_changed_callback_set(
    Ewk_Context_Form_Autofill_CreditCard_Changed_Callback callback,
    void* user_data)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_changed_callback_set(
      callback, user_data);
#endif
}

Eina_List* ewk_context_form_autofill_credit_card_get_all(Ewk_Context* context)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);
  return EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_get_all(context);
#else
  return NULL;
#endif
}

Ewk_Autofill_CreditCard* ewk_context_form_autofill_credit_card_get(Ewk_Context* context, unsigned id)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);
  return EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_get(context, id);
#else
  return NULL;
#endif
}

Eina_Bool ewk_context_form_autofill_credit_card_set(Ewk_Context* context, unsigned id, Ewk_Autofill_CreditCard* card)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(card, EINA_FALSE);
  return EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_set(context, id, card);
#else
  return EINA_FALSE;
#endif
}

Eina_Bool ewk_context_form_autofill_credit_card_add(Ewk_Context* context, Ewk_Autofill_CreditCard* card)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(card, EINA_FALSE);
  return EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_add(context, card);
#else
  return EINA_FALSE;
#endif
}

Eina_Bool ewk_context_form_autofill_credit_card_remove(Ewk_Context* context, unsigned id)
{
#if defined(TIZEN_AUTOFILL_SUPPORT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return EwkContextFormAutofillCreditCardManager::priv_form_autofill_credit_card_remove(context, id);
#else
  return EINA_FALSE;
#endif
}
/* LCOV_EXCL_STOP */

/* LCOV_EXCL_START */
void ewk_context_vibration_client_callbacks_set(Ewk_Context* context,
                                                Ewk_Vibration_Client_Vibrate_Cb vibrate,
                                                Ewk_Vibration_Client_Vibration_Cancel_Cb cancel,
                                                void* data)
{
  VibrationProviderClientEwk* vibra_client = VibrationProviderClientEwk::GetInstance();
  vibra_client->SetVibrationClientCallbacks(vibrate, cancel, data);
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_context_tizen_extensible_api_string_set(Ewk_Context* context,  const char* extensible_api, Eina_Bool enable)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(extensible_api, EINA_FALSE);

  return context->SetExtensibleAPI(extensible_api, enable);
}

Eina_Bool ewk_context_tizen_extensible_api_string_get(Ewk_Context* context,  const char* extensible_api)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(extensible_api, EINA_FALSE);

  return context->GetExtensibleAPI(extensible_api);
}

/* LCOV_EXCL_START */
Eina_Bool ewk_context_tizen_extensible_api_set(Ewk_Context * /*context*/, Ewk_Extensible_API /*extensibleAPI*/, Eina_Bool /*enable*/)
{
  // This API was deprecated.
  NOTIMPLEMENTED();
  return EINA_FALSE;
}

Eina_Bool ewk_context_tizen_extensible_api_get(Ewk_Context * /*context*/, Ewk_Extensible_API /*extensibleAPI*/)
{
  // This API was deprecated.
  NOTIMPLEMENTED();
  return EINA_FALSE;
}

void ewk_context_storage_path_reset(Ewk_Context* /*context*/)
{
  //not supported in chromium to dynamically update the storage path
  NOTIMPLEMENTED();
}

Eina_Bool ewk_context_pixmap_set(Ewk_Context *context, int pixmap)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->SetPixmap(pixmap);
  return true;
}

int ewk_context_pixmap_get(Ewk_Context *context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, 0);
  return context->Pixmap();
}

unsigned int ewk_context_inspector_server_start(Ewk_Context* ewkContext, unsigned int port)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);
  return ewkContext->InspectorServerStart(port);
}

Eina_Bool ewk_context_inspector_server_stop(Ewk_Context* ewkContext)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, EINA_FALSE);
  return ewkContext->InspectorServerStop();
}

void ewk_send_widget_info(Ewk_Context *context,
                          const char* tizen_id,
                          double scale,
                          const char *theme,
                          const char *encodedBundle)
{
  EINA_SAFETY_ON_NULL_RETURN(theme);
  EINA_SAFETY_ON_NULL_RETURN(encodedBundle);
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetWidgetInfo(tizen_id, scale, theme, encodedBundle);
}

void ewk_context_tizen_app_id_set(Ewk_Context* context,
                                  const char* tizen_app_id) {
  EINA_SAFETY_ON_NULL_RETURN(context);
  EINA_SAFETY_ON_NULL_RETURN(tizen_app_id);
  // Ewk_Context::SetWidgetInfo() will be changed with removal of argumets
  // hardcoded in this call once bug
  // http://107.108.218.239/bugzilla/show_bug.cgi?id=15424 is merged.
  context->SetWidgetInfo(tizen_app_id, 1.0, "theme", "encoded_bundle");
}

Eina_Bool ewk_context_tizen_app_version_set(Ewk_Context* context, const char* tizen_app_version)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(tizen_app_version, EINA_FALSE);

  return context->SetAppVersion(tizen_app_version);
}

Eina_Bool ewk_context_pwa_storage_path_set(Ewk_Context* context,
                                  const char* pwa_storage_path) {
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(pwa_storage_path, EINA_FALSE);
  return context->SetPWAInfo(pwa_storage_path);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return EINA_FALSE;
#endif
}

Ewk_Application_Cache_Manager* ewk_context_application_cache_manager_get(const Ewk_Context* ewkContext)
{
  LOG_EWK_API_MOCKUP("for Tizen TV Browser");
  return NULL;
}

Ewk_Favicon_Database* ewk_context_favicon_database_get(const Ewk_Context* ewkContext)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);
  return ewkContext->GetFaviconDatabase();
}
/* LCOV_EXCL_STOP */

void ewk_context_resource_cache_clear(Ewk_Context* ewkContext)
{
  EINA_SAFETY_ON_NULL_RETURN(ewkContext);
  ewkContext->ClearNetworkCache();
  ewkContext->ClearWebkitCache();
}

/* LCOV_EXCL_START */
Eina_Bool ewk_context_favicon_database_directory_set(Ewk_Context* ewkContext, const char* directoryPath)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(directoryPath, EINA_FALSE);
  base::FilePath path(directoryPath);
  path = path.Append(FaviconDatabasePrivate::defaultFilename);
  return ewkContext->SetFaviconDatabasePath(path);
}

void ewk_context_preferred_languages_set(Eina_List* languages)
{
  ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN(cbce);

  std::string preferredLanguages;

  Eina_List* it;
  void* data;

  EINA_LIST_FOREACH(languages, it, data) {
    if (data) {
      preferredLanguages.append(static_cast<char*>(data));
      if (it->next) {
        preferredLanguages.append(",");
      }
    }
  }
  std::transform(preferredLanguages.begin(), preferredLanguages.end(), preferredLanguages.begin(), ::tolower);
  std::replace(preferredLanguages.begin(), preferredLanguages.end(), '_', '-');

  cbce->SetPreferredLangs(preferredLanguages);
}

Ewk_Storage_Manager* ewk_context_storage_manager_get(const Ewk_Context* ewkContext)
{
  LOG_EWK_API_MOCKUP("Tizen TV Browser");
  return NULL;
}

Eina_Bool ewk_context_app_control_set(const Ewk_Context* context, void* app_control)
{
#if defined(OS_TIZEN)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(app_control, EINA_FALSE);

  char* notification_id = nullptr;
  app_control_h control = static_cast<app_control_h>(app_control);
  int ret = app_control_get_extra_data(control, "notification_id",
                                       &notification_id);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << "app_control_get_extra_data is failed with err " << ret;
    return EINA_FALSE;
  }

  if (notification_id) {
    ContentBrowserClientEfl* cbce = GetContentBrowserClient();
    if (cbce) {
      char* notification_origin = nullptr;
      app_control_get_extra_data(control, "notification_origin",
                                 &notification_origin);
      cbce->GetNotificationController()->
          PersistentNotificationClicked(context->browser_context(),
                                        notification_id,
                                        notification_origin);
    }
  }
  return EINA_TRUE;
#else
  return EINA_FALSE;
#endif
}

Eina_Bool ewk_context_notification_callbacks_set(
    Ewk_Context* context,
    Ewk_Context_Notification_Show_Callback show_callback,
    Ewk_Context_Notification_Cancel_Callback cancel_callback,
    void* user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(show_callback, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(cancel_callback, EINA_FALSE);

  context->SetNotificationCallbacks(show_callback, cancel_callback, user_data);
  return EINA_TRUE;
}

Eina_Bool ewk_context_notification_callbacks_reset(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);

  context->SetNotificationCallbacks(nullptr, nullptr, nullptr);
  return EINA_TRUE;
}

void ewk_context_intercept_request_callback_set(
    Ewk_Context* context, Ewk_Context_Intercept_Request_Callback callback,
    void* user_data) {
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetContextInterceptRequestCallback(callback, user_data);
}

void ewk_context_intercept_request_cancel_callback_set(
    Ewk_Context* context, Ewk_Context_Intercept_Request_Cancel_Callback callback,
    void* user_data) {
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetContextInterceptRequestCancelCallback(callback, user_data);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

void ewk_context_compression_proxy_enabled_set(Ewk_Context* context, Eina_Bool enabled)
{
  LOG_EWK_API_MOCKUP();
}

Eina_Bool ewk_context_compression_proxy_enabled_get(Ewk_Context* context)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

void ewk_context_compression_proxy_image_quality_set(Ewk_Context* context, Ewk_Compression_Proxy_Image_Quality quality)
{
  LOG_EWK_API_MOCKUP();
}

Ewk_Compression_Proxy_Image_Quality ewk_context_compression_proxy_image_quality_get(Ewk_Context* context)
{
  LOG_EWK_API_MOCKUP();
  return EWK_COMPRESSION_PROXY_IMAGE_QUALITY_LOW;
}

void ewk_context_compression_proxy_data_size_get(Ewk_Context* context, unsigned* original_size, unsigned* compressed_size)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_context_compression_proxy_data_size_reset(Ewk_Context* context)
{
  LOG_EWK_API_MOCKUP();
}

Eina_Bool ewk_context_audio_latency_mode_set(Ewk_Context* context, Ewk_Audio_Latency_Mode latency_mode)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->SetAudioLatencyMode(latency_mode);
  return EINA_TRUE;
}

Ewk_Audio_Latency_Mode ewk_context_audio_latency_mode_get(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EWK_AUDIO_LATENCY_MODE_ERROR);
  return context->GetAudioLatencyMode();
}

void ewk_context_time_offset_set(Ewk_Context* context, double time_offset)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetTimeOffset(time_offset);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

void ewk_context_timezone_offset_set(Ewk_Context* context,
                                     double time_zone_offset,
                                     double daylight_saving_time)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetTimeZoneOffset(time_zone_offset, daylight_saving_time);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

void ewk_context_max_refresh_rate_set(Ewk_Context* context, int max_refresh_rate)
{
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetMaxRefreshRate(max_refresh_rate);
}

Eina_Bool ewk_context_push_message_callback_set(Ewk_Context *context, Ewk_Push_Message_Cb callback, void *user_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->GetImpl()->SetPushMessageCallback(callback, user_data);
  return EINA_TRUE;
}

Eina_Bool ewk_context_send_push_message(Ewk_Context *context, const char *push_data)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(push_data, EINA_FALSE);
  return context->GetImpl()->DeliverPushMessage(push_data);
}

void ewk_context_proxy_set(Ewk_Context* context, const char* proxy, const char* bypass_rule)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetProxy(proxy, bypass_rule);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

const char* ewk_context_proxy_bypass_rule_get(Ewk_Context* context)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);
  return context->GetProxyBypassRule();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return NULL;
#endif
}

Eina_Bool ewk_context_proxy_default_auth_set(Ewk_Context* context, const char* username, const char* password)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  context->SetProxyDefaultAuth(username, password);
  return EINA_TRUE;
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return EINA_FALSE;
#endif
}

void ewk_context_password_confirm_popup_callback_set(Ewk_Context* context, Ewk_Context_Password_Confirm_Popup_Callback callback, void* user_data)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_context_password_confirm_popup_reply(Ewk_Context* context, Ewk_Context_Password_Popup_Option result)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_context_icon_database_delete_all(Ewk_Context* context)
{
  LOG_EWK_API_MOCKUP();
}

Eina_Bool ewk_context_check_accessible_path_callback_set(Ewk_Context* context, Ewk_Context_Check_Accessible_Path_Callback callback, void* user_data)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  context->SetCheckAccessiblePathCallback(callback, user_data);

  return EINA_TRUE;
#else
  return EINA_FALSE;
#endif
}

void ewk_context_register_url_schemes_as_cors_enabled(Ewk_Context* context, const Eina_List* schemes)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(context);
  EINA_SAFETY_ON_NULL_RETURN(schemes);
  context->RegisterURLSchemesAsCORSEnabled(schemes);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

void ewk_context_register_jsplugin_mime_types(Ewk_Context* context, const Eina_List* mime_types)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(context);
  EINA_SAFETY_ON_NULL_RETURN(mime_types);
  context->RegisterJSPluginMimeTypes(mime_types);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

void ewk_context_websdi_set(Eina_Bool enable)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  LOG(INFO) << "This App use WebSDI for client authentication [" << enable << "]";
  ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN(cbce);
  cbce->SetWebSDI(enable);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

Eina_Bool ewk_context_websdi_get()
{
#if defined(OS_TIZEN_TV_PRODUCT)
  const ContentBrowserClientEfl* cbce = GetContentBrowserClient();
  EINA_SAFETY_ON_NULL_RETURN_VAL(cbce, EINA_FALSE);
  return cbce->GetWebSDI();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return false;
#endif
}

void ewk_context_disable_nosniff_set(Ewk_Context* context, Eina_Bool enable)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_context_emp_certificate_file_set(Ewk_Context *context, const char* emp_ca_path, const char* default_ca_path)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(context);
  EINA_SAFETY_ON_NULL_RETURN(emp_ca_path);
  EINA_SAFETY_ON_NULL_RETURN(default_ca_path);
  context->SetEMPCertificatePath(emp_ca_path, default_ca_path);
#else
  LOG_EWK_API_MOCKUP();
#endif
}

Eina_Bool ewk_context_web_database_usage_for_origin_get(Ewk_Context* context, Ewk_Web_Database_Usage_Get_Callback callback, void* user_data, Ewk_Security_Origin* origin)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(callback, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(origin, EINA_FALSE);
  context->GetWebDBUsageForOrigin(origin->GetURL(), callback, user_data);
  return EINA_TRUE;
}

void ewk_context_form_password_data_update(Ewk_Context* context, const char* url, Eina_Bool useFingerprint)
{
  LOG_EWK_API_MOCKUP();
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_context_background_music_get(Ewk_Context* context) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return ewk_context_tizen_extensible_api_string_get(context, "background,music");
}

Eina_Bool ewk_context_background_music_set(Ewk_Context* context, Eina_Bool enable) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return ewk_context_tizen_extensible_api_string_set(context, "background,music", enable);
}

Eina_Bool ewk_context_block_multimedia_on_call_get(Ewk_Context* context) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return ewk_context_tizen_extensible_api_string_get(context, "block,multimedia,on,call");
}

Eina_Bool ewk_context_block_multimedia_on_call_set(Ewk_Context* context, Eina_Bool enable) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return ewk_context_tizen_extensible_api_string_set(context, "block,multimedia,on,call", enable);
}

Eina_Bool ewk_context_rotation_lock_get(Ewk_Context* context) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return ewk_context_tizen_extensible_api_string_get(context, "rotation,lock");
}

Eina_Bool ewk_context_rotation_lock_set(Ewk_Context* context, Eina_Bool enable) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return ewk_context_tizen_extensible_api_string_set(context, "rotation,lock", enable);
}

Eina_Bool ewk_context_sound_overlap_get(Ewk_Context* context) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return ewk_context_tizen_extensible_api_string_get(context, "sound,mode");
}

Eina_Bool ewk_context_sound_overlap_set(Ewk_Context* context, Eina_Bool enable) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, EINA_FALSE);
  return ewk_context_tizen_extensible_api_string_set(context, "sound,mode", enable);
}

/* LCOV_EXCL_START */
void ewk_context_service_worker_register(Ewk_Context *context, const char* scope_url, const char* script_url, Ewk_Context_Service_Worker_Registration_Result_Callback result_callback, void* user_data)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_context_service_worker_unregister(Ewk_Context *context, const char* scope_url, Ewk_Context_Service_Worker_Unregistration_Result_Callback result_callback, void* user_data)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_context_application_type_set(Ewk_Context* ewkContext, const Ewk_Application_Type applicationType)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(ewkContext);
  LOG(INFO)<<"Set current application type: " << applicationType;
  ewkContext->SetApplicationType(applicationType);

#if defined(TIZEN_PEPPER_EXTENSIONS)
  EwkExtensionSystemDelegate::SetEmbedderName(applicationType);
  pepper::UpdatePluginService(applicationType);
#endif  // defined(TIZEN_PEPPER_EXTENSIONS)

#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

Ewk_Application_Type ewk_context_application_type_get(Ewk_Context* ewkContext)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, EWK_APPLICATION_TYPE_OTHER);
  LOG(INFO)<<"Get current application type: " << ewkContext->GetApplicationType();
  return ewkContext->GetApplicationType();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
  return EWK_APPLICATION_TYPE_OTHER;
#endif
}

void ewk_context_url_maxchars_set(Ewk_Context* context, size_t max_chars)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetMaxURLChars(max_chars);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}

void ewk_context_cache_model_memory_cache_capacities_set(Ewk_Context* ewk_context, uint32_t cache_min_dead_capacity, uint32_t cache_max_dead_capacity, uint32_t cache_total_capacity)
{
  LOG_EWK_API_MOCKUP();
}

void ewk_context_default_zoom_factor_set(Ewk_Context* context, double zoom_factor)
{
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->SetDefaultZoomFactor(zoom_factor);
}

double ewk_context_default_zoom_factor_get(Ewk_Context* context)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(context, -1.0);
  return context->GetDefaultZoomFactor();
}

void ewk_context_enable_app_control(Ewk_Context *context, Eina_Bool enabled)
{
  EINA_SAFETY_ON_NULL_RETURN(context);
  context->EnableAppControl(enabled);
}
/* LCOV_EXCL_STOP */
