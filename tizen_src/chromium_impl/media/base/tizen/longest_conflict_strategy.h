// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_TIZEN_LONGEST_CONFLICT_STRATEGY_H_
#define MEDIA_BASE_TIZEN_LONGEST_CONFLICT_STRATEGY_H_

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>
#include "media/base/tizen/media_capabilities.h"

namespace media {
class LongestConflictStrategy {
 public:
  using Clock = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;

  using SlotsType = platform::SlotsType;
  using DescriptorType = platform::DescriptorType;

  using PlayerList =
      std::unordered_map<MediaPlayerInterfaceEfl*, platform::PlayerInfo>;
  using Resolution = std::vector<PlayerList::iterator>;

  void OnAdded(MediaPlayerInterfaceEfl* player);
  void OnRemoved(MediaPlayerInterfaceEfl* player);
  void OnActivated(MediaPlayerInterfaceEfl* player);
  void OnDeactivated(MediaPlayerInterfaceEfl* player);
  Resolution Resolve(PlayerList& players,
                     const SlotsType& slots,
                     const DescriptorType& descr);

  struct ExtraInfo {
    MediaPlayerInterfaceEfl* player;
    Timepoint timepoint;
  };

  class Listing {
    const std::vector<ExtraInfo>& info_;

   public:
    Listing(const std::vector<ExtraInfo>& info) : info_{info} {}

    template <class CharT>
    friend std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& o,
                                                 const Listing& list) {
      using namespace std::chrono;
      o << "\n  Time points:";
      for (auto const& info : list.info_) {
        auto us =
            duration_cast<microseconds>(info.timepoint.time_since_epoch());
        auto s = duration_cast<seconds>(us);
        auto m = duration_cast<minutes>(s);
        auto h = duration_cast<hours>(m);

        us -= s;
        s -= m;
        m -= h;

        char buffer[100];
        sprintf(buffer, "%d:%02d:%02d.%06d", (int)h.count(), (int)m.count(),
                (int)s.count(), (int)us.count());
        o << "\n    " << info.player << ' ' << buffer;
      }
      return o;
    }
  };

  Listing Info() const { return {info_}; }

 private:
  std::vector<ExtraInfo> info_;

  std::vector<ExtraInfo>::iterator find(MediaPlayerInterfaceEfl* player) {
    return std::find_if(info_.begin(), info_.end(), [=](const ExtraInfo& info) {
      return info.player == player;
    });
  }
};
}  // namespace media

#endif  // MEDIA_BASE_TIZEN_LONGEST_CONFLICT_STRATEGY_H_
