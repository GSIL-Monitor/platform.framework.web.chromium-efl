#!/bin/bash
#
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

BUILD_PATH=$(readlink -f "$(dirname "$0")")
NDK_BUILD=$BUILD_PATH/../../../third_party/android_tools/ndk/ndk-build

export NDK_PROJECT_PATH=$BUILD_PATH

$NDK_BUILD APP_BUILD_SCRIPT=$NDK_PROJECT_PATH/Android.mk \
    NDK_APP_DST_DIR=$NDK_PROJECT_PATH && rm -rf $NDK_PROJECT_PATH/obj
