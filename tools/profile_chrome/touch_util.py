#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys
import thread
import threading
import time

class TouchUtil:
  _touch_device = None
  _installed = False

  # cls._Execute() and cls._Adb() are not thead-safe because of pipe.
  # If you want thread-safe, you can use *Sync() functions (without pipe).
  @classmethod
  def _Execute(cls, command):
    fd = subprocess.Popen(command, shell=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return fd.stdout;

  @classmethod
  def _Adb(cls, command):
    fd = subprocess.Popen("adb shell \"%s\"" % command, shell=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return fd.stdout;

  @classmethod
  def _ExecuteSync(cls, command):
    os.system("%s > /dev/null 2>&1" % command)

  @classmethod
  def _AdbSync(cls, command):
    cls._ExecuteSync("adb shell \"%s\"" % command)

  @classmethod
  def _GetAllDevices(cls):
    out = cls._Adb("ls /dev/input/event*")
    return out.read().splitlines()

  @classmethod
  def _WaitTouchInput(cls):
    out = cls._Execute(
        "adb shell getevent | "
        "grep -m 1 '0003 0039 ffffffff' | "
        "grep -Eo -m 1 '/dev/input/event[0-9]*'")
    cls._touch_device = out.readline().rstrip()

  @classmethod
  def _Query(cls, device):
    cls._AdbSync("/system/touch_util %s tap -1 -1 1" % device)

  @classmethod
  def _QueryAllDevices(cls, devices):
    for device in devices:
      cls._Query(device.rstrip())

  @classmethod
  def _FindTouchDevice(cls):
    devices = cls._GetAllDevices()
    waiter = threading.Thread(target=cls._WaitTouchInput)
    waiter.start()
    finder = threading.Thread(target=cls._QueryAllDevices, args=[devices])
    finder.start()
    waiter.join(5)

  @classmethod
  def _IsInstalledBinary(cls):
    out = cls._Adb("/system/touch_util | grep -Eo touch_util")
    if out.readline().rstrip() == "touch_util":
      return True
    return False

  @classmethod
  def _InstallBinary(cls):
    if cls._IsInstalledBinary():
      return True

    pwd = "%s/touch_util" % os.path.dirname(__file__)
    cls._ExecuteSync("%s/build.sh > /dev/null 2>&1" % pwd)
    cls._AdbSync("su -c setenforce 0")
    cls._AdbSync("mount -o remount,rw /system")
    cls._ExecuteSync("adb push %s/touch_util /system/touch_util" % pwd)
    cls._ExecuteSync("rm -rf %s/touch_util > /dev/null 2>&1" % pwd)

    return cls._IsInstalledBinary()

  @classmethod
  def ShouldBeReady(cls):
    if cls._touch_device != None and cls._installed:
      return True

    cls._installed = cls._InstallBinary()
    if cls._installed == False:
      return False

    if cls._touch_device == None:
      cls._FindTouchDevice()

    return cls._touch_device != None and cls._installed

  @classmethod
  def _GetDisplaySize(cls):
    size = cls._Adb("wm size | grep -Eo '[0-9]+'").readlines()
    return int(size[0]), int(size[1])

  @classmethod
  def _Touch(cls, gesture, x, y, length):
    if cls.ShouldBeReady() == False:
      return

    if x == 0 and y == 0 and length == 0:
      width, height = cls._GetDisplaySize()
      x = width / 2
      y = height / 2
      length = height / 4

    cls._AdbSync("/system/touch_util %s %s %d %d %d"
        % (cls._touch_device, gesture, x, y, length))

  @classmethod
  def Tap(cls, x = 0, y = 0):
    cls._Touch("tap", x, y, 0)

  @classmethod
  def DoubleTap(cls, x = 0, y = 0):
    cls._Touch("doubletap", x, y, 0)

  @classmethod
  def ScrollDown(cls, x = 0, y = 0, length = 0):
    cls._Touch("scrolldown", x, y, length)

  @classmethod
  def ScrollUp(cls, x = 0, y = 0, length = 0):
    cls._Touch("scrollup", x, y, length)

  @classmethod
  def ZoomIn(cls, x = 0, y = 0, length = 0):
    cls._Touch("zoomin", x, y, length)

  @classmethod
  def ZoomOut(cls, x = 0, y = 0, length = 0):
    cls._Touch("zoomout", x, y, length)
