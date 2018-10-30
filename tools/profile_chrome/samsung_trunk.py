#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import os
import time

from profile_chrome import chrome_controller
from profile_chrome import profiler
from profile_chrome import touch_util

from pylib import android_commands
from pylib.device import device_utils

class SamsungTestBase():
  def __init__(self):
    devices = android_commands.GetAttachedDevices()
    self._device = device_utils.DeviceUtils(devices[0])

    # FIXME: Remove a dependency about profiler and support for sbrowser.
    self._package_info = profiler.GetSupportedBrowsers()["chrome_shell"]

    categories = ["benchmark", "input"]
    #categories = ["benchmark", "input", "renderer", "disabled-by-default-gpu.debug*"]

    self._tracer = chrome_controller.ChromeTracingController(
        self._device, self._package_info, categories, None)

  def _StartBrowser(self, url):
    print("Load URL.. %s" % url)
    index = self._package_info[1].rfind(".")
    activity_name = self._package_info[0] + "/" + self._package_info[1][index:]
    os.system("adb shell am start -n '%s' '%s' > /dev/null 2>&1"
        % (activity_name, url))

  def _KillBrowser(self):
    package_name = self._package_info[0]
    os.system("adb shell am force-stop '%s' > /dev/null 2>&1"
        % package_name)

  def _PreProcess(self, parameter):
    return True

  def _DoSomething(self, parameter):
    return

  def _PostProcess(self, result):
    return

  def Test(self, parameter):
    if self._PreProcess(parameter):
      time.sleep(1)
      self._tracer.StartTracing(0)
      time.sleep(3)
      self._DoSomething(parameter)
      time.sleep(1)
      self._tracer.StopTracing()
      fd = open(self._tracer.PullTrace())
      jsonData = json.loads(fd.read())
      fd.close()
      self._PostProcess(parameter, jsonData)


class SamsungTestIRT(SamsungTestBase):
  def __init__(self):
    SamsungTestBase.__init__(self)

    # FIXME: We should fix double tap zoomin/zoomout.
    self._test = {
      "scroll" : {
        "url" : "http://m.naver.com",
        "do" : touch_util.TouchUtil.ScrollDown,
        "post" : "GestureScrollUpdate"
      },
      "zoomin" : {
        "url" : "http://www.naver.com/?mobile",
        "do" : touch_util.TouchUtil.ZoomIn,
        "post" : "GesturePinchUpdate"
      },
      "zoomout" : {
        "url" : "http://www.naver.com/?mobile",
        "pre" : touch_util.TouchUtil.DoubleTap,
        "do" : touch_util.TouchUtil.ZoomOut,
        "post" : "GesturePinchUpdate"
      },
      "dt_zoomin" : {
        "url" : "http://www.naver.com/?mobile",
        "do" : touch_util.TouchUtil.DoubleTap,
        "post" : "GestureDoubleTap"
      },
      "dt_zoomout" : {
        "url" : "http://www.naver.com/?mobile",
        "pre" : touch_util.TouchUtil.DoubleTap,
        "do" : touch_util.TouchUtil.DoubleTap,
        "post" : "GestureDoubleTap"
      },
    }

  def _PreProcess(self, gesture):
    if touch_util.TouchUtil.ShouldBeReady() != True:
      return False

    self._KillBrowser()

    test = self._test[gesture]
    self._StartBrowser(test["url"])

    # FIXME: How can we know finishing loading?
    time.sleep(10)

    if "pre" in test:
      print("Preset for %s test.." % gesture)
      test["pre"]()

    return True

  def _DoSomething(self, gesture):
    print("Perform %s test.." % gesture)
    self._test[gesture]["do"]()

  def _PostProcess(self, gesture, result):
    start_time = 0
    end_time = 0
    data_list = {}
    step_list = []
    BEGIN_SWAP = "INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT"
    END_SWAP = "INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT"

    for item in result["traceEvents"]:
      name = item["name"]
      args = item["args"]
      if name == "InputLatency":
        if "data" in args:
          data = args["data"]
          if BEGIN_SWAP in data and END_SWAP in data:
            data_list[item["id"]] = \
                int(data[END_SWAP]["time"]) - \
                int(data[BEGIN_SWAP]["time"])
            if gesture[:2] == "dt":
              data_list[item["id"]] = data_list[item["id"]] / 2
        elif "step" in args:
          step = args["step"]
          ts = item["ts"]
          if step == "TouchStart":
            if start_time == 0 or start_time > ts:
              start_time = int(ts)
          elif step == self._test[gesture]["post"]:
            step_list.append(item)

    for i, item in enumerate(step_list):
      id = item["id"]
      if id in data_list:
        end_time = int(item["ts"]) + data_list[id]
        break

    print("[IRT] %.3f ms" % ((end_time - start_time) / 1000.0))


class SamsungTestKeyLagging(SamsungTestBase):
  def _DoSomething(self, parameter):
    # keycodes: "whiteny houston" (FYI, This is not ascii codes)
    keycodes = [51, 36, 37, 48, 42, 33, 53, 62, 36, 43, 49, 47, 48, 43, 42]
    command = ""
    for keycode in keycodes:
      command += "input keyevent %d;" % keycode
    self._device.RunShellCommand(command)

  def _PostProcess(self, result):
    SWAP_KEY_STRING = "INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT"
    first = []
    second = {}

    for item in result["traceEvents"]:
      if item["name"] == "InputLatency":
        args = item["args"]
        if "step" in args and args["step"] == "Char":
          first.append(item)
        elif "data" in args and SWAP_KEY_STRING in args["data"]:
          second[item["id"]] = args["data"][SWAP_KEY_STRING]["time"]

    sum = 0.0
    for i, value in enumerate(first):
      diff = (second[value["id"]] - float(value["ts"])) / 1000.0
      print("[%s] %.3f ms" % (value["id"], diff))
      sum = sum + diff
    print("[AVG] %.3f ms" % (sum / len(first)))
