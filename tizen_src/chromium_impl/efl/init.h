// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _EFL_PORT_IMPL_INIT_H_
#define _EFL_PORT_IMPL_INIT_H_

#include "base/command_line.h"
#include "efl/efl_export.h"

namespace efl {

EFL_EXPORT int Initialize(int argc, const char* argv[]);

EFL_EXPORT void AppendPortParams(base::CommandLine&);

} // namespace efl

#endif // _EFL_PORT_IMPL_INIT_H_
