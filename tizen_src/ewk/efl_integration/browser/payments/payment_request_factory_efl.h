// Copyright 2017 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PAYMENT_REQUEST_FACTORY_H_
#define PAYMENT_REQUEST_FACTORY_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/WebKit/public/platform/modules/payments/payment_request.mojom.h"

namespace content {
class RenderFrameHost;
}

namespace payments {

// Will create a PaymentRequest attached to |web_contents|, based on the
// contents of |request|. This is called everytime a new Mojo PaymentRequest is
// created.
void CreatePaymentRequestForWebContents(
    mojom::PaymentRequestRequest request,
    content::RenderFrameHost* render_frame_host);

}  // namespace payments

#endif  // PAYMENT_REQUEST_FACTORY_H_
