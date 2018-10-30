// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/shared_worker/shared_worker_factory_impl.h"

#include "base/memory/ptr_util.h"
#include "content/renderer/shared_worker/embedded_shared_worker_stub.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

// static
void SharedWorkerFactoryImpl::Create(
    mojom::SharedWorkerFactoryRequest request) {
  mojo::MakeStrongBinding<mojom::SharedWorkerFactory>(
      base::WrapUnique(new SharedWorkerFactoryImpl()), std::move(request));
}

SharedWorkerFactoryImpl::SharedWorkerFactoryImpl() {}

void SharedWorkerFactoryImpl::CreateSharedWorker(
    mojom::SharedWorkerInfoPtr info,
    bool pause_on_start,
    int32_t route_id,
    blink::mojom::WorkerContentSettingsProxyPtr content_settings,
    mojom::SharedWorkerHostPtr host,
    mojom::SharedWorkerRequest request) {
  // Bound to the lifetime of the underlying blink::WebSharedWorker instance.
  new EmbeddedSharedWorkerStub(std::move(info), pause_on_start, route_id,
                               std::move(content_settings), std::move(host),
                               std::move(request));
}

}  // namespace content
