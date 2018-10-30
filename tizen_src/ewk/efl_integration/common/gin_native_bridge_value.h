// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_COMMON_GIN_NATIVE_BRIDGE_VALUE_H_
#define EWK_EFL_INTEGRATION_COMMON_GIN_NATIVE_BRIDGE_VALUE_H_

#include <stdint.h>

#include "base/pickle.h"
#include "base/values.h"
#include "content/common/content_export.h"

namespace content {

class GinNativeBridgeValue {  // LCOV_EXCL_LINE
 public:
  enum Type {
    TYPE_FIRST_VALUE = 0,
    // JavaScript 'undefined'
    TYPE_UNDEFINED = 0,
    // JavaScript NaN and Infinity
    TYPE_NONFINITE,
    // Bridge Object ID
    TYPE_OBJECT_ID,
    TYPE_LAST_VALUE
  };

  CONTENT_EXPORT static std::unique_ptr<base::Value> CreateObjectIDValue(
      int32_t in_value);

  // De-serialization
  CONTENT_EXPORT static bool ContainsGinJavaBridgeValue(
      const base::Value* value);
  CONTENT_EXPORT static std::unique_ptr<const GinNativeBridgeValue> FromValue(
      const base::Value* value);
  CONTENT_EXPORT Type GetType() const;
  CONTENT_EXPORT bool IsType(Type type) const;
  CONTENT_EXPORT bool GetAsNonFinite(float* out_value) const;
  CONTENT_EXPORT bool GetAsObjectID(int32_t* out_object_id) const;

 private:
  explicit GinNativeBridgeValue(Type type);
  explicit GinNativeBridgeValue(const base::Value* value);
  std::unique_ptr<base::Value> SerializeToValue();

  base::Pickle pickle_;

  DISALLOW_COPY_AND_ASSIGN(GinNativeBridgeValue);
};

}  // namespace content

#endif  // EWK_EFL_INTEGRATION_COMMON_GIN_NATIVE_BRIDGE_VALUE_H_
