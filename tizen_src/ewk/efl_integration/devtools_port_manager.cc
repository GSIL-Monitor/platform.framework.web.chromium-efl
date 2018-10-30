// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "devtools_port_manager.h"

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include "base/logging.h"
#include "devtools_port_msg.h"
#if defined(OS_TIZEN)
#include <vconf/vconf.h>
#endif
namespace devtools_http_handler {

DevToolsPortManager* DevToolsPortManager::s_PortManager = 0;
const int DevToolsPortManager::s_ValidPortList[] = {
    DevToolsPort_7011, DevToolsPort_7012, DevToolsPort_7013};
const int DevToolsPortManager::s_ValidPortCount =
    sizeof(s_ValidPortList) / sizeof(int);
const int DevToolsPortManager::s_ProceeNameLength = 128;
// The size of SHM is composed of
// PortCount|Port1|Port2|Port3|pid1|pid2|pid3|appName1|appName2|appName3
const int DevToolsPortManager::s_DevToolsPortSharedMemorySize =
    sizeof(int) + DevToolsPortManager::s_ValidPortCount * sizeof(int) * 2 +
    s_ProceeNameLength * sizeof(char) * 3;

DevToolsPortManager* DevToolsPortManager::GetInstance() {
  if (!s_PortManager) {
    DevToolsPortSharedMemory* dpsm =
        DevToolsPortSharedMemory::Create(s_DevToolsPortSharedMemorySize);
    if (dpsm) {
      char* memp = (char*)dpsm->Data();
      s_PortManager = new DevToolsPortManager(dpsm);
      s_PortManager->m_UsedPortList = (int*)(dpsm->Data());
      s_PortManager->m_PIDList =
          s_PortManager->m_UsedPortList + s_ValidPortCount + 1;
      s_PortManager->m_AppName = memp;
      s_PortManager->m_AppName +=
          sizeof(int) + DevToolsPortManager::s_ValidPortCount * sizeof(int) * 2;
    }
  }

  return s_PortManager;
}

DevToolsPortManager::DevToolsPortManager(DevToolsPortSharedMemory* p)
    : m_UsedPortList(0), m_DevToolsSHM(p), usedPort(0) {}

bool DevToolsPortManager::IsPortUsed(const int port) {
  DevToolsLockHolder h(m_DevToolsSHM);

  int usedPortCount = m_UsedPortList[0];
  if (!usedPortCount)
    return false;

  if (usedPortCount <= s_ValidPortCount) {
    while (usedPortCount > 0) {
      if (m_UsedPortList[usedPortCount] == port &&
          getpid() == m_PIDList[usedPortCount - 1])
        return true;

      usedPortCount--;
    }
  }

  return false;
}

bool DevToolsPortManager::GetValidPort(int& port) {
  int pid = 0;

  if (GetValidPort(port, pid)) {
    if (pid) {
      DevToolsPortMsg* dpm = new DevToolsPortMsg();
      dpm->Send(pid, port);
    }
    m_DevToolsSHM->Lock_Port(port);
    if (pid)
      UpdatePortInfoInMem(port);
    return true;
  }

  return false;
}
bool DevToolsPortManager::GetValidPort(int& port, int& pid) {
  DevToolsLockHolder h(m_DevToolsSHM);

  int usedPortCount = m_UsedPortList[0];
  if (usedPortCount == s_ValidPortCount) {
    port = m_UsedPortList[1];
    pid = m_PIDList[0];
  } else {
    int i = 0;
    int j = 0;
    bool selected = 0;
    std::string processName = GetCurrentProcessName();
    for (i = 0; i < s_ValidPortCount; i++) {
      selected = 0;
      for (j = 1; j <= usedPortCount; j++) {
        if (s_ValidPortList[i] == m_UsedPortList[j])
          selected = 1;
      }
      if (!selected)
        break;
    }
    if (i < s_ValidPortCount && !selected) {
      usedPortCount++;
      m_UsedPortList[usedPortCount] = s_ValidPortList[i];
      m_PIDList[usedPortCount - 1] = getpid();
      memset(m_AppName + (usedPortCount - 1) * s_ProceeNameLength, 0,
             s_ProceeNameLength);
      memcpy(m_AppName + (usedPortCount - 1) * s_ProceeNameLength,
             processName.c_str(), processName.length());
      m_UsedPortList[0] = usedPortCount;
      port = s_ValidPortList[i];
    } else
      return false;
  }
  return true;
}
void DevToolsPortManager::UpdatePortInfoInMem(int port) {
  DevToolsLockHolder h(m_DevToolsSHM);
  std::string processName = GetCurrentProcessName();
  int usedPortCount = m_UsedPortList[0];

  if (usedPortCount < s_ValidPortCount) {
    usedPortCount++;
    m_UsedPortList[usedPortCount] = port;
    m_PIDList[usedPortCount - 1] = getpid();
    memset(m_AppName + (usedPortCount - 1) * s_ProceeNameLength, 0,
           s_ProceeNameLength);
    memcpy(m_AppName + (usedPortCount - 1) * s_ProceeNameLength,
           processName.c_str(), processName.length());
    m_UsedPortList[0] = usedPortCount;
  }
}

bool DevToolsPortManager::ReleasePort() {
  DevToolsLockHolder h(m_DevToolsSHM);
  std::string PrcocessName;
  PrcocessName = DevToolsPortManager::GetCurrentProcessName();
  int i;
  int port = 0;
  int usedPortCount = m_UsedPortList[0];
  bool isPortExist = false;
  if (usedPort)
    port = usedPort;

  if (!usedPortCount)
    return false;
  for (i = 1; i <= usedPortCount; i++) {
    if (m_UsedPortList[i] == port && getpid() == m_PIDList[i - 1]) {
      isPortExist = true;
      break;
    }
  }
  if (isPortExist) {
    m_UsedPortList[i] = 0;
    m_PIDList[i - 1] = 0;
    memset(m_AppName + (i - 1) * s_ProceeNameLength, 0, s_ProceeNameLength);
    for (int k = i + 1; k <= usedPortCount; k++) {
      m_UsedPortList[i] = m_UsedPortList[k];
      m_PIDList[i - 1] = m_PIDList[k - 1];
      memset(m_AppName + (i - 1) * s_ProceeNameLength, 0, s_ProceeNameLength);
      memcpy(m_AppName + (i - 1) * s_ProceeNameLength,
             m_AppName + (k - 1) * s_ProceeNameLength, s_ProceeNameLength);
      i++;
    }
    m_UsedPortList[usedPortCount] = 0;
    m_PIDList[usedPortCount - 1] = 0;
    memset(m_AppName + (usedPortCount - 1) * s_ProceeNameLength, 0,
           s_ProceeNameLength);
    usedPortCount--;
    m_UsedPortList[0] = usedPortCount;
    m_DevToolsSHM->UnLock_Port(usedPort);
  }
  return isPortExist;
}

bool DevToolsPortManager::IsValidPort(const int port) {
  for (int i = 0; i < s_ValidPortCount; i++) {
    if (s_ValidPortList[i] == port)
      return true;
  }

  return false;
}

void DevToolsPortManager::GetMappingInfo(std::string& usedMappingInfo) {
  DevToolsLockHolder h(m_DevToolsSHM);

  int usedPortCount = m_UsedPortList[0];
  std::string processName;
  char portNumber[] = "  99999";
  bool isPortExist = false;

  if (usedPortCount <= 0 || usedPortCount > s_ValidPortCount)
    return;

  usedMappingInfo.append("RWI mapping info \n");

  for (int i = 0; i < s_ValidPortCount; i++) {
    isPortExist = false;

    char processName[s_ProceeNameLength];

    for (int j = 1; j <= usedPortCount; j++) {
      if (s_ValidPortList[i] == m_UsedPortList[j]) {
        isPortExist = true;
        for (int k = 0; k < s_ProceeNameLength; k++)
          processName[k] = m_AppName[k + (j - 1) * s_ProceeNameLength];

        break;
      }
    }

    snprintf(portNumber, sizeof(portNumber), "  %d", s_ValidPortList[i]);
    usedMappingInfo.append(portNumber);
    usedMappingInfo.append("(RWI):");
    if (isPortExist)
      usedMappingInfo.append(processName);
    else
      usedMappingInfo.append("N/A");

    usedMappingInfo.append("\n");
  }
}

bool DevToolsPortManager::ProcessCompare() {
  std::string processName = GetCurrentProcessName();
#if defined(OS_TIZEN)
  char* vconfWidgetName = vconf_get_str("db/rwi/inspected_appid");
#else
  char* vconfWidgetName = NULL;
#endif
  if (!vconfWidgetName) {
    LOG(INFO) << "[RWI]  vconf_ None";
    return false;
  }

  std::string widgetName = vconfWidgetName;
  free(vconfWidgetName);
  LOG(INFO) << "[RWI] WidgetName = " << widgetName.c_str();
  LOG(INFO) << "[RWI] ProcessName = " << processName.c_str();

  if (!processName.empty()) {
    if (widgetName == "org.tizen.browser")
      widgetName = "browser";

    if (processName == widgetName)
      return true;
  }

  return false;
}

std::string DevToolsPortManager::GetCurrentProcessName() {
  std::string processName = GetUniqueName("/proc/self/cmdline");

  if (!processName.empty()) {
    unsigned int slashIndex = processName.rfind('/');

    if (slashIndex != std::string::npos && slashIndex < processName.size())
      processName = processName.substr(slashIndex + 1);
  }

  return processName;
}

std::string DevToolsPortManager::GetUniqueName(const char* name) {
  if (!access(name, F_OK)) {
    FILE* file = fopen(name, "r");

    if (file) {
      const int MAX_BUFF = 100;
      char buff[MAX_BUFF] = {
          0,
      };
      unsigned index = 0;

      while (index < MAX_BUFF) {
        int ch = fgetc(file);
        if (ch == EOF)
          break;

        buff[index] = ch;
        index++;
      }

      fclose(file);
      return std::string(buff);
    }
  }
  return std::string();
}
}  // namespace devtools_http_handler
