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

#include "ewk_policy_decision_internal.h"
#include "ewk_policy_decision_product.h"

#include "private/ewk_frame_private.h"
#include "private/ewk_policy_decision_private.h"
#include "private/ewk_private.h"
#include "web_contents_delegate_efl.h"

const char* ewk_policy_decision_cookie_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return policyDecision->GetCookie();  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
const char* ewk_policy_decision_userid_get(const Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return policyDecision->GetAuthUser();
}

const char* ewk_policy_decision_password_get(const Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return policyDecision->GetAuthPassword();
}
/* LCOV_EXCL_STOP */

const char* ewk_policy_decision_url_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return policyDecision->GetUrl();  // LCOV_EXCL_LINE
}

const char* ewk_policy_decision_scheme_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return policyDecision->GetScheme();  // LCOV_EXCL_LINE
}

const char* ewk_policy_decision_host_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return policyDecision->GetHost();  // LCOV_EXCL_LINE
}

const char* ewk_policy_decision_http_method_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return policyDecision->GetHttpMethod();
}

Ewk_Policy_Decision_Type ewk_policy_decision_type_get(const Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, EWK_POLICY_DECISION_USE);
  return policyDecision->GetDecisionType();  // LCOV_EXCL_LINE
}

const char* ewk_policy_decision_response_mime_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return policyDecision->GetResponseMime();  // LCOV_EXCL_LINE
}

const Eina_Hash* ewk_policy_decision_response_headers_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return policyDecision->GetResponseHeaders();  // LCOV_EXCL_LINE
}

int ewk_policy_decision_response_status_code_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);
  return policyDecision->GetResponseStatusCode();  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
Eina_Bool ewk_policy_decision_suspend(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, EINA_FALSE);
  return policyDecision->Suspend();
}
/* LCOV_EXCL_STOP */

Eina_Bool ewk_policy_decision_use(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, EINA_FALSE);
  /* LCOV_EXCL_START */
  policyDecision->Use();
  return EINA_TRUE;
  /* LCOV_EXCL_STOP */
}

Eina_Bool ewk_policy_decision_ignore(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, EINA_FALSE);
  /* LCOV_EXCL_START */
  policyDecision->Ignore();
  return EINA_TRUE;
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
Eina_Bool ewk_policy_decision_download(Ewk_Policy_Decision* policyDecision)
{
  LOG_EWK_API_MOCKUP("This API is deprecated");
  return EINA_FALSE;
}
/* LCOV_EXCL_STOP */

Ewk_Policy_Navigation_Type ewk_policy_decision_navigation_type_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, EWK_POLICY_NAVIGATION_TYPE_OTHER);
  return policyDecision->GetNavigationType();  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
Ewk_Frame_Ref ewk_policy_decision_frame_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return static_cast<Ewk_Frame_Ref>(policyDecision->GetFrameRef());
}

Eina_Bool ewk_policy_decision_is_main_frame(const Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, EINA_FALSE);
  return policyDecision->GetFrameRef()->IsMainFrame() ? EINA_TRUE : EINA_FALSE;
}

const char* ewk_policy_decision_http_body_get(Ewk_Policy_Decision* policyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, NULL);
  return policyDecision->GetHttpBody();
}
/* LCOV_EXCL_STOP */
