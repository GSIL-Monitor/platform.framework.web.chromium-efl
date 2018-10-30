#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import optparse
import sys

from profile_chrome import samsung_trunk

def _CreateOptionParser():
  parser = optparse.OptionParser()

  # FIXME: We should add selecting browser option.

  key_lagging_option = optparse.OptionGroup(parser, "Key Lagging Test")
  key_lagging_option.add_option("-k", "--key-lagging",
      help="Send 'whiteny houston' and then measure key lagging performance.",
      action="store_true")
  parser.add_option_group(key_lagging_option)

  irt_item = ["scroll", "zoomin", "zoomout", "dt_zoomin", "dt_zoomout", "all"]
  irt_option = optparse.OptionGroup(parser, "IRT Test")
  irt_option.add_option("-i", "--irt",
      help="Select test item for measurement of IRT. One of " +
      ", ".join(irt_item) + ". 'all' is used by default.",
      metavar="TEST", type="choice", choices=irt_item, default="all")
  parser.add_option_group(irt_option)

  return parser


def main():
  parser = _CreateOptionParser()

  if len(sys.argv) == 1:
    parser.print_help()
    return 0

  options, args = parser.parse_args()

  if options.key_lagging:
    samsung_trunk.SamsungTestKeyLagging().Test(None)
  else:
    samsung_trunk.SamsungTestIRT().Test(options.irt)


if __name__ == "__main__":
  main()
