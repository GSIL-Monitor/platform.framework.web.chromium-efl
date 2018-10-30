// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk/efl_integration/common/gin_native_bridge_value.h"

namespace content {

namespace {
// The magic value is only used to prevent accidental attempts of reading
// GinJavaBridgeValue from a random BinaryValue.  GinJavaBridgeValue is not
// intended for scenarios where with BinaryValues are being used for anything
// else than holding GinJavaBridgeValues.  If a need for such scenario ever
// emerges, the best solution would be to extend GinJavaBridgeValue to be able
// to wrap raw BinaryValues.
const uint32_t kHeaderMagic = 0xBEEFCAFE;

#pragma pack(push, 4)
struct Header : public base::Pickle::Header {
  uint32_t magic;
  int32_t type;
};
#pragma pack(pop)
}  // namespace

// static
std::unique_ptr<base::Value>
GinNativeBridgeValue::CreateObjectIDValue(  // LCOV_EXCL_LINE
    int32_t in_value) {
  /* LCOV_EXCL_START */
  GinNativeBridgeValue gin_value(TYPE_OBJECT_ID);
  gin_value.pickle_.WriteInt(in_value);
  return gin_value.SerializeToValue();
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
// static
bool GinNativeBridgeValue::ContainsGinJavaBridgeValue(
    const base::Value* value) {
  if (!value->IsType(base::Value::Type::BINARY))
    return false;
  const base::Value* binary_value = reinterpret_cast<const base::Value*>(value);
  if (binary_value->GetBlob().size() < sizeof(Header))
    return false;
  base::Pickle pickle(binary_value->GetBlob().data(),
                      binary_value->GetBlob().size());
  // Broken binary value: payload or header size is wrong
  if (!pickle.data() || pickle.size() - pickle.payload_size() != sizeof(Header))
    return false;
  Header* header = pickle.headerT<Header>();
  return (header->magic == kHeaderMagic && header->type >= TYPE_FIRST_VALUE &&
          header->type < TYPE_LAST_VALUE);
}
/* LCOV_EXCL_STOP */

// static
std::unique_ptr<const GinNativeBridgeValue>
GinNativeBridgeValue::FromValue(  // LCOV_EXCL_LINE
    const base::Value* value) {
  return std::unique_ptr<const GinNativeBridgeValue>(
      value->IsType(base::Value::Type::BINARY)  // LCOV_EXCL_LINE
          ? new GinNativeBridgeValue(
                /* LCOV_EXCL_START */
                reinterpret_cast<const base::Value*>(value))
          : NULL);
  /* LCOV_EXCL_STOP */
}

GinNativeBridgeValue::Type GinNativeBridgeValue::GetType()
    const {                                          // LCOV_EXCL_LINE
  const Header* header = pickle_.headerT<Header>();  // LCOV_EXCL_LINE
  DCHECK(header->type >= TYPE_FIRST_VALUE && header->type < TYPE_LAST_VALUE);
  return static_cast<Type>(header->type);  // LCOV_EXCL_LINE
}

/* LCOV_EXCL_START */
bool GinNativeBridgeValue::IsType(Type type) const {
  return GetType() == type;
}

bool GinNativeBridgeValue::GetAsNonFinite(float* out_value) const {
  if (GetType() != TYPE_NONFINITE)
    return false;

  base::PickleIterator iter(pickle_);
  return iter.ReadFloat(out_value);
}

bool GinNativeBridgeValue::GetAsObjectID(int32_t* out_object_id) const {
  if (GetType() != TYPE_OBJECT_ID)
    return false;

  base::PickleIterator iter(pickle_);
  return iter.ReadInt(out_object_id);
}

GinNativeBridgeValue::GinNativeBridgeValue(Type type)
    : pickle_(sizeof(Header)) {
  Header* header = pickle_.headerT<Header>();
  header->magic = kHeaderMagic;
  header->type = type;
}

GinNativeBridgeValue::GinNativeBridgeValue(const base::Value* value)
    : pickle_(value->GetBlob().data(), value->GetBlob().size()) {
  /* LCOV_EXCL_STOP */
  DCHECK(ContainsGinJavaBridgeValue(value));
}  // LCOV_EXCL_LINE

std::unique_ptr<base::Value>
GinNativeBridgeValue::SerializeToValue() {  // LCOV_EXCL_LINE
  return base::Value::CreateWithCopiedBuffer(
      reinterpret_cast<const char*>(pickle_.data()),
      pickle_.size());  // LCOV_EXCL_LINE
}

}  // namespace content
