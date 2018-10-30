// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_SECURITY_STYLE_EXPLANATION_H_
#define CONTENT_PUBLIC_BROWSER_SECURITY_STYLE_EXPLANATION_H_

#include <string>

#include "content/common/content_export.h"
#include "net/cert/x509_certificate.h"
#include "third_party/WebKit/public/platform/WebMixedContentContextType.h"

namespace content {

// A human-readable summary phrase and more detailed description of a
// security property that was used to compute the SecurityStyle of a
// resource. An example summary phrase would be "Expired Certificate",
// with a description along the lines of "This site's certificate chain
// contains errors (net::CERT_DATE_INVALID)".
struct CONTENT_EXPORT SecurityStyleExplanation {
  SecurityStyleExplanation();
  SecurityStyleExplanation(const std::string& summary,
                           const std::string& description);
  SecurityStyleExplanation(
      const std::string& summary,
      const std::string& description,
      scoped_refptr<net::X509Certificate> certificate,
      blink::WebMixedContentContextType mixed_content_type);
  SecurityStyleExplanation(const SecurityStyleExplanation& other);
  SecurityStyleExplanation& operator=(const SecurityStyleExplanation& other);
  ~SecurityStyleExplanation();

  std::string summary;
  std::string description;
  // |certificate| indicates that this explanation has an associated
  // certificate.
  scoped_refptr<net::X509Certificate> certificate;
  // |mixed_content_type| indicates that the explanation describes a particular
  // type of mixed content. A value of kNotMixedContent means that the
  // explanation does not relate to mixed content. UI surfaces can use this to
  // customize the display of mixed content explanations.
  blink::WebMixedContentContextType mixed_content_type;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_SECURITY_STYLE_EXPLANATION_H_
