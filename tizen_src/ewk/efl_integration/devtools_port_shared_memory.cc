// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "devtools_port_shared_memory.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

namespace devtools_http_handler {
const char* SEM_NAME_ARRAY[4] = {"DevToolsPort.lock", "DevToolsPort.lock_7011",
                                 "DevToolsPort.lock_7012",
                                 "DevToolsPort.lock_7013"};
#define INSPECTOR_PORT_7011 7011
#define INSPECTOR_PORT_7012 7012
#define INSPECTOR_PORT_7013 7013

DevToolsLocker::DevToolsLocker(int port) : m_sem(0) {
  const char* sem_name;
  if (!port)
    sem_name = SEM_NAME_ARRAY[0];
  else
    sem_name = SEM_NAME_ARRAY[port - 7010];
  m_port = port;
  m_sem = sem_open(sem_name, O_CREAT, 0644, 1);
  if (m_sem == SEM_FAILED)
    sem_unlink(sem_name);
}

DevToolsLocker::~DevToolsLocker() {
  const char* sem_name;
  if (!m_sem)
    return;
  if (!m_port)
    sem_name = SEM_NAME_ARRAY[m_port];
  else
    sem_name = SEM_NAME_ARRAY[m_port - 7010];
  sem_close(m_sem);
  sem_unlink(sem_name);
}

bool DevToolsLocker::Lock() {
  if (!m_sem)
    return false;

  int result = sem_wait(m_sem);

  return !result;
}

bool DevToolsLocker::TryLock() {
  if (!m_sem)
    return false;

  int result = sem_trywait(m_sem);

  return !result;
}

bool DevToolsLocker::Unlock() {
  if (!m_sem)
    return false;

  int result = sem_post(m_sem);

  return !result;
}

const char* DevToolsPortSharedMemory::s_SHMKey =
    "/WK2SharedMemory.inspector.port";

DevToolsPortSharedMemory::DevToolsPortSharedMemory() {
  for (int i = 0; i < 3; i++) {
    m_PortLocker[i] = new DevToolsLocker(i + 7011);
  }
}
DevToolsPortSharedMemory::~DevToolsPortSharedMemory() {
  if (!(*(int*)m_data))
    shm_unlink(s_SHMKey);

  for (int i = 0; i < 3; i++) {
    if (m_PortLocker[i])
      delete (m_PortLocker[i]);
  }
  munmap(m_data, m_size);
}
void DevToolsPortSharedMemory::Lock_Port(int port) {
  if (port >= INSPECTOR_PORT_7011 && port <= INSPECTOR_PORT_7013)
    m_PortLocker[port - INSPECTOR_PORT_7011]->Lock();
}
void DevToolsPortSharedMemory::UnLock_Port(int port) {
  if (port >= INSPECTOR_PORT_7011 && port <= INSPECTOR_PORT_7013)
    m_PortLocker[port - INSPECTOR_PORT_7011]->Unlock();
}
bool DevToolsPortSharedMemory::Lock() {
  return m_Locker.Lock();
}
bool DevToolsPortSharedMemory::Unlock() {
  return m_Locker.Unlock();
}

DevToolsPortSharedMemory* DevToolsPortSharedMemory::Create(size_t size) {
  int fd = shm_open(s_SHMKey, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if (fd == -1)
    return 0;

  while (ftruncate(fd, size) == -1) {
    if (errno != EINTR) {
      close(fd);
      shm_unlink(s_SHMKey);
      return 0;
    }
  }

  void* data = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    close(fd);
    shm_unlink(s_SHMKey);
    return 0;
  }

  close(fd);

  static DevToolsPortSharedMemory devtoolsSHM;
  devtoolsSHM.m_data = data;
  devtoolsSHM.m_size = size;

  return &devtoolsSHM;
}
}  // namespace devtools_http_handler
