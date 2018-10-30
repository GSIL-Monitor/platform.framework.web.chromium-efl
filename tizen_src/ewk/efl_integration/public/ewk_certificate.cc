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

#include "ewk_certificate_internal.h"

#include <string>

#include "net/cert/cert_status_flags.h"
#include "private/ewk_certificate_info_private.h"
#include "private/ewk_certificate_private.h"
#include "private/ewk_private.h"

void ewk_certificate_policy_decision_allowed_set(Ewk_Certificate_Policy_Decision* certificatePolicyDecision, Eina_Bool allowed)
{
  EINA_SAFETY_ON_NULL_RETURN(certificatePolicyDecision);
  certificatePolicyDecision->SetDecision(allowed == EINA_TRUE);
}

Eina_Bool ewk_certificate_policy_decision_suspend(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, EINA_FALSE);
  return certificatePolicyDecision->Suspend();
}

Eina_Stringshare* ewk_certificate_policy_decision_url_get(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, "");
  return certificatePolicyDecision->url();
}

Eina_Stringshare* ewk_certificate_policy_decision_certificate_pem_get(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, "");
  return certificatePolicyDecision->certificate_pem();
}

Ewk_Certificate_Policy_Decision_Error ewk_certificate_policy_decision_error_get(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, EWK_CERTIFICATE_POLICY_DECISION_ERROR_UNKNOWN);
  net::CertStatus cert_status = net::MapNetErrorToCertStatus(certificatePolicyDecision->error());
  switch (cert_status) {
    case net::CERT_STATUS_COMMON_NAME_INVALID:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_COMMON_NAME_INVALID;
    case net::CERT_STATUS_DATE_INVALID:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_DATE_INVALID;
    case net::CERT_STATUS_AUTHORITY_INVALID:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_AUTHORITY_INVALID;
    case net::CERT_STATUS_NO_REVOCATION_MECHANISM:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_NO_REVOCATION_MECHANISM;
    case net::CERT_STATUS_UNABLE_TO_CHECK_REVOCATION:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_UNABLE_TO_CHECK_REVOCATION;
    case net::CERT_STATUS_REVOKED:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_REVOKED;
    case net::CERT_STATUS_INVALID:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_INVALID;
    case net::CERT_STATUS_WEAK_SIGNATURE_ALGORITHM:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_WEAK_ALGORITHM;
    case net::CERT_STATUS_NON_UNIQUE_NAME:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_NON_UNIQUE_NAME;
    case net::CERT_STATUS_WEAK_KEY:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_WEAK_KEY;
    case net::CERT_STATUS_PINNED_KEY_MISSING:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_PINNED_KEY_NOT_IN_CHAIN;
    case net::CERT_STATUS_NAME_CONSTRAINT_VIOLATION:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_NAME_VIOLATION;
    case net::CERT_STATUS_VALIDITY_TOO_LONG:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_VALIDITY_TOO_LONG;
    default:
      return EWK_CERTIFICATE_POLICY_DECISION_ERROR_UNKNOWN;
  }
}

const char* ewk_certificate_info_pem_get(const Ewk_Certificate_Info* cert_info)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(cert_info, nullptr);
  return cert_info->pem;
}

Eina_Bool ewk_certificate_info_is_context_secure(const Ewk_Certificate_Info* cert_info)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(cert_info, EINA_FALSE);
  return cert_info->is_context_secure;
}
Eina_Bool ewk_certificate_policy_decision_from_main_frame_get(const Ewk_Certificate_Policy_Decision* certificate_policy_decision)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(certificate_policy_decision, EINA_FALSE);
  return certificate_policy_decision->is_from_main_frame();
}
