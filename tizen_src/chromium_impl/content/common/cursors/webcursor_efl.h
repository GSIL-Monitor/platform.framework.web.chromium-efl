// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBCURSOR_EFL_H
#define WEBCURSOR_EFL_H

#if defined(USE_WAYLAND)
const char* GetCursorName(int type);
#else
int GetCursorType(int type);
#endif

#endif
