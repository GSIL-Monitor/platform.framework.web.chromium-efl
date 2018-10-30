// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/test/test_file_system_context.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "storage/browser/fileapi/external_mount_points.h"
#include "storage/browser/fileapi/file_system_backend.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/test/mock_special_storage_policy.h"
#include "storage/browser/test/test_file_system_backend.h"
#include "storage/browser/test/test_file_system_options.h"

namespace content {

storage::FileSystemContext* CreateFileSystemContextForTesting(
    storage::QuotaManagerProxy* quota_manager_proxy,
    const base::FilePath& base_path) {
  std::vector<std::unique_ptr<storage::FileSystemBackend>> additional_providers;
  additional_providers.push_back(base::MakeUnique<TestFileSystemBackend>(
      base::ThreadTaskRunnerHandle::Get().get(), base_path));
  return CreateFileSystemContextWithAdditionalProvidersForTesting(
      base::ThreadTaskRunnerHandle::Get().get(),
      base::ThreadTaskRunnerHandle::Get().get(), quota_manager_proxy,
      std::move(additional_providers), base_path);
}

storage::FileSystemContext*
CreateFileSystemContextWithAdditionalProvidersForTesting(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::SequencedTaskRunner> file_task_runner,
    storage::QuotaManagerProxy* quota_manager_proxy,
    std::vector<std::unique_ptr<storage::FileSystemBackend>>
        additional_providers,
    const base::FilePath& base_path) {
  return new storage::FileSystemContext(
      io_task_runner.get(), file_task_runner.get(),
      storage::ExternalMountPoints::CreateRefCounted().get(),
      base::MakeRefCounted<MockSpecialStoragePolicy>().get(),
      quota_manager_proxy, std::move(additional_providers),
      std::vector<storage::URLRequestAutoMountHandler>(), base_path,
      CreateAllowFileAccessOptions());
}

storage::FileSystemContext* CreateFileSystemContextWithAutoMountersForTesting(
    storage::QuotaManagerProxy* quota_manager_proxy,
    std::vector<std::unique_ptr<storage::FileSystemBackend>>
        additional_providers,
    const std::vector<storage::URLRequestAutoMountHandler>& auto_mounters,
    const base::FilePath& base_path) {
  return new storage::FileSystemContext(
      base::ThreadTaskRunnerHandle::Get().get(),
      base::ThreadTaskRunnerHandle::Get().get(),
      storage::ExternalMountPoints::CreateRefCounted().get(),
      base::MakeRefCounted<MockSpecialStoragePolicy>().get(),
      quota_manager_proxy, std::move(additional_providers), auto_mounters,
      base_path, CreateAllowFileAccessOptions());
}

storage::FileSystemContext* CreateIncognitoFileSystemContextForTesting(
    storage::QuotaManagerProxy* quota_manager_proxy,
    const base::FilePath& base_path) {
  return new storage::FileSystemContext(
      base::ThreadTaskRunnerHandle::Get().get(),
      base::ThreadTaskRunnerHandle::Get().get(),
      storage::ExternalMountPoints::CreateRefCounted().get(),
      base::MakeRefCounted<MockSpecialStoragePolicy>().get(),
      quota_manager_proxy,
      std::vector<std::unique_ptr<storage::FileSystemBackend>>(),
      std::vector<storage::URLRequestAutoMountHandler>(), base_path,
      CreateIncognitoFileSystemOptions());
}

}  // namespace content
