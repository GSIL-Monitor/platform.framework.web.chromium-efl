// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_VAR_VALUE_CONVERTER_H_
#define PPAPI_PROXY_VAR_VALUE_CONVERTER_H_

#include "base/values.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/shared_impl/scoped_pp_var.h"

namespace ppapi {
namespace proxy {

// TODO(m.majczak) Implement cycle detection in vars and values.
// Currentely value/var with cycle will result in an application crash.

/**
 * Converts PP_Var to base::Value, passes the ownership of the created value.
 *
 * @param var Reference to an object of type PP_Var that will be converted to
 * base::Value. The ownership of the var is not passed.
 *
 * @return scoped_ptr pointing to newly created base::Value.
 */
std::unique_ptr<base::Value> ValueFromVar(const PP_Var& var);

/**
 * Converts base::Value to PP_Var.
 *
 * @param value Pointer to an object of type base::Value that will be converted
 * to PP_Var. The ownership of the value is not passed.
 *
 * @return ScopedPPVar that was creted from the value
 */
ScopedPPVar VarFromValue(const base::Value* value);

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_VAR_VALUE_CONVERTER_H_
