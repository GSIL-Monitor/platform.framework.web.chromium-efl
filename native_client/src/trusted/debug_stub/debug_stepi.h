/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// This module provides methods used to perform single step debugging.

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DEBUG_STUB_DEBUG_STEPI_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DEBUG_STUB_DEBUG_STEPI_H_

#include "native_client/src/trusted/debug_stub/thread.h"

// Returns next instruction system address (PC) or 0 in case of errors.
// Used to handle gdb stepi command.
uint32_t NaClNextInstructionAddr(port::Thread* thread);

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DEBUG_STUB_DEBUG_STEPI_H_
