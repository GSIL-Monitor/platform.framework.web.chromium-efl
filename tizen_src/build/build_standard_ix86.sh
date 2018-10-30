#!/bin/bash

. `dirname $0`/common.sh

setupAndExecuteTargetBuild unified --standard "$@" -A i586
