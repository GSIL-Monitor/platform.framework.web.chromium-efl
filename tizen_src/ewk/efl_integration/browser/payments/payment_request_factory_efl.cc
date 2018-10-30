// Copyright 2017 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/payments/payment_request_factory_efl.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "browser/payments/payment_request_delegate_efl.h"
#include "components/payments/content/payment_request_web_contents_manager.h"
#include "components/payments/core/payment_request_delegate.h"
#include "content/public/browser/web_contents.h"

namespace payments {

void CreatePaymentRequestForWebContents(
    mojom::PaymentRequestRequest request,
    content::RenderFrameHost* render_frame_host) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents)
    return;
  PaymentRequestWebContentsManager::GetOrCreateForWebContents(web_contents)
      ->CreatePaymentRequest(
          render_frame_host, web_contents,
          base::MakeUnique<PaymentRequestDelegateEfl>(web_contents),
          std::move(request), nullptr);
}

}  // namespace payments
