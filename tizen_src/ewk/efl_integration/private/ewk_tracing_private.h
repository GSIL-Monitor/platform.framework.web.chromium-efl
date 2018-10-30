// Copyright 2014 The Samsung Electronics Co., Ltd All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_tracing_private_h
#define ewk_tracing_private_h

bool ewk_start_tracing_private(const char* categories,
                               const char* trace_options,
                               const char* trace_file_name);
void ewk_stop_tracing_private();

#endif  // ewk_tracing_private_h
