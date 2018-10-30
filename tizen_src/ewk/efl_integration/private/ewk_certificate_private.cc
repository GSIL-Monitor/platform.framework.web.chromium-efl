// Copyright 2016-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_certificate_private.h"

#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

_Ewk_Certificate_Policy_Decision::_Ewk_Certificate_Policy_Decision(
    const GURL& url,
    const std::string& cert,
    bool is_from_main_frame,
    int error_code,
    const base::Callback<void(content::CertificateRequestResultType)>&
        result_callback)
    : url_(nullptr),
      certificate_pem_(nullptr),
      is_from_main_frame_(is_from_main_frame),
      error_(error_code),
      callback_(result_callback) {
  is_decided_ = false;
  is_suspended_ = false;
  url_ = eina_stringshare_add(url.spec().c_str());
  certificate_pem_ = eina_stringshare_add(cert.c_str());
}

_Ewk_Certificate_Policy_Decision::~_Ewk_Certificate_Policy_Decision() {
  eina_stringshare_del(url_);
  eina_stringshare_del(certificate_pem_);
}

bool _Ewk_Certificate_Policy_Decision::SetDecision(bool allowed) {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!is_decided_) {
    is_decided_ = true;

    if (!callback_.is_null()) {
      callback_.Run(allowed ?
          content::CERTIFICATE_REQUEST_RESULT_TYPE_CONTINUE :
          content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
    }

    if (is_suspended_) {
      // If decision was suspended, then it was not deleted by the creator
      // Deletion of this object must be done after decision was made, as
      // this object will no longer be valid. But if decision was not suspended
      // it will be deleted right after permission callbacks are executed.
      BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
    }
    return true;
  }
  return false;
}

void _Ewk_Certificate_Policy_Decision::Ignore() {
  is_decided_ = true;
}

bool _Ewk_Certificate_Policy_Decision::Suspend() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (!is_decided_) {
    is_suspended_ = true;
    return true;
  }
  return false;
}
