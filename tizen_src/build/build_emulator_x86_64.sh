#!/bin/bash

. `dirname $0`/common.sh

setupAndExecuteTargetBuild unified --emulator "$@" -A x86_64
