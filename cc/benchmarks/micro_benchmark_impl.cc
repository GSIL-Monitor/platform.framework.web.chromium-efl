// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/benchmarks/micro_benchmark_impl.h"

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/values.h"

namespace cc {

namespace {

void RunCallback(const MicroBenchmarkImpl::DoneCallback& callback,
                 std::unique_ptr<base::Value> result) {
  callback.Run(std::move(result));
}
}

MicroBenchmarkImpl::MicroBenchmarkImpl(
    const DoneCallback& callback,
    scoped_refptr<base::SingleThreadTaskRunner> origin_task_runner)
    : callback_(callback),
      is_done_(false),
      origin_task_runner_(origin_task_runner) {}

MicroBenchmarkImpl::~MicroBenchmarkImpl() {}

bool MicroBenchmarkImpl::IsDone() const {
  return is_done_;
}

void MicroBenchmarkImpl::DidCompleteCommit(LayerTreeHostImpl* host) {}

void MicroBenchmarkImpl::NotifyDone(std::unique_ptr<base::Value> result) {
  origin_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(RunCallback, callback_, base::Passed(&result)));
  is_done_ = true;
}

void MicroBenchmarkImpl::RunOnLayer(LayerImpl* layer) {}

void MicroBenchmarkImpl::RunOnLayer(PictureLayerImpl* layer) {}

}  // namespace cc
