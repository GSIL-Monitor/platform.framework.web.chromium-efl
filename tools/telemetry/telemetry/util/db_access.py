# Copyright 2015 Samsung Electronics. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.core.backends import adb_commands

class DeviceDBInterface(object):
  def __init__(self, application, database):
    self._serial = adb_commands.GetAttachedDevices()[0]
    self._adb = adb_commands.AdbCommands(device=self._serial)
    self._device = self._adb.device()
    self._db = '/data/data/%s/databases/%s' % (application, database)

  def SelectQuery(self, table, columns):
    columns_string = ', '.join(columns)
    query = 'sqlite3 %s \"select %s from \'%s\'\"'\
             % (self._db, columns_string, table)
    return self._device.RunShellCommand(query,
                                       check_return=True,
                                       large_output=True)

  def  InsertQuery(self, table, column_values):
    columns_string = '\', \''.join(column_values.keys())
    values_string = '\', \''.join(column_values.values())
    query = 'sqlite3 %s \"insert into %s(\'%s\') values(\'%s\')\"'\
             % (self._db, table, columns_string, values_string)
    return self._device.RunShellCommand(query,
                                       check_return=True,
                                       large_output=True)

  def  DeleteQuery(self, table, condition=None):
    query = 'sqlite3 %s \"delete from %s ' % (self._db, table)
    if condition != None:
      query += 'where'
      condition_query = ''
      for key, value in condition.iteritems():
        condition_query += ' %s = \'%s\' and' % (key, value)
      condition_query = condition_query[:-4]
      query += condition_query
      return self._device.RunShellCommand(query,
                                         check_return=True,
                                         large_output=True)
