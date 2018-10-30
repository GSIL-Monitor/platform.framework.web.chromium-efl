// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVTOOLS_PORT_MANAGER_H_
#define DEVTOOLS_PORT_MANAGER_H_

#include "devtools_port_shared_memory.h"

#include <string>
#include <vector>

namespace devtools_http_handler {
enum DevToolsPortType {
  DevToolsPort_7011 = 7011,
  DevToolsPort_7012 = 7012,
  DevToolsPort_7013 = 7013
};

class DevToolsPortManager {
 public:
  bool IsPortUsed(const int port);
  bool GetValidPort(int& port);
  bool GetValidPort(int& port, int& pid);
  void GetMappingInfo(std::string& usedMappingInfo);
  bool ReleasePort();
  void SetPort(const int port) { usedPort = port; }
  int GetPort() { return usedPort; }
  void UpdatePortInfoInMem(int port);
  void DumpSharedMemeory();

  ~DevToolsPortManager();

  static DevToolsPortManager* GetInstance();
  static bool ProcessCompare();
  static std::string GetCurrentProcessName();
  static std::string GetUniqueName(const char* name);

 private:
  DevToolsPortManager(DevToolsPortSharedMemory*);
  bool IsValidPort(const int port);

  // PortCount|Port1|Port2|Port3
  int* m_UsedPortList;
  // pid1|pid2|pid3
  int* m_PIDList;
  // appName1|appName2|appName3
  char* m_AppName;

  DevToolsPortSharedMemory* m_DevToolsSHM;

  int usedPort;

  static DevToolsPortManager* s_PortManager;

  static const int s_ValidPortList[];
  static const int s_ValidPortCount;
  static const int s_DevToolsPortSharedMemorySize;
  static const int s_ProceeNameLength;
};
}  // namespace devtools_http_handler
#endif  // DEVTOOLS_PORT_MANAGER_H_
