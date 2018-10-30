// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_MEDIA_CAPABILITY_MANAGER_IMPL_H_
#define MEDIA_BASE_TIZEN_MEDIA_CAPABILITY_MANAGER_IMPL_H_

#include <algorithm>

namespace media {
namespace detail {

struct CanAccomodate : platform::SlotsTypeEnum<CanAccomodate> {
  template <size_t Index>
  void visit(const platform::SlotsType& slots,
             const platform::DescriptorType& descr) {
    if (!descr.get<Index>())
      return;

    auto& type = slots.template get<Index>();
    if (type.used < type.players.size())
      return;

    result_ = false;
  }

  explicit operator bool() const { return result_; }

 private:
  bool result_ = true;
};

struct InsertPlayer : platform::SlotsTypeEnum<InsertPlayer> {
  bool result = true;
  template <size_t Index>
  void visit(platform::SlotsType& slots,
             const platform::DescriptorType& descr,
             MediaPlayerInterfaceEfl* player) {
    if (!descr.get<Index>())
      return;

    Insert(player, slots.template get<Index>());
  }

  template <typename SlotsType>
  static void Insert(MediaPlayerInterfaceEfl* player, SlotsType& type) {
    for (auto& player_ref : type.players) {
      if (player_ref)
        continue;
      player_ref = player;
      type.used++;
      break;
    }
  }
};

struct RemovePlayer : platform::SlotsTypeEnum<RemovePlayer> {
  template <size_t Index>
  void visit(platform::SlotsType& slots, MediaPlayerInterfaceEfl* player) {
    Remove(player, slots.template get<Index>());
  }

