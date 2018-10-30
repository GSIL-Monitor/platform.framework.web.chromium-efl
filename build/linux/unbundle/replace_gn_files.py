#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Replaces GN files in tree with files from here that
make the build use system libraries.
"""

from __future__ import print_function

import argparse
import os
import shutil
import sys


REPLACEMENTS = {
  'expat': 'third_party/expat/BUILD.gn',
  'ffmpeg': 'third_party/ffmpeg/BUILD.gn',
  'flac': 'third_party/flac/BUILD.gn',
  'freetype': 'third_party/freetype/BUILD.gn',
  'harfbuzz-ng': 'third_party/harfbuzz-ng/BUILD.gn',
  'icu': 'third_party/icu/BUILD.gn',
  'libdrm': 'third_party/libdrm/BUILD.gn',
  'libevent': 'base/third_party/libevent/BUILD.gn',
  'libjpeg': 'build/secondary/third_party/libjpeg_turbo/BUILD.gn',
  'libpng': 'third_party/libpng/BUILD.gn',
  'libvpx': 'third_party/libvpx/BUILD.gn',
  'libwebp': 'third_party/libwebp/BUILD.gn',
  'libxml': 'third_party/libxml/BUILD.gn',
  'libxslt': 'third_party/libxslt/BUILD.gn',
  'openh264': 'third_party/openh264/BUILD.gn',
  'opus': 'third_party/opus/BUILD.gn',
  're2': 'third_party/re2/BUILD.gn',
  'snappy': 'third_party/snappy/BUILD.gn',
  'yasm': 'third_party/yasm/yasm_assemble.gni',
  'zlib': 'third_party/zlib/BUILD.gn',
}


def DoMain(argv):
  my_dirname = os.path.dirname(__file__)
  source_tree_root = os.path.abspath(
    os.path.join(my_dirname, '..', '..', '..'))

  parser = argparse.ArgumentParser()
  parser.add_argument('--system-libraries', nargs='*', default=[])
  parser.add_argument('--undo', action='store_true')

  args = parser.parse_args(argv)

  handled_libraries = set()
  for lib, path in REPLACEMENTS.items():
    if lib not in args.system_libraries:
      continue
    handled_libraries.add(lib)

    if args.undo:
      try:
        # Restore original file, and also remove the backup.
        # This is meant to restore the source tree to its original state.
        os.rename(os.path.join(source_tree_root, path + '.orig'),
                  os.path.join(source_tree_root, path))
      except OSError:
        # .orig file may be not created (when we skip fetching the target
        # library due to unexpected termination), so just ignore the error.
        pass
    else:
      try:
        # Create a backup copy for --undo.
        shutil.copyfile(os.path.join(source_tree_root, path),
                        os.path.join(source_tree_root, path + '.orig'))
      except IOError:
        # This exception may happen when the target directory does not exist,
        # which is the case when we skip fetching the third_party library due to unexpected
        # termination. To proceed even in such case, let's create directory forcibly
        # to have the configuration file copied.
        target_dir = os.path.dirname(os.path.join(source_tree_root, path))
        if not os.path.isdir(target_dir):
          try:
            os.makedirs(target_dir)
          except OSError, e:
            if e.errno != errno.EEXIST:
              raise

      # Copy the GN file from directory of this script to target path.
      src = os.path.join(my_dirname, '%s.gn' % lib)
      dst = os.path.join(source_tree_root, path)
      print('** use system %s library **' % lib)
      print('  From %s\n  To %s' % (src, dst))
      shutil.copyfile(src, dst)
      src = os.path.join(my_dirname, '%s' % lib)
      # Copy the additional configuration files from directory of this script to target path.
      if os.path.isdir(src):
        dst = os.path.join(source_tree_root, 'third_party')
        os.system("cp -rf " + src + " " + dst)
        print('  From %s\n  To %s' % (src, dst))

  unhandled_libraries = set(args.system_libraries) - handled_libraries
  if unhandled_libraries:
    print('Unrecognized system libraries requested: %s' % ', '.join(
        sorted(unhandled_libraries)), file=sys.stderr)
    return 1

  return 0


if __name__ == '__main__':
  sys.exit(DoMain(sys.argv[1:]))
