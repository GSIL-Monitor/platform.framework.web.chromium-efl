// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_STORAGE_PARTITION_PATH_OBSERVER_H_
#define CONTENT_PUBLIC_STORAGE_PARTITION_PATH_OBSERVER_H_

#include "base/files/file_path.h"
#include "content/common/content_export.h"

namespace content {

class StoragePartitionImpl;

// An observer API implemented by classes which are interested
// in StoragePartition path updated events.
class CONTENT_EXPORT StoragePartitionPathObserver {
 public:
  // This method is invoked when the path of StoragePartition updated
  virtual void UpdatePath(const base::FilePath& path) {}

  // This method is invoked when the StoragePartition created
  void StoragePartitionReady(StoragePartitionImpl* storage_partition);

  // This method is invoked when the StoragePartition is going to exit
  void StoragePartitionWillExit(StoragePartitionImpl* storage_partition);

 protected:
  StoragePartitionPathObserver();
  virtual ~StoragePartitionPathObserver();

 private:
  StoragePartitionImpl* storage_partition_;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_STORAGE_PARTITION_PATH_OBSERVER_H_