  template <typename SlotsType>
  static void Remove(MediaPlayerInterfaceEfl* player, SlotsType& type) {
    for (auto& player_ref : type.players) {
      if (player_ref != player)
        continue;
      player_ref = nullptr;
      type.used--;
      break;
    }
  }
};

struct UpdatePlayer : platform::SlotsTypeEnum<UpdatePlayer> {
  bool result = true;
  template <size_t Index>
  void visit(platform::SlotsType& slots,
             const platform::DescriptorType& descr,
             MediaPlayerInterfaceEfl* player) {
    auto& type = slots.template get<Index>();
    RemovePlayer::Remove(player, type);

    if (!descr.get<Index>())
      return;

    InsertPlayer::Insert(player, type);
  }
};
}  // namespace detail

template <typename ConflictStrategy>
bool MediaCapabilityManager<ConflictStrategy>::AddPlayer(
    MediaPlayerInterfaceEfl* player) {
  if (MediaCapabilityManagerBase::AddPlayer(player)) {
    ConflictStrategy::OnAdded(player);
    return true;
  }
  return false;
}

template <typename ConflictStrategy>
bool MediaCapabilityManager<ConflictStrategy>::MarkInactive(
    MediaPlayerInterfaceEfl* player) {
  if (MediaCapabilityManagerBase::MarkInactive(player)) {
    ConflictStrategy::OnDeactivated(player);
    return true;
  }
  return false;
}

template <typename ConflictStrategy>
bool MediaCapabilityManager<ConflictStrategy>::MarkHidden(
    MediaPlayerInterfaceEfl* player) {
  return MediaCapabilityManagerBase::MarkHidden(player);
}

template <typename ConflictStrategy>
bool MediaCapabilityManager<ConflictStrategy>::RemovePlayer(
    MediaPlayerInterfaceEfl* player) {
  if (MediaCapabilityManagerBase::RemovePlayer(player)) {
    ConflictStrategy::OnRemoved(player);
    return true;
  }
  return false;
}

template <typename ConflictStrategy>
bool MediaCapabilityManager<ConflictStrategy>::MarkActive(
    MediaPlayerInterfaceEfl* player,
    const SuspensionCallback& suspend) {
  // If in BARRED or AWAITING, MoveFromBarred<ACTIVE> will
  // optionally do the BARRED -> AWAITING transition and
  // will call OnActivated for strategy
  //
  // If in RUNNING or PREEMPTED, the MarkRunning loop will
  // be performed
  return MoveTowardsRunning<State::ACTIVE>(player, suspend);
}

template <typename ConflictStrategy>
bool MediaCapabilityManager<ConflictStrategy>::MarkVisible(
    MediaPlayerInterfaceEfl* player,
    const SuspensionCallback& suspend) {
  // If in BARRED or PREEMPTED, MoveFromBarred<VISIBLE>
  // will do the BARRED -> AWAITING transition.
  //
  // If in RUNNING or AWAITING, the MarkRunning loop will
  // be performed
  return MoveTowardsRunning<State::VISIBLE>(player, suspend);
}

template <typename ConflictStrategy>
template <State which_transition>
bool MediaCapabilityManager<ConflictStrategy>::MoveTowardsRunning(
    MediaPlayerInterfaceEfl* player,
    const SuspensionCallback& suspend) {
  if (MovedFromBarred<which_transition>(player)) {
    return false;
  }

  bool succeeded = true;
  std::queue<media::MediaPlayerInterfaceEfl*> players;
  players.push(player);
  while (!players.empty()) {
    auto front = players.front();
    players.pop();
    auto conflicts = MarkRunning(front);
    for (auto ptr : conflicts) {
      if (ptr == player)
        succeeded = false;
      if (!suspend(ptr))
        players.push(ptr);  // try re-insert it into RUNNING
      // it may be also needed to re-insert everything else in
      // ptr..end(players)
    }
  }

  return succeeded;
}

template <typename ConflictStrategy>
template <State which_transition>
bool MediaCapabilityManager<ConflictStrategy>::MovedFromBarred(
    MediaPlayerInterfaceEfl* player) {
  auto it = players_.find(player);
  if (it == players_.end())
    return true;  // NOOPs also reported as "don't continue"

  // here: RUNNING and the other one
  // of the single-bit |PlayerState|s
  if ((it->second.state & ~which_transition) != State::NONE)
    return false;

  // either BARRED -> which_transition, or
  // which_transition -> which_transition
  it->second.state |= which_transition;

  // Allow to react to Play/Pause/Seek while hidden
  if (it->second.state == PlayerState::AWAITING)
    ConflictStrategy::OnActivated(player);

  return true;
}

template <typename ConflictStrategy>
std::vector<MediaPlayerInterfaceEfl*>
MediaCapabilityManager<ConflictStrategy>::MarkRunning(
    MediaPlayerInterfaceEfl* player) {
  std::vector<MediaPlayerInterfaceEfl*> evicted;
  auto descriptor = platform::DefaultCapabilitiesSetup::Describe(*player);

  LOG(INFO) << __func__ << " player: " << player
            << " descriptor: " << descriptor << ConflictStrategy::Info();

  auto it = players_.find(player);
  if (it != players_.end()) {
    if (it->second.state == PlayerState::RUNNING) {
      auto outstanding = descriptor & ~it->second.descriptor;
      LOG(INFO) << player << ": Player RUNNING, (" << it->second.descriptor
                << ") -> (" << descriptor << ") = (" << outstanding << ")";
      if (!outstanding) {
        // either equal, or less capabilities set:
        if (descriptor != it->second.descriptor)
          detail::UpdatePlayer{}(slots_, descriptor, player);

        ConflictStrategy::OnActivated(player);
        return evicted;
      }

      // non-empty "outstanding" means there are
      // new capablities in the descriptor; remove
      // the player to free up slots for re-insert
      // and attempt to insert it again, with new
      // descriptor.
      detail::RemovePlayer{}(slots_, player);
    }
  }

  if (detail::CanAccomodate{}(slots_, descriptor)) {
    LOG(INFO) << "Can accomodate " << descriptor;
    RunPlayer(descriptor, player);
    return evicted;
  }

  auto removed = ConflictStrategy::Resolve(players_, slots_, descriptor);
  if (removed.empty()) {
    // empty "removed" means there were zeros and the player cannot
    // be activated in any way
    LOG(INFO) << "Cannot resolve for " << descriptor << ", player " << player
              << " looses";
    evicted.push_back(player);
    return evicted;
  }

  LOG(INFO) << "Resolved for " << descriptor << " with:";
  evicted.reserve(removed.size());
  for (auto& item : removed) {
    LOG(INFO) << "  " << item->first << " " << item->second.descriptor;

    evicted.push_back(item->first);
    item->second.state &= ~State::ACTIVE;
    detail::RemovePlayer{}(slots_, item->first);
    ConflictStrategy::OnDeactivated(item->first);
  }

  RunPlayer(descriptor, player);
  return evicted;
}

template <typename ConflictStrategy>
bool MediaCapabilityManager<ConflictStrategy>::RunPlayer(
    const platform::DescriptorType& descr,
    MediaPlayerInterfaceEfl* player) {
  if (MediaCapabilityManagerBase::RunPlayer(descr, player)) {
    ConflictStrategy::OnActivated(player);
    return true;
  }
  return false;
}

template <typename ConflictStrategy>
unsigned MediaCapabilityManager<ConflictStrategy>::PlayersCount(State state) {
  return std::count_if(begin(players_), end(players_),
                       [state](const PlayerList::value_type& val) {
                         return (val.second.state & state) == state;
                       });
}

}  // namespace media

#endif  // MEDIA_BASE_TIZEN_MEDIA_CAPABILITY_MANAGER_IMPL_H_
