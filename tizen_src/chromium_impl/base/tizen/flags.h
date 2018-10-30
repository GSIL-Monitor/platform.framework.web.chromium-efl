// Copyright 2018 the V8 project authors, 2018 Samsung Electronics Inc. All
// rights reserved. Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

#ifndef BASE_TIZEN_FLAGS_H_
#define BASE_TIZEN_FLAGS_H_

#include <cstddef>
#include <type_traits>

namespace base {

// The Flags class provides a type-safe way of storing OR-combinations of enum
// values. The Flags<T, S> class is a template class, where T is an enum type,
// and S is the underlying storage type (usually int).
//
// The traditional C++ approach for storing OR-combinations of enum values is to
// use an int or unsigned int variable. The inconvenience with this approach is
// that there's no type checking at all; any enum value can be OR'd with any
// other enum value and passed on to a function that takes an int or unsigned
// int.
template <typename T, typename S = typename std::underlying_type<T>::type>
class Flags final {
 public:
  typedef T flag_type;
  typedef S mask_type;

  Flags() : mask_(0) {}
  Flags(flag_type flag)  // NOLINT(runtime/explicit)
      : mask_(static_cast<S>(flag)) {}
  explicit Flags(mask_type mask) : mask_(mask) {}

  bool operator==(flag_type flag) const {
    return mask_ == static_cast<S>(flag);
  }
  bool operator!=(flag_type flag) const {
    return mask_ != static_cast<S>(flag);
  }

  Flags& operator&=(const Flags& flags) {
    mask_ &= flags.mask_;
    return *this;
  }
  Flags& operator|=(const Flags& flags) {
    mask_ |= flags.mask_;
    return *this;
  }
  Flags& operator^=(const Flags& flags) {
    mask_ ^= flags.mask_;
    return *this;
  }

  Flags operator&(const Flags& flags) const { return Flags(*this) &= flags; }
  Flags operator|(const Flags& flags) const { return Flags(*this) |= flags; }
  Flags operator^(const Flags& flags) const { return Flags(*this) ^= flags; }

  Flags& operator&=(flag_type flag) { return operator&=(Flags(flag)); }
  Flags& operator|=(flag_type flag) { return operator|=(Flags(flag)); }
  Flags& operator^=(flag_type flag) { return operator^=(Flags(flag)); }

  Flags operator&(flag_type flag) const { return operator&(Flags(flag)); }
  Flags operator|(flag_type flag) const { return operator|(Flags(flag)); }
  Flags operator^(flag_type flag) const { return operator^(Flags(flag)); }

  Flags operator~() const { return Flags(~mask_); }

  operator mask_type() const { return mask_; }
  bool operator!() const { return !mask_; }

  bool test(const Flags& flags) const { return operator&(flags) != Flags(); }
  Flags& set(const Flags& flags) { return operator|=(flags); }
  Flags& unset(const Flags& flags) { return operator&=(~flags); }

  bool test(flag_type flag) const { return test(Flags(flag)); }
  Flags& set(flag_type flag) { return set(Flags(flag)); }
  Flags& unset(flag_type flag) { return unset(Flags(flag)); }

  friend size_t hash_value(const Flags& flags) { return flags.mask_; }

 private:
  mask_type mask_;
};

#define DEFINE_OPERATORS_FOR_FLAGS(Type)                                      \
  inline Type operator&(                                                      \
      Type::flag_type lhs,                                                    \
      Type::flag_type rhs)ALLOW_UNUSED_TYPE WARN_UNUSED_RESULT;               \
  inline Type operator&(Type::flag_type lhs, Type::flag_type rhs) {           \
    return Type(lhs) & rhs;                                                   \
  }                                                                           \
  inline Type operator&(Type::flag_type lhs,                                  \
                        const Type& rhs)ALLOW_UNUSED_TYPE WARN_UNUSED_RESULT; \
  inline Type operator&(Type::flag_type lhs, const Type& rhs) {               \
    return rhs & lhs;                                                         \
  }                                                                           \
  inline void operator&(Type::flag_type lhs,                                  \
                        Type::mask_type rhs)ALLOW_UNUSED_TYPE;                \
  inline void operator&(Type::flag_type lhs, Type::mask_type rhs) {}          \
  inline Type operator|(Type::flag_type lhs, Type::flag_type rhs)             \
      ALLOW_UNUSED_TYPE WARN_UNUSED_RESULT;                                   \
  inline Type operator|(Type::flag_type lhs, Type::flag_type rhs) {           \
    return Type(lhs) | rhs;                                                   \
  }                                                                           \
  inline Type operator|(Type::flag_type lhs, const Type& rhs)                 \
      ALLOW_UNUSED_TYPE WARN_UNUSED_RESULT;                                   \
  inline Type operator|(Type::flag_type lhs, const Type& rhs) {               \
    return rhs | lhs;                                                         \
  }                                                                           \
  inline void operator|(Type::flag_type lhs, Type::mask_type rhs)             \
      ALLOW_UNUSED_TYPE;                                                      \
  inline void operator|(Type::flag_type lhs, Type::mask_type rhs) {}          \
  inline Type operator^(Type::flag_type lhs, Type::flag_type rhs)             \
      ALLOW_UNUSED_TYPE WARN_UNUSED_RESULT;                                   \
  inline Type operator^(Type::flag_type lhs, Type::flag_type rhs) {           \
    return Type(lhs) ^ rhs;                                                   \
  }                                                                           \
  inline Type operator^(Type::flag_type lhs, const Type& rhs)                 \
      ALLOW_UNUSED_TYPE WARN_UNUSED_RESULT;                                   \
  inline Type operator^(Type::flag_type lhs, const Type& rhs) {               \
    return rhs ^ lhs;                                                         \
  }                                                                           \
  inline void operator^(Type::flag_type lhs, Type::mask_type rhs)             \
      ALLOW_UNUSED_TYPE;                                                      \
  inline void operator^(Type::flag_type lhs, Type::mask_type rhs) {}          \
  inline Type operator~(Type::flag_type val)ALLOW_UNUSED_TYPE;                \
  inline Type operator~(Type::flag_type val) { return ~Type(val); }           \
  inline bool operator==(Type::flag_type lhs, const Type& rhs) {              \
    return rhs == lhs;                                                        \
  }                                                                           \
  inline bool operator!=(Type::flag_type lhs, const Type& rhs) {              \
    return rhs != lhs;                                                        \
  }

}  // namespace base

#endif  // BASE_TIZEN_FLAGS_H_
