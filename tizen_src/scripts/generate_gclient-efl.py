#!/usr/bin/env python

# Copyright 2015 Samsung Electronics. All rights reserved.
# Copyright (c) 2013 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This script is responsible for generating .gclient-efl in the top-level
source directory from DEPS.efl.

User-configurable values such as |cache_dir| are fetched from .gclient instead.
"""

import logging
import optparse
import os
import pprint


EFL_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GCLIENT_ROOT = os.path.dirname(os.path.dirname(EFL_ROOT))

def ParseGClientConfig():
  """
  Parses the top-level .gclient file (NOT .gclient-efl) and returns the
  values set there as a dictionary.
  """
  with open(os.path.join(GCLIENT_ROOT, '.gclient')) as dot_gclient:
    config = {}
    exec(dot_gclient, config)
  return config


def GenerateGClientEFL(options):
  with open(os.path.join(EFL_ROOT, 'DEPS.efl')) as deps_file:
    deps_contents = deps_file.read()

  gclient_config = ParseGClientConfig()
  if options.cache_dir:
    logging.warning('--cache_dir is deprecated and will be removed.'
                    'You should set cache_dir in .gclient instead.')
    cache_dir = options.cache_dir
  else:
    cache_dir = gclient_config.get('cache_dir')
  deps_contents += 'cache_dir = %s\n' % pprint.pformat(cache_dir)

  with open(os.path.join(GCLIENT_ROOT, '.gclient-efl'), 'w') as gclient_file:
    gclient_file.write(deps_contents)

def main():
  option_parser = optparse.OptionParser()
  # TODO(rakuco): Remove in Crosswalk 8.
  option_parser.add_option('--cache-dir',
                           help='DEPRECATED Set "cache_dir" in .gclient-efl '
                                'to this directory, so that all git '
                                'repositories are cached there.')
  options, _ = option_parser.parse_args()
  GenerateGClientEFL(options)

if __name__ == '__main__':
  main()
