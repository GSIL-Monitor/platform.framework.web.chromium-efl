// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IPC_CC_IPC_EXPORT_H_
#define CC_IPC_CC_IPC_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(CC_IPC_IMPLEMENTATION)
#define CC_IPC_EXPORT __declspec(dllexport)
#else
#define CC_IPC_EXPORT __declspec(dllimport)
#endif  // defined(CC_IPC_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(CC_IPC_IMPLEMENTATION)
#define CC_IPC_EXPORT __attribute__((visibility("default")))
#else
#define CC_IPC_EXPORT
#endif
#endif

#else  // defined(COMPONENT_BUILD)
#define CC_IPC_EXPORT
#endif

#endif  // CC_IPC_CC_IPC_EXPORT_H_
