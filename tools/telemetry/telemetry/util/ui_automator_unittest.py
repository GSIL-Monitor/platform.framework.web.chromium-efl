#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import unittest

from telemetry.core import util
from telemetry.util import ui_automator

class UIAutomatorTest(unittest.TestCase):
  def setUp(self):
    test_data_path = util.GetUnittestDataDir() + "/ui_automator.xml"
    self.ui_automator = ui_automator.UIAutomator()
    self.ui_automator.RunAutomatorForTest(test_data_path)

  def testGetBoundByClass(self):
    result= self.ui_automator.GetBoundByClass('android.widget.LinearLayout')
    self.assertEqual(result[0],"720 1280")

  def testGetBoundByResourceId(self):
    result = self.ui_automator.GetBoundByResourceId(\
        'com.sec.android.app.sbrowser:id/ph_toolbar_multiwindow_count')
    self.assertEqual(result[0],"1296 2464")
