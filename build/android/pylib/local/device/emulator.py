# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import subprocess
import time

def is_emualtor():
  l = subprocess.check_output(["adb", "devices"])
  return "emulator-" in l

def reboot_emulator():
  logging.critical("# emulator rebooting")
  os.system("adb reboot")
  time.sleep(5)
  loop = True
  max_count = 0
  while loop and max_count < 60:
    l = subprocess.check_output(["adb", "devices"])
    loop = not ("emulator-" in l)
    max_count += 1
    time.sleep(1)
  time.sleep(5)
