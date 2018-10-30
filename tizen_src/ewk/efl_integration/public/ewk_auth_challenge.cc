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
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SAMSUNG ELECTRONICS. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ewk_auth_challenge_internal.h"

#include "private/ewk_auth_challenge_private.h"

const char* ewk_auth_challenge_realm_get(Ewk_Auth_Challenge* authChallenge)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(authChallenge, 0);
  return authChallenge->realm.c_str();
}

const char* ewk_auth_challenge_url_get(Ewk_Auth_Challenge* authChallenge)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(authChallenge, 0);
  return authChallenge->url.c_str();
}

void ewk_auth_challenge_suspend(Ewk_Auth_Challenge* authChallenge)
{
  EINA_SAFETY_ON_NULL_RETURN(authChallenge);
  authChallenge->is_suspended = true;
}

void ewk_auth_challenge_credential_use(Ewk_Auth_Challenge* authChallenge, const char* user, const char* password)
{
  EINA_SAFETY_ON_NULL_RETURN(authChallenge);
  EINA_SAFETY_ON_NULL_RETURN(user);
  EINA_SAFETY_ON_NULL_RETURN(password);

  authChallenge->is_decided = true;
  authChallenge->login_delegate->Proceed(user, password);
}

void ewk_auth_challenge_credential_cancel(Ewk_Auth_Challenge* authChallenge)
{
  EINA_SAFETY_ON_NULL_RETURN(authChallenge);
  authChallenge->is_decided = true;
  authChallenge->login_delegate->Cancel();
}
