// Copyright 2014-17 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_CERTIFICATE_PRIVATE_H_
#define EWK_CERTIFICATE_PRIVATE_H_

#include <string>
#include <Eina.h>

#include "base/callback.h"
#include "content/public/browser/certificate_request_result_type.h"
#include "url/gurl.h"

struct _Ewk_Certificate_Policy_Decision {
 public:
  _Ewk_Certificate_Policy_Decision(
      const GURL& url,
      const std::string& cert,
      bool is_from_main_frame,
      int error_code,
      const base::Callback<void(content::CertificateRequestResultType)>&
          result_callback);
  ~_Ewk_Certificate_Policy_Decision();

  bool SetDecision(bool allowed);
  void Ignore();
  bool Suspend();
  bool IsDecided() const { return is_decided_; }
  bool IsSuspended() const { return is_suspended_; }

  Eina_Stringshare* url() const { return url_; }
  Eina_Stringshare* certificate_pem() const { return certificate_pem_; }
  bool is_from_main_frame() const { return is_from_main_frame_; }
  int error() const { return error_; }

 private:
  bool is_decided_;
  bool is_suspended_;

  Eina_Stringshare* url_;
  Eina_Stringshare* certificate_pem_;
  bool is_from_main_frame_;
  int error_;

  // run when policy is set by app
  base::Callback<void(content::CertificateRequestResultType)> callback_;
};

#endif /* EWK_CERTIFICATE_PRIVATE_H_ */
