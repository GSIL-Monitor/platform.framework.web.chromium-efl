#!/usr/bin/env python
#
# Copyright (c) 2016 Samsung Electronics Co., Ltd All Rights Reserved
#
# PROPRIETARY/CONFIDENTIAL
#
# This software is the confidential and proprietary information of
# SAMSUNG ELECTRONICS ("Confidential Information").
# You shall not disclose such Confidential Information and shall
# use it only in accordance with the terms of the license agreement
# you entered into with SAMSUNG ELECTRONICS. SAMSUNG make no
# representations or warranties about the suitability
# of the software, either express or implied, including but not
# limited to the implied warranties of merchantability, fitness for
# a particular purpose, or non-infringement. SAMSUNG shall not be liable
# for any damages suffered by licensee as a result of using, modifying
# or distributing this software or its derivatives.

import fcntl
import os
import socket
import struct
import sys

SCRIPT_PATH = os.path.abspath(os.path.dirname(__file__))
CHROMIUM_PATH = os.path.join(SCRIPT_PATH, os.pardir, os.pardir, os.pardir)
BOT_DEVICE_PATH = os.path.join(CHROMIUM_PATH, os.pardir, "bot", "device")
sys.path.append(BOT_DEVICE_PATH)

def GetIP():
  s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  ip_address = ""
  for eth_dev_ in os.listdir('/sys/class/net/'):
    try:
      if not eth_dev_.startswith("eth"):
        continue
      ip_address = socket.inet_ntoa(fcntl.ioctl(s.fileno(),
        0x8915, struct.pack('256s', eth_dev_))[20:24])
      if len(ip_address) != 0:
        break
    except Exception as e:
      print e.message
  return ip_address.strip()

def GetAgentName():
  path_list = os.path.dirname(os.path.abspath(__file__)).split('/')
  for path in path_list:
    if "buildagent_88" in path:
      return path.strip()
  return ""

def FilterAttachedDevices(devices):
  print "# FilterAttachedDevices : a log for checking the maintenance"
  try:
    from device_list import tester_ip
    from device_list import device_map
  except ImportError:
    print "# bot device path is wrong / not able to run in quickbuild"
    return devices

  agent_ = GetAgentName()
  ip_ = GetIP()
  tester_ = ""
  for tester_name in tester_ip.keys():
    if tester_ip.get(tester_name) == ip_:
      tester_ = tester_name
      break
  if not tester_ or not agent_:
    print "# ERR : tester_ or agent_ is empty " + tester_ + " " + agent_
    return devices

  print "# DEVICE INFO : " + " ".join([agent_, ip_, tester_])
  registed_device_list = set(device_map.get(tester_).get(agent_))
  if not registed_device_list:
    return devices
  healthy_device = set([device.adb.GetDeviceSerial() for device in devices])
  filtered_device = registed_device_list.intersection(healthy_device)
  test_devices = []
  for d in devices:
    if d.adb.GetDeviceSerial() in filtered_device:
      test_devices.append(d)
  return test_devices

if __name__ == '__main__':
  print "IP:" + GetIP()
  print "AGENT:" + GetAgentName()
