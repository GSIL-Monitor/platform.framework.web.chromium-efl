// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_value_private.h"

EwkValuePrivate::EwkValuePrivate() = default;

EwkValuePrivate::EwkValuePrivate(bool value) : value_(new base::Value(value)) {}

EwkValuePrivate::EwkValuePrivate(int value) : value_(new base::Value(value)) {}

EwkValuePrivate::EwkValuePrivate(double value)
    : value_(new base::Value(value)) {}

EwkValuePrivate::EwkValuePrivate(const std::string& value)
    : value_(new base::Value(value)) {}

EwkValuePrivate::EwkValuePrivate(std::unique_ptr<base::Value> value)
    : value_(std::move(value)) {}

base::Value::Type EwkValuePrivate::GetType() const {
  if (!value_)
    return base::Value::Type::NONE;

  return value_->type();
}

base::Value* EwkValuePrivate::GetValue() const {
  return value_.get();
}

EwkValuePrivate::~EwkValuePrivate() = default;
