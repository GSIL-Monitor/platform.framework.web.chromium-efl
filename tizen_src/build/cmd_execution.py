#!/usr/bin/env python

import sys
import os

if __name__ == '__main__':

  l = len(sys.argv)
  c = 1;
  cmd = ""
  while c < l:
    cmd += sys.argv[c] + " "
    c += 1

  os.system(cmd)

  sys.exit(0)
