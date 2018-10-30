// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_MEDIA_CAPABILITY_MANAGER_H_
#define MEDIA_BASE_TIZEN_MEDIA_CAPABILITY_MANAGER_H_

#include <algorithm>
#include <functional>
#include <queue>
#include <unordered_map>
#include <vector>
#include "media/base/tizen/media_capabilities.h"

namespace media {
class MediaCapabilityManagerBase {
 public:
  using SuspensionCallback = std::function<bool(MediaPlayerInterfaceEfl*)>;
  // start -> PREEMPTED
  bool AddPlayer(MediaPlayerInterfaceEfl* player);

  // RUNNING -> PREEMPTED
  // AWAITING -> BARRED
  bool MarkInactive(MediaPlayerInterfaceEfl* player);

  // RUNNING -> AWAITING
  // PREEMPTED -> BARRED
  bool MarkHidden(MediaPlayerInterfaceEfl* player);

  // any -> stop
  bool RemovePlayer(MediaPlayerInterfaceEfl* player);

  PlayerState GetState(MediaPlayerInterfaceEfl* player) const;

  template <MediaType type>
  detail::RangeView<MediaPlayerInterfaceEfl*> GetActive() const {
    return FindSlots<type>{}(slots_).result;
  }

 protected:
  using PlayerList =
      std::unordered_map<MediaPlayerInterfaceEfl*, platform::PlayerInfo>;

  platform::SlotsType slots_;
  PlayerList players_;

  bool RunPlayer(const platform::DescriptorType& descr,
                 MediaPlayerInterfaceEfl* player);

  // Will locate CapabilitySlots for given MediaType
  // and will returns its players as RangeView in result
  // member variable.
  template <MediaType media_type>
  struct FindSlots : platform::SlotsTypeEnum<FindSlots<media_type>> {
    detail::RangeView<MediaPlayerInterfaceEfl*> result;

    // Looks up one of tagged versions of GetActiveSlots,
    // depending whether or not the CapabilityType is a type
    // with proper kMediaType
    template <size_t Index, typename CapabilityType>
    void visitOne(const platform::SlotsType& slots) {
      using tag_type = typename MediaTypeTag<CapabilityType>::type;
      GetActiveSlots<Index>(slots, tag_type{});
    }

   private:
    // Implements the tagging for given CapabilityType
    // If the CapabilityType has a kMediaType of type
    // "media::MediaType" and its value is "media_type",
    // then the tag is true_type, with false_type otherwise
    template <class CapabilityType>
    struct MediaTypeTag {
      template <class T>
      using no_cvr = typename std::decay<T>::type;

      // This compares the decayed type of CapabilityType::kMediaType
      // ('const MediaType' turned into to 'MediaType') to the
      // enum type of MediaType
      using types_match =
          std::is_same<no_cvr<decltype(CapabilityType::kMediaType)>, MediaType>;

      using type = typename std::conditional<
          types_match::value && (CapabilityType::kMediaType == media_type),

          std::true_type,
          std::false_type>::type;
    };

    // false_type-tagged version of GetActiveSlots - does nothing
    template <size_t Index>
    void GetActiveSlots(const platform::SlotsType&, std::false_type) {}

