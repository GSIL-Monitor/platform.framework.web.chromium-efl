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

#include "ewk_cookie_manager_internal.h"
#include "ewk_cookie_manager_product.h"
#include "ewk_log_internal.h"
#include "cookie_manager.h"
#include "private/ewk_cookie_manager_private.h"
#include "private/ewk_private.h"

// TODO: "ERR" should be changed to "CRITICAL"
// (http://web.sec.samsung.net/bugzilla/show_bug.cgi?id=15311)
#define EWK_COOKIE_MANAGER_GET_OR_RETURN(manager, cookie_manager, ...) \
  if (!(manager)) {                                                    \
    ERR("ewk cookie manager is NULL.");                                \
    return __VA_ARGS__;                                                \
  }                                                                    \
  if (!(manager)->cookieManager().get()) {                             \
    ERR("ewk cookie manager->cookieManager() is NULL.");               \
    return __VA_ARGS__;                                                \
  }                                                                    \
  scoped_refptr<CookieManager> cookie_manager = (manager)->cookieManager()

void ewk_cookie_manager_persistent_storage_set(Ewk_Cookie_Manager* manager,
                                               const char* file_name,
                                               Ewk_Cookie_Persistent_Storage storage)
{
  EWK_COOKIE_MANAGER_GET_OR_RETURN(manager, cookie_manager);
  EINA_SAFETY_ON_NULL_RETURN(file_name);
  std::string fileName(file_name);
  cookie_manager->SetStoragePath(file_name, true /*persist session*/, storage);
}

void ewk_cookie_manager_accept_policy_set(Ewk_Cookie_Manager* manager,
                                          Ewk_Cookie_Accept_Policy policy)
{
  EWK_COOKIE_MANAGER_GET_OR_RETURN(manager, cookie_manager);
  cookie_manager->SetCookiePolicy(policy);
}

void ewk_cookie_manager_accept_policy_async_get(const Ewk_Cookie_Manager* manager,
                                                Ewk_Cookie_Manager_Policy_Async_Get_Cb callback,
                                                void* data)
{
  EWK_COOKIE_MANAGER_GET_OR_RETURN(manager, cookie_manager);
  EINA_SAFETY_ON_NULL_RETURN(callback);

  cookie_manager->GetAcceptPolicyAsync(callback, data);
}

/* LCOV_EXCL_START */
void ewk_cookie_manager_async_hostnames_with_cookies_get(const Ewk_Cookie_Manager* manager,
                                                         Ewk_Cookie_Manager_Async_Hostnames_Get_Cb callback,
                                                         void* data)
{
  EWK_COOKIE_MANAGER_GET_OR_RETURN(manager, cookie_manager);
  EINA_SAFETY_ON_NULL_RETURN(callback);
  cookie_manager->GetHostNamesWithCookiesAsync(callback, data);
}

void ewk_cookie_manager_hostname_cookies_clear(Ewk_Cookie_Manager* manager,
                                               const char* host_name)
{
  EWK_COOKIE_MANAGER_GET_OR_RETURN(manager, cookie_manager);
  EINA_SAFETY_ON_NULL_RETURN(host_name);
  std::string url(host_name);
  cookie_manager->DeleteCookiesAsync(url);
}
/* LCOV_EXCL_STOP */

void ewk_cookie_manager_cookies_clear(Ewk_Cookie_Manager* manager)
{
  EWK_COOKIE_MANAGER_GET_OR_RETURN(manager, cookie_manager);
  cookie_manager->DeleteCookiesAsync();  // LCOV_EXCL_LINE
}  // LCOV_EXCL_LINE

Eina_Bool ewk_cookie_manager_file_scheme_cookies_allow_get(Ewk_Cookie_Manager* manager)
{
  EWK_COOKIE_MANAGER_GET_OR_RETURN(manager, cookie_manager, EINA_FALSE);
  return cookie_manager->IsFileSchemeCookiesAllowed();
}

void ewk_cookie_manager_file_scheme_cookies_allow_set(Ewk_Cookie_Manager* manager, Eina_Bool allow)
{
  EWK_COOKIE_MANAGER_GET_OR_RETURN(manager, cookie_manager);
  cookie_manager->SetAcceptFileSchemeCookies(allow);
}

/* LCOV_EXCL_START */
void ewk_cookie_manager_changes_watch(Ewk_Cookie_Manager* manager,
                                      Ewk_Cookie_Manager_Changes_Watch_Cb callback,
                                      void *data)
{
#if defined(OS_TIZEN_TV_PRODUCT) || defined(TIZEN_DESKTOP_SUPPORT)
  EWK_COOKIE_MANAGER_GET_OR_RETURN(manager, cookie_manager);
  cookie_manager->SetCookiesChangedCallback(callback, data);
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV");
#endif
}
/* LCOV_EXCL_STOP */
