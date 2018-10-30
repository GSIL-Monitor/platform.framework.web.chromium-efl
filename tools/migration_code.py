#!/usr/bin/env python
#
# Copyright (c) 2017 Samsung Electronics. All Rights Reserved
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
# limited to the implied warranties of merchantability, fitness for a
# particular purpose, or non-infringement. SAMSUNG shall not be liable
# for any damages suffered by licensee as a result of using, modifying
# or distributing this software or its derivatives.
#

import json
import os
import re
import shutil
import sys

SCRIPT_PATH = os.path.abspath(os.path.dirname(__file__))
ROOT_PATH = os.path.join(SCRIPT_PATH, os.path.pardir)

def main(argv):
  if len(argv) != 2:
    print("wrong argument")
    print("command : migration_code.py [rule.json]")
    return 1

  file_name = argv[1]
  json_data = None
  with open(file_name, "r") as f:
    json_data = json.loads("".join(f.readlines()))
  for origin_file in json_data:
    source_path = os.path.join(ROOT_PATH, origin_file)
    json_item = json_data.get(origin_file)

    # copy
    json_dest = json_item.get("dest")
    if not json_dest:
      raise AssertionError("destination is mandatory")
    dest_path = os.path.join(ROOT_PATH, json_dest)
    copy_file(source_path, dest_path)

    # get source code
    source_code = None
    with open(dest_path, "r") as f:
      source_code = "".join(f.readlines())

    # replace header define
    source_code = replace_define_header(
        source_code, origin_file, json_dest)

    # comment one line
    json_comment_one_line = json_item.get("comment")
    if json_comment_one_line:
      for comment in json_comment_one_line:
        source_code = comment_one_line(source_code, comment)

    # comment header
    json_comment_header = json_item.get("comment_h")
    if json_comment_header:
      for comment in json_comment_header:
        source_code = commented_header(source_code, comment)

    json_comment_function = json_item.get("comment_func")
    if json_comment_function:
      for func in json_comment_function:
         source_code = commented_function(source_code, func)

    json_comment_lines = json_item.get("comment_from_to")
    if json_comment_lines:
      source_code = comment_from_to(source_code, json_comment_lines)

    # replace
    json_replace = json_item.get("replace")
    if json_replace:
      source_code = replace(source_code, json_replace)

    # write converted source code
    if source_code:
      with open(dest_path, "w") as f:
        f.write(source_code)

def copy_file(source, dest):
  s = os.path.split(source)[-1:][0]
  d = os.path.split(dest)[-1:][0]
  print("# copy from %s to %s" % (s, d))
  shutil.copy(source, dest)

def comment_one_line(source, line):
  out = []
  for l in source.split('\n'):
    if line in l:
        l = "// BY_SCRIPT " + l
    out.append(l)
  return "\n".join(out)

def commented_header(source, header):
  out = []
  for l in source.split('\n'):
    if "#include" in l and header in l:
        l = "// BY_SCRIPT " + l
    out.append(l)
  return "\n".join(out)

def commented_function(source, function):
  out = []
  start_function = False
  comment_now = False
  re_proto_type = re.compile(r'^(([A-Za-z*_<>:]+) ([A-Za-z*:_<>]+)\()')
  re_header = re.compile(r'^(([A-Za-z*_<>:]+) )')

  function_type = None
  for l in source.split('\n'):
    t = re_proto_type.search(l)
    h = re_header.search(l)
    if h:
      function_type = h.group(0).strip()
    if t and (function in l):
      start_function = True
      if '{' in l:
        comment_now = True
        ret = ''
        if function_type == 'void':
          ret = ' return;'
        if function_type == 'bool':
          ret = '\n  return false;'
        if function_type and ('*' in function_type or 'ptr' in function_type):
          ret = '\n  return nullptr;'
        out.append(l + ret)
        continue
    elif start_function and not comment_now and '{' in l:
      comment_now = True
      ret = ''
      if function_type == 'void':
        ret = ' return;'
      if function_type == 'bool':
        ret = '\n  return false;'
      if function_type and ('*' in function_type or 'ptr' in function_type):
        ret = '\n  return nullptr;'
      out.append(l + ret)
      continue
    if l.find('}') == 0:
      start_function = False
      comment_now = False
      function_type = None
    if comment_now:
      l = " " + l.rstrip() if len(l.rstrip()) else l.rstrip()
      out.append("// BY_SCRIPT" + l)
      continue
    out.append(l)
  return "\n".join(out)

def comment_from_to(source, lines):
  s = source.split('\n')
  for (i, k) in zip(lines[::2], lines[1::2]):
    start_index = 0
    end_index = 0
    for index in range(0, len(s)):
      if i in s[index]:
        start_index = index
      if k in s[index]:
        end_index = index
    for r in range(start_index, end_index + 1):
      s[r] = "// BY_SCRIPT" + s[r]
  return "\n".join(s)

def replace_define_header(source, source_path, dest_path):
  s = source_path.upper()
  s = s.replace(".", "_").replace("/", "_")
  d = dest_path.upper()
  d = d.replace(".", "_").replace("/", "_")
  source = source.replace(s, d)
  return source

def replace(source_code, json_replace):
  for i in range(0, len(json_replace), 2):
    original_word = json_replace[i]
    replace_word = json_replace[i+1]
    source_code = source_code.replace(original_word, replace_word)
  return source_code

if __name__ == '__main__':
  sys.exit(main(sys.argv))
