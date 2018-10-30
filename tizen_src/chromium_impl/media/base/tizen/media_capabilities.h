// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_MEDIA_CAPABILITIES_H_
#define MEDIA_BASE_TIZEN_MEDIA_CAPABILITIES_H_

#include "base/tizen/sequence.h"
#include "media/base/tizen/media_player_interface_efl.h"

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>

using std::size_t;

namespace media {

// =====================================================================
// Player states and stages?

enum class State {
  NONE = 0,
  ACTIVE = 1,
  VISIBLE = 2,
};

inline constexpr State operator~(State domain) {
  return State(~(int)domain);
}
inline constexpr State operator|(State l, State r) {
  return State((int)l | (int)r);
}

enum class PlayerState {
  BARRED = int(State::NONE),
  AWAITING = int(State::ACTIVE),
  PREEMPTED = int(State::VISIBLE),
  RUNNING = int(State::ACTIVE | State::VISIBLE),
};

inline PlayerState& operator|=(PlayerState& l, State r) {
  (int&)l |= (int)r;
  return l;
}

inline PlayerState& operator&=(PlayerState& l, State r) {
  (int&)l &= (int)r;
  return l;
}

inline constexpr State operator&(PlayerState l, State r) {
  return State((int)l & (int)r);
}

inline constexpr PlayerState operator&(PlayerState l, PlayerState r) {
  return PlayerState((int)l & (int)r);
}

namespace detail {

// =====================================================================
// Integer type selector for Descriptor's storage. From given type list,
// it will choose the smallest integer, still being able to fit all the
// bits. For programs with Descriptor defined over more Capabilites, than
// bits in long long are ill-formed (the Select will no compile).

template <size_t Size, class Head, class... Tail>
struct Select {
  // Assuming char contains 8 bits, this conditional will
  // either select current Head (if the Head's size would
  // accomodate needed amount of bits), or will repeat
  // the process with the remainding tail.
  using type =
      typename std::conditional<(Size <= (sizeof(Head) * 8)),
                                Head,
                                typename Select<Size, Tail...>::type>::type;
};

// Halting condition
template <size_t Size, class Head>
struct Select<Size, Head> {
  using type =
      typename std::enable_if<(Size <= (sizeof(Head) * 8)), Head>::type;
};

template <size_t Bits>
struct SmallestInt {
  using type =
      typename Select<Bits, unsigned int, unsigned long, unsigned long long>::
          type;
};

// =====================================================================
// Machinery for generating slot list for every capability in the
// descriptor. Each capability will have a CapabilitySlots, consisting of
// used-slots counter and a list of player links, generated from definition
// of the capability.

template <size_t N>
struct CapabilitySlots {
  size_t used{0};
  std::array<MediaPlayerInterfaceEfl*, N> players;

  friend std::ostream& operator<<(std::ostream& o,
                                  const CapabilitySlots& slots) {
    o << ':' << slots.used << ":[";
    bool first = true;
    for (auto player : slots.players) {
      if (!player)
        continue;

      if (first)
        first = false;
      else
        o << ", ";
      o << player;
    }
    return o << ']';
  }
};

// Helper automaton which takes a Rebinder callback and a tuple
// and produces tuple, where each type was bound to another type
// by the Rebinder callback.
template <typename Rebinder, typename Tuple>
struct RebindTuple;

// (works only with tuples)
template <typename Rebinder, typename... TupleType>
struct RebindTuple<Rebinder, std::tuple<TupleType...>> {
  template <class T>
  using rebind = typename Rebinder::template Rebind<T>::type;
  using type = std::tuple<rebind<TupleType>...>;
};

// Rebinder for SlotsType - takes the kMax constant from each
// capability definition and uses it to create CapabilitySlots
// for that capability
struct MakeCapabilitySlotTuple {
  template <typename T>
  struct Rebind {
    using type = CapabilitySlots<T::kMax>;
  };
};

template <typename DescriptorTypeT>
struct SlotsType {
  using DescriptorType = DescriptorTypeT;
  using Storage = typename DescriptorType::Storage;
  // DataTuple is a tuple analog for the descriptor's
  // TypeTuple, a tuple of capability types
  using DataTuple =
      typename RebindTuple<MakeCapabilitySlotTuple,
                           typename DescriptorType::TypeTuple>::type;

  // Storage for the slots
  DataTuple slots;

