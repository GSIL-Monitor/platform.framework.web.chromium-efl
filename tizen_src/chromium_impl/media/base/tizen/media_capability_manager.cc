// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/media_capability_manager.h"

namespace media {

bool MediaCapabilityManagerBase::AddPlayer(MediaPlayerInterfaceEfl* player) {
  players_[player];  // default-creates a PREEMPTED player with empty descriptor

  LOG_MCM(INFO, "Adding", player, players_);
  return true;
}

bool MediaCapabilityManagerBase::MarkInactive(MediaPlayerInterfaceEfl* player) {
  auto it = players_.find(player);
  if (it == players_.end())
    return false;

  it->second.state &= ~State::ACTIVE;
  detail::RemovePlayer{}(slots_, player);
  LOG_MCM(INFO, "Deactivating", player, players_, slots_);
  return true;
}

bool MediaCapabilityManagerBase::MarkHidden(MediaPlayerInterfaceEfl* player) {
  auto it = players_.find(player);
  if (it == players_.end())
    return false;

  it->second.state &= ~State::VISIBLE;
  detail::RemovePlayer{}(slots_, player);
  LOG_MCM(INFO, "Hiding", player, players_, slots_);
  return true;
}

bool MediaCapabilityManagerBase::RemovePlayer(MediaPlayerInterfaceEfl* player) {
  auto it = players_.find(player);
  if (it == players_.end())
    return false;

  detail::RemovePlayer{}(slots_, player);
  players_.erase(it);
  LOG_MCM(INFO, "Removing", player, players_, slots_);
  return true;
}

PlayerState MediaCapabilityManagerBase::GetState(
    MediaPlayerInterfaceEfl* player) const {
  auto it = players_.find(player);
  if (it == players_.end())
    return PlayerState::PREEMPTED;

  return it->second.state;
}

bool MediaCapabilityManagerBase::RunPlayer(
    const platform::DescriptorType& descr,
    MediaPlayerInterfaceEfl* player) {
  auto it = players_.find(player);
  if (it == players_.end())
    return false;  // No AddPlayer, or post RemovePlayer

  detail::InsertPlayer{}(slots_, descr, player);

  auto& key = it->second;
  bool was_not_awaiting = key.state != PlayerState::AWAITING;
  key.state = PlayerState::RUNNING;
  key.descriptor = descr;

  LOG_MCM(INFO, "Activating", player, descr, players_, slots_);
  // Startegy's OnActivated should be called on
  // PREEMPTED/RUNNING -> RUNNING, but not on
  // AWAITING -> RUNNING, to preserve previous
  // data strategy associated with the player,
  // when hiding
  return was_not_awaiting;
}
}  // namespace media
