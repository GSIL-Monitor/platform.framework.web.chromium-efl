/*
 * Copyright (c) 2017 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/debug_stub/debug_stepi.h"

uint32_t NaClNextInstructionAddr(port::Thread*) {
  return 0;
}
