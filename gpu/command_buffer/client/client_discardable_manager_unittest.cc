// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/client/client_discardable_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {
namespace {
class FakeCommandBuffer : public CommandBuffer {
 public:
  FakeCommandBuffer() = default;
  ~FakeCommandBuffer() override { EXPECT_TRUE(active_ids_.empty()); }
  // Overridden from CommandBuffer:
  State GetLastState() override {
    NOTREACHED();
    return State();
  };
  void Flush(int32_t put_offset) override { NOTREACHED(); }
  void OrderingBarrier(int32_t put_offset) override { NOTREACHED(); }
  State WaitForTokenInRange(int32_t start, int32_t end) override {
    NOTREACHED();

    return State();
  }
  State WaitForGetOffsetInRange(uint32_t set_get_buffer_count,
                                int32_t start,
                                int32_t end) override {
    NOTREACHED();
    return State();
  }
  void SetGetBuffer(int32_t transfer_buffer_id) override { NOTREACHED(); }
  scoped_refptr<gpu::Buffer> CreateTransferBuffer(size_t size,
                                                  int32_t* id) override {
    EXPECT_GE(size, 2048u);
    *id = next_id_++;
    active_ids_.insert(*id);
    std::unique_ptr<base::SharedMemory> shared_mem(new base::SharedMemory);
    shared_mem->CreateAndMapAnonymous(size);
    return MakeBufferFromSharedMemory(std::move(shared_mem), size);
  }
  void DestroyTransferBuffer(int32_t id) override {
    auto found = active_ids_.find(id);
    EXPECT_TRUE(found != active_ids_.end());
    active_ids_.erase(found);
  }

 private:
  int32_t next_id_ = 1;
  std::set<int32_t> active_ids_;
};

void UnlockClientHandleForTesting(
    const ClientDiscardableHandle& client_handle) {
  ServiceDiscardableHandle service_handle(client_handle.BufferForTesting(),
                                          client_handle.byte_offset(),
                                          client_handle.shm_id());
  service_handle.Unlock();
}

bool DeleteClientHandleForTesting(
    const ClientDiscardableHandle& client_handle) {
  ServiceDiscardableHandle service_handle(client_handle.BufferForTesting(),
                                          client_handle.byte_offset(),
                                          client_handle.shm_id());
  return service_handle.Delete();
}

void UnlockAndDeleteClientHandleForTesting(
    const ClientDiscardableHandle& client_handle) {
  UnlockClientHandleForTesting(client_handle);
  EXPECT_TRUE(DeleteClientHandleForTesting(client_handle));
}

}  // namespace

TEST(ClientDiscardableManagerTest, BasicUsage) {
  FakeCommandBuffer command_buffer;
  ClientDiscardableManager manager;
  {
    ClientDiscardableHandle handle =
        manager.InitializeTexture(&command_buffer, 1);
    EXPECT_TRUE(handle.IsLockedForTesting());
    EXPECT_EQ(handle.shm_id(), 1);
    EXPECT_FALSE(DeleteClientHandleForTesting(handle));
    UnlockClientHandleForTesting(handle);
    manager.LockTexture(1);
    EXPECT_FALSE(DeleteClientHandleForTesting(handle));
    UnlockAndDeleteClientHandleForTesting(handle);
  }
  manager.FreeTexture(1);
  manager.CheckPendingForTesting(&command_buffer);
}

TEST(ClientDiscardableManagerTest, Reuse) {
  FakeCommandBuffer command_buffer;
  ClientDiscardableManager manager;
  manager.SetElementCountForTesting(1024);
  for (int i = 1; i <= 1024; ++i) {
    ClientDiscardableHandle handle =
        manager.InitializeTexture(&command_buffer, i);
    EXPECT_TRUE(handle.IsLockedForTesting());
    EXPECT_EQ(handle.shm_id(), 1);
    UnlockAndDeleteClientHandleForTesting(handle);
  }
  // Delete every other entry.
  for (int i = 1; i <= 1024; i += 2) {
    manager.FreeTexture(i);
  }
  // Allocate 512 more entries, ensure we re-use the original buffer.
  for (int i = 1; i <= 512; ++i) {
    ClientDiscardableHandle handle =
        manager.InitializeTexture(&command_buffer, 1024 + i);
    EXPECT_TRUE(handle.IsLockedForTesting());
    EXPECT_EQ(handle.shm_id(), 1);
    UnlockAndDeleteClientHandleForTesting(handle);
  }
  // Delete the other half of the original allocations.
  for (int i = 2; i <= 1024; i += 2) {
    manager.FreeTexture(i);
  }
  // And delete the second set of allocations.
  for (int i = 1; i <= 512; ++i) {
    manager.FreeTexture(1024 + i);
  }
  manager.CheckPendingForTesting(&command_buffer);
}

TEST(ClientDiscardableManagerTest, MultipleAllocations) {
  FakeCommandBuffer command_buffer;
  ClientDiscardableManager manager;
  manager.SetElementCountForTesting(1024);
  for (int i = 1; i <= 1024; ++i) {
    ClientDiscardableHandle handle =
        manager.InitializeTexture(&command_buffer, i);
    EXPECT_TRUE(handle.IsLockedForTesting());
    EXPECT_EQ(handle.shm_id(), 1);
    UnlockAndDeleteClientHandleForTesting(handle);
  }
  // Allocate and free one entry multiple times, this should cause the
  // allocation and release of a new shm_id each time.
  for (int i = 1; i < 10; ++i) {
    {
      ClientDiscardableHandle handle =
          manager.InitializeTexture(&command_buffer, 1024 + i);
      EXPECT_TRUE(handle.IsLockedForTesting());
      EXPECT_EQ(handle.shm_id(), i + 1);
      UnlockAndDeleteClientHandleForTesting(handle);
    }
    manager.FreeTexture(1024 + i);
  }
  // Delete every other entry.
  for (int i = 1; i <= 1024; ++i) {
    manager.FreeTexture(i);
  }
  manager.CheckPendingForTesting(&command_buffer);
}

}  // namespace gpu
