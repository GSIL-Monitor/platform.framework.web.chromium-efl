# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import xml.etree.ElementTree as ET

from telemetry.core.backends import adb_commands

class UIAutomator(object):
  def __init__(self):
    self._ui_automator_command = "uiautomator dump "
    self._ui_dump_path = "/data/local/tmp/window_dump.xml"
    self._ui_pull_path = "/tmp/window_dump.xml"
    self._node_list = []
    self._SetupAdbCommands()

  def _SetupAdbCommands(self):
    serial = adb_commands.GetAttachedDevices()[0]
    self.adb = adb_commands.AdbCommands(device=serial)
    self.device = self.adb.device()

  def _ExecuteUIAutomator(self):
    self.device.RunShellCommand(self._ui_automator_command + self._ui_dump_path)
    self.device.PullFile(self._ui_dump_path, self._ui_pull_path)
    pass

  def _RecursiveNodeTraverse(self, root, depth):
    depth += 1
    for child in root:
      self._node_list.append(child.attrib)
      self._RecursiveNodeTraverse(child, depth)

  def _ParseXMLParser(self):
    self._node_list[:] = []
    tree = ET.parse(self._ui_pull_path)
    root = tree.getroot()
    self._RecursiveNodeTraverse(root, depth = 0)

  @staticmethod
  def _GetCenterPointOfBound(bound):
    coord = re.split(r",|\[|\]",bound)
    coord = filter(lambda x:x != '', coord)
    return "%d %d" % ((int(coord[0]) + int(coord[2])) / 2,
        (int(coord[1]) + int(coord[3])) / 2)

  def RunAutomator(self):
    self._ExecuteUIAutomator()
    self._ParseXMLParser()

  def RunAutomatorForTest(self, test_data_path):
    self._ui_pull_path = test_data_path
    self._ParseXMLParser()

  def GetBoundByClass(self, class_name):
    bound_list = []
    for node in self._node_list:
      if node['class'] == class_name:
        bound_list.append(self._GetCenterPointOfBound(node['bounds']))
    return bound_list

  def GetBoundByResourceId(self, resource_id):
    bound_list = []
    for node in self._node_list:
      if node['resource-id'] == resource_id:
        bound_list.append(self._GetCenterPointOfBound(node['bounds']))
    return bound_list
