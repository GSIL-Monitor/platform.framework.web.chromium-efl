// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TIZEN_SEQUENCE_H_
#define BASE_TIZEN_SEQUENCE_H_

#include <tuple>
#include "base/compiler_specific.h"

using std::size_t;

namespace base {
// =====================================================================
// NthType
// Type trait selecting Nth type in the list

template <size_t Index, typename Head, typename... Types>
struct NthType {
  using type = typename NthType<Index - 1, Types...>::type;
};

// Type trait's halting condition
template <typename Head, typename... Types>
struct NthType<0, Head, Types...> {
  using type = Head;
};

// =====================================================================
// C++14's index_sequence and make_index_sequence:
//   For given Size, BuildSequence<Size> will
//   become a Sequence<0, 1, ... Size - 1>, that is,
//   BuildSequence<5> will produce Sequence<0, 1, 2, 3, 4>
//   This is useful to iterate over tuples, type packs
//   (with NthType) and over DescriptorType's getters
//   and setters.

template <size_t... Index>
struct Sequence {};
template <size_t Size, size_t... Index>
struct SequenceBuilder {
  using type = typename SequenceBuilder<Size - 1, Size - 1, Index...>::type;
};
template <size_t... Index>
struct SequenceBuilder<0, Index...> {
  using type = Sequence<Index...>;
  // For non-zero Size, go one level deeper,
  // but prepend the next Size to the answer.
  // Zero Size is the halting condition; whatever
  // is in the Index pack at this point, is the
  // 0..Size-1 sequence:
  //     BuildSequence<5> is a
  //     SequenceBuilder<5 | >::type, which is a
  //     SequenceBuilder<4 | 4>::type, which is a
  //     SequenceBuilder<3 | 3, 4>::type, which is a
  //     SequenceBuilder<2 | 2, 3, 4>::type, which is a
  //     SequenceBuilder<1 | 1, 2, 3, 4>::type, which is a
  //     SequenceBuilder<0 | 0, 1, 2, 3, 4>::type, which is a
  //     Sequence<0, 1, 2, 3, 4>
};

// Friendly name for the sequence being built
template <size_t Size>
using BuildSequence = typename SequenceBuilder<Size>::type;

// =====================================================================
// TypesEnum helper class.
//
// For any use of Types pack one needs to provide
// a function with Sequence, a function extracting
// the NthType and extrating or setting nth data piece
//
// This class is a CRTP helper, providing all the machinery
// and asking its Final descendant to react only on a single
// index and a single type, both within the Types pack.
//
// The static polymorphism is done through visitOne method.
// Users of this class need to create a method template
//
//   template <size_t Index, typename Visitor>
//   void visitOne(list-of-parameters)
//
// and start the enumeration with
//
//   MyEnum myenum; myenum(args-for-parameters);
//
// (or quicker 'MyEnum{}(args-for-parameters)')

template <class Final, class... Types>
struct TypesEnum {
  template <typename... Args>
  Final& operator()(Args&&... args) {
    // Only one packed set is allowed; moving Args... to type definition,
    // so it does not clash with Index... from Sequence
    using Helper = EnumHelper<Args...>;
    Helper::visitAll(this, std::forward<Args>(args)...,
                     BuildSequence<sizeof...(Types)>{});

    // return ref-to-self for possible call-chaining
    return *static_cast<Final*>(this);
  }

  template <size_t Index, typename Visitor, typename... Args>
  void visitOne(Args&&...) {}

 protected:
  template <typename... Args>
  struct EnumHelper {
    template <size_t Index>
    static void visitSingle(TypesEnum* thiz, Args&&... args) {
      using ConcreteType = typename NthType<Index, Types...>::type;
      auto that = static_cast<Final*>(thiz);  // CRTP in action...
      that->template visitOne<Index, ConcreteType>(std::forward<Args>(args)...);
    }

    template <size_t... Index>
    static void visitAll(TypesEnum* thiz, Args&&... args, Sequence<Index...>) {
      // Once we have an expandable sequence of Indexes,
      // we could use it to generate all the visitSingle()s:
      //
      //    visitSingle<Index>(thiz, std::forward<Args>(args)...)...;
      //
      // A direct expansion might be possible in C++17, but in C++11
      // it can be only created as comma-expansion inside a call or
      // brace-init.
      //
      // ? ignore[] = {
      //     visitSingle<Index>(thiz, std::forward<Args>(args)...)...
      // };
      //
      // Problem with this approach is, the visitSingle is a void-
      // returning function. C++ does not allow brace-initialization
      // of a bunch of voids. For that we can use a comma operator,
      // which means "evaluate all the expressions in order, use last
      // one as my value":
      //
      //  (visitSingle<Index>(thiz, std::forward<Args>(args)...), 0)
      //
      // Now we can use 'int' as type of 'ignore' elements and expand
      // that expression as a means to expand visitSingle over Index...
      //
      // If the compiler ever supports C++17, then the next two
      // lines should be replaced with comma-expansion:
      //   (visitSingle<Index>(thiz, std::forward<Args>(args)...), ...);
      //   ^                                                     ^ ~~~^
      int ignore[] = {
          (visitSingle<Index>(thiz, std::forward<Args>(args)...), 0)...};
      ALLOW_UNUSED_LOCAL(ignore);
    }
  };
};

// Simplified inheritance. For Final classes, which in turn are
// also templates, the TypesEnum is used as such:
//
//   template <class Extra1, class Extra2, class... Types>
//   struct TheFinalClass : TypesEnum<TheFinalClass<Extra1, Extra2, Types...>,
//   Types...> {
//        template <size_t Index, typename Visitor...
//        void visitOne(....
//   };
//
// which is rather mouthfull. This alias simplifies this for
// template Final classes, which has no extra template params.
// With this alias, the example above becomes (remember, this
// only works _without_ any extra params):
//
//   template <class... Types>
//   struct TheFinalClass : SimpleTypesEnum<TheFinalClass, Types...> {
//        template <size_t Index, typename Visitor...
//        void visitOne(....
//   };
//
template <template <class...> class Final, class... Types>
using SimpleTypesEnum = TypesEnum<Final<Types...>, Types...>;
}  // namespace base

#endif  // BASE_TIZEN_SEQUENCE_H_