  // getter for the TypesEnum-like expansions
  template <size_t Index>
  typename std::tuple_element<Index, DataTuple>::type& get() {
    return std::get<Index>(slots);
  }

  template <size_t Index>
  typename std::tuple_element<Index, const DataTuple>::type& get() const {
    return std::get<Index>(slots);
  }
};

// =====================================================================
// DescriptorType represents an opaque data for capabilities required.
//
// Built autmagically at compile time - takes the packed set of capability
// types and turns it into storage, builds SlotsType out of them

template <typename... CapabilityTypes>
class DescriptorType {
 public:
  constexpr DescriptorType() = default;

  using TypeTuple = std::tuple<CapabilityTypes...>;
  using Storage = typename SmallestInt<sizeof...(CapabilityTypes)>::type;
  using SlotsType = detail::SlotsType<DescriptorType>;

  template <typename Final>
  struct SlotsTypeEnum : base::TypesEnum<Final, CapabilityTypes...> {
    template <size_t Index, typename, typename... Args>
    void visitOne(Args&&... args) {
      auto that = static_cast<Final*>(this);
      that->template visit<Index>(std::forward<Args>(args)...);
    }
    template <size_t Index, typename... Args>
    void visit(Args&&... args) {}
  };

  // A 'one', that - for bit operations - is always casted to the right type
  static constexpr Storage kOne = std::integral_constant<Storage, 1>::value;

  // setter for the TypesEnum-like expansions; will set or clear a bit
  // depending on whether or not the capability is requested.
  template <size_t Index>
  void set(bool capability_required) {
    Storage qic[] = {storage_ & ~(kOne << Index), storage_ | (kOne << Index)};
    storage_ = qic[!!capability_required];
  }

  // getter for the TypesEnum-like expansions; returns true for
  // requested capabilities and false otherwise
  template <size_t Index>
  bool get() const {
    return !!(storage_ & (kOne << Index));
  }

  // visitOne helper for the TypesEnum-like expansions; takes a reference
  // to the player and uses the concrete capability type to
  template <size_t Index>
  void check(const MediaPlayerInterfaceEfl& player) {
    using ConcreteType =
        typename base::NthType<Index, CapabilityTypes...>::type;
    auto capability_required = ConcreteType::Check(player);
    set<Index>(capability_required);
  }

  // The strength is the number of capabilites set in this secriptor
  int strength() const { return popcount(storage_); }

  explicit operator bool() const { return !!storage_; }

  bool operator==(const DescriptorType& oth) const {
    return storage_ == oth.storage_;
  }

  bool operator!=(const DescriptorType& oth) const {
    return storage_ != oth.storage_;
  }

  DescriptorType operator&(const DescriptorType& oth) const {
    return {storage_ & oth.storage_};
  }

  DescriptorType operator~() const { return {~storage_}; }

  friend std::ostream& operator<<(std::ostream& o,
                                  const DescriptorType& descr) {
    StreamEnum{}(o, descr);
    return o;
  }

 private:
  constexpr DescriptorType(Storage stg) : storage_{stg} {};
  Storage storage_{0};

  // src: https://en.wikipedia.org/wiki/Hamming_weight
  static int popcount(uint64_t x) {
    constexpr uint64_t m1 = 0x5555555555555555;  // binary: 0101...
    constexpr uint64_t m2 = 0x3333333333333333;  // binary: 00110011..
    constexpr uint64_t m4 =
        0x0f0f0f0f0f0f0f0f;  // binary:  4 zeros,  4 ones ...

    x -= (x >> 1) & m1;  // put count of each 2 bits into those 2 bits
    x = (x & m2) + ((x >> 2) & m2);  // put count of each 4 bits into those 4
                                     // bits
    x = (x + (x >> 4)) & m4;  // put count of each 8 bits into those 8 bits
    x += x >> 8;   // put count of each 16 bits into their lowest 8 bits
    x += x >> 16;  // put count of each 32 bits into their lowest 8 bits
    x += x >> 32;  // put count of each 64 bits into their lowest 8 bits
    return x & 0x7f;
  }

  struct StreamEnum : SlotsTypeEnum<StreamEnum> {
    template <size_t Index, typename CapabilityType>
    void visitOne(std::ostream& o, const DescriptorType& descr) {
      if (!descr.get<Index>())
        return;
      o << CapabilityType{};
    }
  };
};

// =====================================================================
// Helper for MediaCapabilityManager::GetActive: a read-only view of
// slots for a given media type
template <typename T>
class RangeView {
  const T* ptr_{nullptr};
  size_t size_{0};

