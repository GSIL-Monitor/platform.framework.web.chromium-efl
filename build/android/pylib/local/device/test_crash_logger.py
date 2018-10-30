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

import glob
import json
import logging
import os
import requests
import subprocess
import threading

from pylib import constants
from pylib.base import base_test_result

class Command(object):
  def __init__(self, command, timeout=120):
    self.command = command
    self.process = None
    self.output = None
    self.error = None
    self.timeout = timeout

  def run(self):
    def target():
      self.process = subprocess.Popen(self.command,
                                      stdout=subprocess.PIPE, shell=True)
      self.output, self.error = self.process.communicate()
      if self.error:
        print "# Error command %s : error %s" % (self.command, self.error)
    thread = threading.Thread(target=target)
    thread.start()
    thread.join(self.timeout)
    if thread.is_alive():
      print "# error timeout"
      self.process.terminate()
      thread.join()
    return self

  def get_output(self):
    return self.output

def leave_logs_to_hastebin(data):
  URL = "http://web.sec.samsung.net/hastebin"
  response = requests.post(URL + "/documents", data)
  return "%s/%s\n" % (URL, json.loads(response.text)['key'])


def print_unknown_error(device, result, quickbuild):
  if 'UiObjectNotFoundException' in result.GetLog() or \
     result.GetType() in (base_test_result.ResultType.UNKNOWN,
                          base_test_result.ResultType.CRASH):
    unknown_log = "--------- TEST NAME %s\n" % result
    logging_categories = ["System.err:V", "chromium:V",
                          "TestRunner:V", "DEBUG:I", "AndroidRuntime:E"]
    shell_command = "logcat -d -s " + " ".join(logging_categories) + "-t 1000"
    unknown_log += "\n".join(device.RunShellCommand(shell_command))
    if quickbuild:
      logging.critical('#' * 40 + '\n')
      logging.critical("[UKNOWN ERR] check the URL: " + \
          leave_logs_to_hastebin(unknown_log))
    else:
      logging.critical(unknown_log)
    device.RunShellCommand("logcat -c")

def print_crash_dump(devices, quickbuild):
  def make_short_dump(file_name):
    dump_ = ""
    with open(file_name, "r") as f:
      dump_data = "\n".join(f.readlines())
      dump_ = dump_data[:dump_data.find("stack:")]
    with open(file_name, "w") as f:
      f.write(dump_)

  logging.critical('#' * 40 + '\n')
  logging.critical('# NATIVE CRASH LOGS\n\n')
  ndk_path = os.path.join(constants.ANDROID_NDK_ROOT, "ndk-stack")
  out_lib_path = os.path.join(constants.GetOutDirectory(), "lib.unstripped")

  for device in devices:
    dump_path = "/tmp/crash_tombstones/"
    copied_path = os.path.join(dump_path, "tombstones", "*")
    Command("mkdir -p %s" % dump_path).run()
    Command("rm -rf %s*" % dump_path).run()
    device.RunShellCommand("mkdir -p /data/tombstones")
    device.PullFile("/data/tombstones/", dump_path)

    crash_log = ""
    for dump_file in glob.glob(copied_path):
      if os.path.isdir(dump_file):
        continue
      make_short_dump(dump_file)
      crash_log += '-' * 50 + '\n'
      crash_log += "FILE NAME : %s\n" % dump_file
      ndk_cmd = "%s -sym %s -dump %s" % (ndk_path, out_lib_path, dump_file)
      crash_log += Command(ndk_cmd).run().get_output()
    if not crash_log:
      continue
    if quickbuild:
      # if it's running on quickbuild, leave logs in hastebin
      logging.critical("[CRASH ERR] Check the URL: " + \
          leave_logs_to_hastebin(crash_log))
      # write crash log file for post landing test
      with open(os.path.join(constants.GetOutDirectory(),
          "native_crash_log.txt"), "w") as f:
        f.write(crash_log)
    else:
      logging.critical(crash_log)
  logging.critical('#' * 40 + '\n\n')
