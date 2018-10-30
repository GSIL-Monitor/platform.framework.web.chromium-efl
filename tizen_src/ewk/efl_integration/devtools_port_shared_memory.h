// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVTOOLS_PORT_SHARED_MEMORY_H_
#define DEVTOOLS_PORT_SHARED_MEMORY_H_

#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/file.h>

namespace devtools_http_handler {

class DevToolsLocker {
 public:
  DevToolsLocker(int port = 0);
  ~DevToolsLocker();
  bool Lock();
  bool TryLock();
  bool Unlock();

 private:
  sem_t* m_sem;
  int m_port;
};

class DevToolsPortSharedMemory {
 public:
  // Create a shared memory object with the given size. Will return 0 on
  // failure.
  static DevToolsPortSharedMemory* Create(size_t);

  ~DevToolsPortSharedMemory();

  size_t Size() const { return m_size; }
  void* Data() const { return m_data; }
  bool Lock();
  bool Unlock();
  void Lock_Port(int port);
  void UnLock_Port(int port);

 private:
  static const char* s_SHMKey;
  size_t m_size;
  void* m_data;
  DevToolsLocker m_Locker;
  DevToolsLocker* m_PortLocker[3];
  DevToolsPortSharedMemory();
};

class DevToolsLockHolder {
 private:
  DevToolsPortSharedMemory* m_lock;

 public:
  inline explicit DevToolsLockHolder(DevToolsPortSharedMemory* shmLock)
      : m_lock(shmLock) {
    shmLock->Lock();
  }
  inline ~DevToolsLockHolder() { m_lock->Unlock(); }
};
}  // namespace devtools_http_handler
#endif  // DEVTOOLS_PORT_SHARED_MEMORY_H_
