#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from xml.dom import minidom
from writers import plist_helper
from writers import xml_formatted_writer


# This writer outputs a Preferences Manifest file as documented at
# https://developer.apple.com/library/mac/documentation/MacOSXServer/Conceptual/Preference_Manifest_Files


def GetWriter(config):
  '''Factory method for creating PListWriter objects.
  See the constructor of TemplateWriter for description of
  arguments.
  '''
  return PListWriter(['mac'], config)


class PListWriter(xml_formatted_writer.XMLFormattedWriter):
  '''Class for generating policy templates in Mac plist format.
  It is used by PolicyTemplateGenerator to write plist files.
  '''

  STRING_TABLE = 'Localizable.strings'
  TYPE_TO_INPUT = {
    'string': 'string',
    'int': 'integer',
    'int-enum': 'integer',
    'string-enum': 'string',
    'string-enum-list': 'array',
    'main': 'boolean',
    'list': 'array',
    'dict': 'dictionary',
    'external': 'dictionary',
  }

  def _AddKeyValuePair(self, parent, key_string, value_tag):
    '''Adds a plist key-value pair to a parent XML element.

    A key-value pair in plist consists of two XML elements next two each other:
    <key>key_string</key>
    <value_tag>...</value_tag>

    Args:
      key_string: The content of the key tag.
      value_tag: The name of the value element.

    Returns:
      The XML element of the value tag.
    '''
    self.AddElement(parent, 'key', {}, key_string)
    return self.AddElement(parent, value_tag)

  def _AddStringKeyValuePair(self, parent, key_string, value_string):
    '''Adds a plist key-value pair to a parent XML element, where the
    value element contains a string. The name of the value element will be
    <string>.

    Args:
      key_string: The content of the key tag.
      value_string: The content of the value tag.
    '''
    self.AddElement(parent, 'key', {}, key_string)
    self.AddElement(parent, 'string', {}, value_string)

  def _AddRealKeyValuePair(self, parent, key_string, value_string):
    '''Adds a plist key-value pair to a parent XML element, where the
    value element contains a real number. The name of the value element will be
    <real>.

    Args:
      key_string: The content of the key tag.
      value_string: The content of the value tag.
    '''
    self.AddElement(parent, 'key', {}, key_string)
    self.AddElement(parent, 'real', {}, value_string)

  def _AddTargets(self, parent, policy):
    '''Adds the following XML snippet to an XML element:
      <key>pfm_targets</key>
      <array>
        <string>user-managed</string>
      </array>

      Args:
        parent: The parent XML element where the snippet will be added.
    '''
    array = self._AddKeyValuePair(parent, 'pfm_targets', 'array')
    if self.CanBeRecommended(policy):
      self.AddElement(array, 'string', {}, 'user')
    if self.CanBeMandatory(policy):
      self.AddElement(array, 'string', {}, 'user-managed')

  def PreprocessPolicies(self, policy_list):
    return self.FlattenGroupsAndSortPolicies(policy_list)

  def WritePolicy(self, policy):
    policy_name = policy['name']
    policy_type = policy['type']

    dict = self.AddElement(self._array, 'dict')
    self._AddStringKeyValuePair(dict, 'pfm_name', policy_name)
    # Set empty strings for title and description. They will be taken by the
    # OSX Workgroup Manager from the string table in a Localizable.strings file.
    # Those files are generated by plist_strings_writer.
    self._AddStringKeyValuePair(dict, 'pfm_description', '')
    self._AddStringKeyValuePair(dict, 'pfm_title', '')
    self._AddTargets(dict, policy)
    self._AddStringKeyValuePair(dict, 'pfm_type',
                                self.TYPE_TO_INPUT[policy_type])
    if policy_type in ('int-enum', 'string-enum'):
      range_list = self._AddKeyValuePair(dict, 'pfm_range_list', 'array')
      for item in policy['items']:
        if policy_type == 'int-enum':
          element_type = 'integer'
        else:
          element_type = 'string'
        self.AddElement(range_list, element_type, {}, str(item['value']))
    elif policy_type in ('list', 'string-enum-list'):
      subkeys = self._AddKeyValuePair(dict, 'pfm_subkeys', 'array')
      subkeys_dict = self.AddElement(subkeys, 'dict')
      subkeys_type = self._AddKeyValuePair(subkeys_dict, 'pfm_type', 'string')
      self.AddText(subkeys_type, 'string')

  def BeginTemplate(self):
    self._plist.attributes['version'] = '1'
    dict = self.AddElement(self._plist, 'dict')
    if self._GetChromiumVersionString() is not None:
      self.AddComment(self._plist, self.config['build'] + ' version: ' + \
          self._GetChromiumVersionString())
    app_name = plist_helper.GetPlistFriendlyName(self.config['app_name'])
    self._AddStringKeyValuePair(dict, 'pfm_name', app_name)
    self._AddStringKeyValuePair(dict, 'pfm_description', '')
    self._AddStringKeyValuePair(dict, 'pfm_title', '')
    self._AddRealKeyValuePair(dict, 'pfm_version', '1')
    self._AddStringKeyValuePair(dict, 'pfm_domain',
                                self.config['mac_bundle_id'])

    self._array = self._AddKeyValuePair(dict, 'pfm_subkeys', 'array')

  def CreatePlistDocument(self):
    dom_impl = minidom.getDOMImplementation('')
    doctype = dom_impl.createDocumentType(
        'plist',
        '-//Apple//DTD PLIST 1.0//EN',
        'http://www.apple.com/DTDs/PropertyList-1.0.dtd')
    return dom_impl.createDocument(None, 'plist', doctype)

  def Init(self):
    self._doc = self.CreatePlistDocument()
    self._plist = self._doc.documentElement

  def GetTemplateText(self):
    return self.ToPrettyXml(self._doc)