    // true_type-tagged version of GetActiveSlots -
    // actually gets the RangeView of players.
    template <size_t Index>
    void GetActiveSlots(const platform::SlotsType& slots, std::true_type) {
      auto& data = slots.get<Index>();
      result = {data.players.data(), data.players.size()};
    }
  };
};

template <typename ConflictStrategy>
class MediaCapabilityManager : public MediaCapabilityManagerBase,
                               private ConflictStrategy {
 public:
  bool AddPlayer(MediaPlayerInterfaceEfl* player);
  bool MarkInactive(MediaPlayerInterfaceEfl* player);
  bool MarkHidden(MediaPlayerInterfaceEfl* player);
  bool RemovePlayer(MediaPlayerInterfaceEfl* player);

  // BARRED -> AWAITING
  // PREEMPTED -> RUNNING
  // Same as MoveTowardsRunning<ACTIVE>
  bool MarkActive(MediaPlayerInterfaceEfl* player,
                  const SuspensionCallback& cb);

  // BARRED -> PREEMPTED
  // AWAITING -> RUNNING
  // Same as MoveTowardsRunning<VISIBLE>
  bool MarkVisible(MediaPlayerInterfaceEfl* player,
                   const SuspensionCallback& cb);

  // Gets number of players that have all |flags| set.
  unsigned PlayersCount(State flags);

 private:
  // Will make one step along the
  // BARRED -VISIBLE-> PREEMPTED -ACTIVE-> RUNNINIG
  // or BARRED -ACTIVE-> AWAITING -VISIBLE-> RUNNINIG
  // lines, based on State given. For the first leg,
  // only the PlayerState and Strategy are updated
  // (via MoveFromBarred), for the second leg, the
  // full Accomodate/Resolve loop will be called (via
  // MarkRunning).
  template <State which_transition>
  bool MoveTowardsRunning(MediaPlayerInterfaceEfl* player,
                          const SuspensionCallback& suspend);

  // MovedFromBarred returns true, if either
  // NONE -> which_transition or single-bit
  // PlayerState -> itself transitions are
  // performed. This means any further
  // actions in MoveTowardsRunning (and outside)
  // should be throttled.
  template <State which_transition>
  bool MovedFromBarred(MediaPlayerInterfaceEfl* player);

  // RUNNING -> RUNNING
  // AWAITING -> RUNNING
  // PREEMPTED -> RUNNING
  //
  // "BARRED -> anything" transitions
  // must be handled before this function
  // is called. As a matter of fact, they
  // are done in MoveTowardsRunning via
  // MovedFromBarred
  std::vector<MediaPlayerInterfaceEfl*> MarkRunning(
      MediaPlayerInterfaceEfl* player);

  // Once the MarkRunning finds enough empty slots,
  // this function will propagate the slots with
  // the player and notify the strategy
  bool RunPlayer(const platform::DescriptorType& descr,
                 MediaPlayerInterfaceEfl* player);
};

namespace debug {
class print_capabilities {
 public:
  using PlayerList =
      std::unordered_map<MediaPlayerInterfaceEfl*, platform::PlayerInfo>;

  explicit print_capabilities(const PlayerList& players) : players(&players) {}

  print_capabilities(const char* message,
                     MediaPlayerInterfaceEfl* player,
                     const platform::DescriptorType& descr)
      : message(message), player(player), descriptor(&descr) {}

  print_capabilities(const char* message,
                     MediaPlayerInterfaceEfl* player,
                     const PlayerList& players)
      : message(message), player(player), players(&players) {}

  print_capabilities(const char* message,
                     MediaPlayerInterfaceEfl* player,
                     const PlayerList& players,
                     const platform::SlotsType& slots)
      : message(message), player(player), slots(&slots), players(&players) {}

  print_capabilities(const char* message,
                     MediaPlayerInterfaceEfl* player,
                     const platform::DescriptorType& descr,
                     const PlayerList& players,
                     const platform::SlotsType& slots)
      : message(message),
        player(player),
        descriptor(&descr),
        slots(&slots),
        players(&players) {}

  friend std::ostream& operator<<(std::ostream& o,
                                  const print_capabilities& info) {
    bool enter = false;
    if (info.player) {
      if (info.message) {
        o << info.message << ' ';
      }
      o << info.player;
      if (info.descriptor) {
        o << " (" << *info.descriptor << ')';
      }
      o << ":";
      enter = true;
    }

    if (info.slots) {
      if (enter)
        o << '\n';
      o << "  Slots:";
      SlotsEnum{}(o, *info.slots);
      enter = true;
    }

    if (info.players) {
      if (enter)
        o << '\n';
      o << "  Players:";
      for (auto pair : *info.players) {
        o << "\n    " << pair.first << " " << pair.second.state;
        if (pair.second.descriptor != platform::DescriptorType{})
          o << " (" << pair.second.descriptor << ")";
      }
    }
    return o;
  }

 private:
  const char* message = nullptr;
  MediaPlayerInterfaceEfl* player = nullptr;
  const platform::DescriptorType* descriptor = nullptr;
  const platform::SlotsType* slots = nullptr;
  const PlayerList* players = nullptr;

  struct SlotsEnum : platform::SlotsTypeEnum<SlotsEnum> {
    template <size_t Index, typename CapabilityType>
    void visitOne(std::ostream& o, const platform::SlotsType& slots) {
      o << "\n    " << CapabilityType{} << slots.get<Index>();
    }
  };
};
}  // namespace debug
}  // namespace media

// #define MCM_DEBUG

#ifdef MCM_DEBUG
#define MCM_LOGGING(LOGGER, ...) \
  LOGGER << ::media::debug::print_capabilities(__VA_ARGS__)
#else
#define MCM_LOGGING(...)
#endif

#define LOG_MCM(LEVEL, ...) MCM_LOGGING(LOG(LEVEL), __VA_ARGS__)

#include "media_capability_manager_impl.h"

#endif  // MEDIA_BASE_TIZEN_MEDIA_CAPABILITY_MANAGER_H_