 public:
  RangeView() = default;
  RangeView(const T* ptr, size_t size) : ptr_{ptr}, size_{size} {}

  const T* begin() const { return ptr_; }
  const T* end() const { return ptr_ + size_; }
  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }
};

}  // namespace detail

// =====================================================================
// CapabilitiesSetup definitions:
//
// Each capability type needs to have kMax constant representing amount
// of slots on the platform, and a static Check method, taking
// const player reference and returning a boolean. True should be
// used, if the capability type decides the player requires this
// capability, false otherwise

template <class... CapabilityTypeT>
struct CapabilitiesSetup {
  using DescriptorType = detail::DescriptorType<CapabilityTypeT...>;
  using SlotsType = typename DescriptorType::SlotsType;

  // Returns a descriptor with all capabilites required
  // from the player are set
  static DescriptorType Describe(const MediaPlayerInterfaceEfl& player) {
    DescriptorType out;
    DescribePlayer{}(player.GetMediaType()
                         ? player.GetMediaType()
                         : MediaType::Video | MediaType::Audio,
                     out);
    return out;
  }

 private:
  class DescribePlayer
      : public base::TypesEnum<DescribePlayer, CapabilityTypeT...> {
   public:
    template <size_t Index, typename ConcreteType>
    void visitOne(MediaTypeFlags flags, DescriptorType& descriptor) {
      auto needs = ConcreteType::Check(flags);
      descriptor.template set<Index>(needs);
    }
  };
};

// Helper providing the kMax contant
template <size_t Count>
struct CapabilityTypeBase {
  static constexpr const size_t kMax = Count;
  static bool Check(MediaTypeFlags needed) { return false; }  // overridable
};

// =====================================================================
// CapabilityType definitions for CAPI player:

template <MediaType type, size_t Count>
struct CapabilityType : CapabilityTypeBase<Count> {
  static constexpr const MediaType kMediaType = type;
  static constexpr void Modify(MediaTypeFlags& needed) { needed |= type; }
  static constexpr bool Check(MediaTypeFlags needed) { return needed & type; }

  friend std::ostream& operator<<(std::ostream& o, const CapabilityType&) {
    return o << kMediaType;
  }
};

template <size_t AudioCount, size_t VideoCount>
using BasicAVCapabilitiesSetup =
    CapabilitiesSetup<CapabilityType<MediaType::Audio, AudioCount>,
                      CapabilityType<MediaType::Video, VideoCount>>;

namespace platform {

// Note : In case of Mobile , maximum decoder instance count is depending on
// contents resolution but to get the resolution of content, player should be
// prepared. Hence maximum instance count is set as 1.
// Resolution <= 720P : 2
// Resolution >  720P : 1
// Incase of TV : audio & video using H/W decoder , so only support 1.
using MobileCapabilitiesSetup = BasicAVCapabilitiesSetup<20, 1>;
using WearableCapabilitiesSetup =
    MobileCapabilitiesSetup;  // synchronized alias
using TVCapabilitiesSetup = BasicAVCapabilitiesSetup<1, 1>;
using LFDCapabilitiesSetup = BasicAVCapabilitiesSetup<3, 3>;

// TODO: Select with #if tests...
#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
using DefaultCapabilitiesSetup = LFDCapabilitiesSetup;
#else
using DefaultCapabilitiesSetup = TVCapabilitiesSetup;
#endif

using DescriptorType = DefaultCapabilitiesSetup::DescriptorType;
using SlotsType = DescriptorType::SlotsType;

// Shorthand
template <class Final>
using SlotsTypeEnum = DescriptorType::SlotsTypeEnum<Final>;

struct PlayerInfo {
  PlayerState state{PlayerState::PREEMPTED};
  DescriptorType descriptor;
};
}  // namespace platform
}  // namespace media

namespace std {
inline ostream& operator<<(ostream& o, media::PlayerState state) {
  switch (state) {
    case media::PlayerState::BARRED:
      return o << "BARRED";
    case media::PlayerState::AWAITING:
      return o << "AWAITING";
    case media::PlayerState::PREEMPTED:
      return o << "PREEMPTED";
    case media::PlayerState::RUNNING:
      return o << "RUNNING";
    default:
      break;
  };
  return o << "PlayerState(" << (int)state << ")";
}
}  // namespace std

#endif  // MEDIA_BASE_TIZEN_MEDIA_CAPABILITIES_H_
