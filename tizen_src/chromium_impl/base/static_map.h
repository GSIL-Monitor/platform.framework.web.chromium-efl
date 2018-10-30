// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_STATIC_MAP_H
#define BASE_STATIC_MAP_H

#include <type_traits>
#include <unordered_map>

// |StaticMap| is a map which resemebles std::unordered_map (ie. has no element
// order) but can inline values for given keys for fast access. Because
// everything is calculated at compile time this has a drawback of using only
// types which match non-type template parameter for a key type.
// This can be especially useful for enumeration types where you know you will
// need to frequently access values for some predefined keys but need some sort
// of fallback in other cases.
//
// Template of declaration of |StaticMap| should contain |KeyType| and
// |ValueType| (duh) then a |StaticKeys| with template containing KeyType and
// then values of keys storage you want to inline, then optionally a
// |DynamicMapType| which will be used when key is not present in static part of
// the map, this defaults to std::unordered_map. For |DynamicMapType| you can
// also use |UseDefaultValue| which will return the same value (by const&) for
// any key. Example declarations:
//
// StaticMap<SomeEnumT, size_t, StaticKeys<SomeEnumT, SomeEnumT::Important,
//           SomeEnumT::EquallyImportant>>
// - this defines a map from SomeEnumT to size_t, with SomeEnumT::Important and
// SomeEnumT::EquallyImportant inlined. Both map values are default initialized
// and as a fallback std::unordered_map<SomeEnumT, size_t> is used.
// StaticMap<OtherEnumT, Clazz*, StaticKeys<OtherEnumT, OtherEnumT::Quick>,
//           UseDefaultValue<Clazz*>>
// - this map will map OtherEnumT to Clazz*, but only allows modification of
// OtherEnumT::Quick key. Other return nullptr.
//
// You can access elements of the map by using operator[], which will behave
// like normal operator[] when using std::unordered_map fallback; this means no
// const version. To access inlined elements in const context you can use
// get<key>(), which will check if key is indeed stored inlined.
// The fun thing is, with |UseDefaultValue| you get const version of operator[]
// so you can use it anywhere.
// To check whether element is present use count().
//
// TODO: iterators and other map interfaces.

namespace base {

// Scroll down for |StaticMap| implementation here are utilities

// |DoesTemplateContain| is a type trait (i.e. it defines static boolean member
// |value|), checking, if the first value template argument is also among the
// rest of the template parameter pack. This is used as a helper in
// |IsTemplateSet| type trait to guarantee the static map keys are unique.
template <class T, T... Tail>
struct DoesTemplateContain;

template <class T, T Value>
struct DoesTemplateContain<T, Value> : std::false_type {};

template <class T, T Value, T Head>
struct DoesTemplateContain<T, Value, Head>
    : std::integral_constant<bool, Value == Head> {};

template <class T, T Value, T Head, T... Tail>
struct DoesTemplateContain<T, Value, Head, Tail...>
    : std::integral_constant<
          bool,
          DoesTemplateContain<T, Value, Head>::value ||
              DoesTemplateContain<T, Value, Tail...>::value> {};

static_assert(!DoesTemplateContain<int, 1>::value, "Test failed");
static_assert(DoesTemplateContain<int, 1, 1>::value, "Test failed");
static_assert(!DoesTemplateContain<int, 2, 1>::value, "Test failed");
static_assert(DoesTemplateContain<int, 1, 1, 1, 1, 1, 1>::value, "Test failed");
static_assert(DoesTemplateContain<int, 1, 2, 2, 2, 2, 1>::value, "Test failed");

// |IsTemplateSet| is a type trait checking, if the value parameter pack set
// consists of unique values, that is, it can be used as a set of unique keys in
// static map. As such, it is used in |StaticKeys| class.
template <class T, T... Tail>
struct IsTemplateSet;

template <class T>
struct IsTemplateSet<T> : std::true_type {};

template <class T, T Head, T... Tail>
struct IsTemplateSet<T, Head, Tail...>
    : std::integral_constant<bool,
                             !DoesTemplateContain<T, Head, Tail...>::value &&
                                 IsTemplateSet<T, Tail...>::value> {};

static_assert(IsTemplateSet<int>::value, "Test failed");
static_assert(IsTemplateSet<int, 2>::value, "Test failed");
static_assert(IsTemplateSet<int, 2, 1>::value, "Test failed");
static_assert(!IsTemplateSet<int, 1, 1>::value, "Test failed");
static_assert(!IsTemplateSet<int, 1, 1, 1, 1, 1, 1>::value, "Test failed");
static_assert(IsTemplateSet<int, 1, 2, 3, 4, 5, 6>::value, "Test failed");
static_assert(!IsTemplateSet<int, 1, 2, 3, 4, 5, 1>::value, "Test failed");

// Here is |StaticMap| implementation

// |StaticKeys| are used for defining key values which storage should be
// inlined. |StaticKeys| checks whether provided keys are not duplicated.
template <class T, T... Values>
struct StaticKeys {
  static_assert(IsTemplateSet<T, Values...>::value, "Keys are duplicated!");
  using KeyType = T;
};

template <class T>
struct StaticKeys<T> {
  // TODO: Make this fail, why use static map without static values...
  //       Right now it will fail in DynamicMap specialization.
  //   static_assert(!std::is_same<T,T>::value, "Static keys are empty");
};

template <class T>
struct UseDefaultValue {
  constexpr UseDefaultValue() : value() {}

