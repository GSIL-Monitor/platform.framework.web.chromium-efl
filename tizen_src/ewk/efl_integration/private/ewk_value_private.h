// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_PRIVATE_EWK_VALUE_PRIVATE_H_
#define EWK_EFL_INTEGRATION_PRIVATE_EWK_VALUE_PRIVATE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/values.h"

class EwkValuePrivate : public base::RefCountedThreadSafe<EwkValuePrivate> {
 public:
  EwkValuePrivate();
  explicit EwkValuePrivate(bool value);
  explicit EwkValuePrivate(int value);
  explicit EwkValuePrivate(double value);
  explicit EwkValuePrivate(const std::string& value);
  explicit EwkValuePrivate(std::unique_ptr<base::Value> value);

  // disable constructor from raw pointer because of implicit conversion
  // of pointer types to bool
  explicit EwkValuePrivate(base::Value* value) = delete;

  base::Value::Type GetType() const;
  base::Value* GetValue() const;

 private:
  friend class base::RefCountedThreadSafe<EwkValuePrivate>;
  ~EwkValuePrivate();

  std::unique_ptr<base::Value> value_;

  DISALLOW_COPY_AND_ASSIGN(EwkValuePrivate);
};

#endif  // EWK_EFL_INTEGRATION_PRIVATE_EWK_VALUE_PRIVATE_H_
