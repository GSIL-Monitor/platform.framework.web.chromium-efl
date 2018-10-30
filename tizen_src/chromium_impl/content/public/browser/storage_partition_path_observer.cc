// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage_partition_path_observer.h"

#include "content/browser/storage_partition_impl.h"

namespace content {

StoragePartitionPathObserver::StoragePartitionPathObserver()
    : storage_partition_(nullptr) {}

StoragePartitionPathObserver::~StoragePartitionPathObserver() {
  if (storage_partition_) {
    storage_partition_->RemoveObserver(this);
    storage_partition_ = nullptr;
  }
}

void StoragePartitionPathObserver::StoragePartitionReady(
    StoragePartitionImpl* storage_partition) {
  DCHECK(!storage_partition_);
  storage_partition_ = storage_partition;
  storage_partition_->AddObserver(this);
}

void StoragePartitionPathObserver::StoragePartitionWillExit(
    StoragePartitionImpl* storage_partition) {
  DCHECK(storage_partition_);
  storage_partition_->RemoveObserver(this);
  storage_partition_ = nullptr;
}

}  // namespace content
