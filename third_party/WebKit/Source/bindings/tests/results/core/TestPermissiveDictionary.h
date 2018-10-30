// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/WebKit/Source/bindings/templates/dictionary_impl.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#ifndef TestPermissiveDictionary_h
#define TestPermissiveDictionary_h

#include "bindings/core/v8/IDLDictionaryBase.h"
#include "core/CoreExport.h"
#include "platform/heap/Handle.h"

namespace blink {

class CORE_EXPORT TestPermissiveDictionary : public IDLDictionaryBase {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();
 public:
  TestPermissiveDictionary();
  virtual ~TestPermissiveDictionary();
  TestPermissiveDictionary(const TestPermissiveDictionary&);
  TestPermissiveDictionary& operator=(const TestPermissiveDictionary&);

  bool hasBooleanMember() const { return has_boolean_member_; }
  bool booleanMember() const {
    DCHECK(has_boolean_member_);
    return boolean_member_;
  }
  inline void setBooleanMember(bool);

  v8::Local<v8::Value> ToV8Impl(v8::Local<v8::Object>, v8::Isolate*) const override;
  DECLARE_VIRTUAL_TRACE();

 private:
  bool has_boolean_member_ = false;

  bool boolean_member_;

  friend class V8TestPermissiveDictionary;
};

void TestPermissiveDictionary::setBooleanMember(bool value) {
  boolean_member_ = value;
  has_boolean_member_ = true;
}

}  // namespace blink

#endif  // TestPermissiveDictionary_h