  template <class... Args>
  explicit UseDefaultValue(Args&&... args)
      : value(std::forward<Args>(args)...) {}

  template <class KeyType>
  const T& operator[](KeyType) const {
    return value;
  }

 private:
  const T value;
};

template <class K, class V>
using DefaultDynamicMap = std::unordered_map<K, V>;

// This is only to fail if StaticKeys were not used
template <class KeyType,
          class ValueType,
          class StaticKeysT,
          class DynamicMapType = DefaultDynamicMap<KeyType, ValueType>,
          KeyType... keys>
class StaticMap {
  static_assert(std::is_same<StaticKeysT, StaticKeys<KeyType, keys...>>::value,
                "You didn't use StaticKeys to pass compile time keys or the "
                "type of it's values does not match KeyType");
};

// |DynamicMapType| version which uses this type for a fallback in case when key
// is not inlined
template <class KeyType, class ValueType, class DynamicMapType>
class StaticMap<KeyType, ValueType, StaticKeys<KeyType>, DynamicMapType> {
 public:
  constexpr StaticMap() = default;
  StaticMap(const StaticMap&) = default;
  StaticMap(StaticMap&&) = default;
  ~StaticMap() = default;

  StaticMap& operator=(const StaticMap&) = default;
  StaticMap& operator=(StaticMap&&) = default;

  template <class... Args>
  explicit StaticMap(Args&&... rest) : map_(std::forward<Args>(rest)...) {}

  // This will should always fail, because DynamicMap won't provide keys at
  // compile time
  template <KeyType V>
  ValueType& get() {
    static_assert(!std::is_same<KeyType, decltype(V)>::value,
                  "There is no such compile time Key!");
  }

  template <KeyType V>
  constexpr const ValueType& get() const {
    static_assert(!std::is_same<KeyType, decltype(V)>::value,
                  "There is no such compile time Key!");
  }

  // Below functions will only exist if map_ defines matching operator[]
  // This ugly monster is used to disable it for const DynamicMapType when this
  // is not const
  template <class U = DynamicMapType>
  auto operator[](KeyType key) -> typename std::enable_if<
      std::is_same<decltype(std::declval<U>()[key]), ValueType&>::value,
      ValueType&>::type {
    return map_[key];
  }

  template <class U = DynamicMapType>
  auto operator[](KeyType key) const -> decltype(std::declval<const U>()[key]) {
    static_assert(std::is_same<decltype(std::declval<const U>()[key]),
                               const ValueType&>::value,
                  "DynamicMapType::operator[] must return ValueType& or const "
                  "ValueType&");
    return map_[key];
  }

 private:
  DynamicMapType map_;
};

// Contains storage for given inlined key value and otherwise delegates to map_,
// which could be again this or |DynamicMapType| version.
template <class KeyType,
          class ValueType,
          class DynamicMapType,
          KeyType key_value_,
          KeyType... rest_of_keys_>
class StaticMap<KeyType,
                ValueType,
                StaticKeys<KeyType, key_value_, rest_of_keys_...>,
                DynamicMapType> {
  // Make sure keys set is ok
  static const StaticKeys<KeyType, key_value_, rest_of_keys_...> static_keys;

  using InnerMapType = StaticMap<KeyType,
                                 ValueType,
                                 StaticKeys<KeyType, rest_of_keys_...>,
                                 DynamicMapType>;

 public:
  using size_type = size_t;

  constexpr StaticMap() : value_() {}
  StaticMap(const StaticMap&) = default;
  StaticMap(StaticMap&&) = default;
  ~StaticMap() = default;

  StaticMap& operator=(const StaticMap&) = default;
  StaticMap& operator=(StaticMap&&) = default;

  template <class T, class... Args>
  explicit StaticMap(T&& arg, Args&&... rest)
      : value_(std::forward<T>(arg)), map_(std::forward<Args>(rest)...) {}

  // Compile time conditional on key value
  template <KeyType V>
  auto get() -> typename std::enable_if<V == key_value_, ValueType&>::type {
    return value_;
  }

  template <KeyType V>
  auto get() -> typename std::enable_if<V != key_value_, ValueType&>::type {
    return map_.get<V>();
  }

  template <KeyType V>
  constexpr auto get() const ->
      typename std::enable_if<V == key_value_, const ValueType&>::type {
    return value_;
  }

  template <KeyType V>
  constexpr auto get() const ->
      typename std::enable_if<V != key_value_, const ValueType&>::type {
    return map_.get<V>();
  }

  // Below functions will only exist if map_ defines matching operator[]
  template <class U = InnerMapType>
  auto operator[](KeyType key) -> decltype(std::declval<U>()[key]) {
    if (key == key_value_)
      return value_;
    return map_[key];
  }

  template <class U = InnerMapType>
  auto operator[](KeyType key) const -> decltype(std::declval<const U>()[key]) {
    if (key == key_value_)
      return value_;
    return map_[key];
  }

  // This is to mimic std::unordered_map and std::map interfaces
  size_type count(KeyType key) const {
    return key == key_value_ ? 1 : map_.count();
  }

 private:
  ValueType value_;
  InnerMapType map_;
};

}  // namespace base

#endif  // BASE_STATIC_MAP_H
