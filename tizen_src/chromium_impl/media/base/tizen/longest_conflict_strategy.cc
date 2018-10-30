// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/longest_conflict_strategy.h"

namespace media {
namespace detail {

struct RemoveIfAvailable : platform::SlotsTypeEnum<RemoveIfAvailable> {
  template <size_t Index>
  void visit(const platform::SlotsType& slots,
             platform::DescriptorType& descr) {
    if (!descr.get<Index>())
      return;

    auto& type = slots.template get<Index>();
    if (type.used < type.players.size()) {
      descr.set<Index>(false);
    }
  }
};

struct LCSTypedefs {
  // Candidate consists of an iterator into player list,
  // a timepoint of last Play/Pause/Suspend,
  // and a descriptor
  using PlayerList =
      std::unordered_map<MediaPlayerInterfaceEfl*, platform::PlayerInfo>;
  using PlayerIterator = PlayerList::iterator;
  struct Candidate {
    PlayerIterator iter;
    LongestConflictStrategy::Timepoint timepoint;
    platform::DescriptorType descriptor;
  };
  using Candidates = std::vector<Candidate>;

  enum : size_t {
    TypesSize = std::tuple_size<platform::SlotsType::DataTuple>::value
  };
  using Mask = std::array<int, TypesSize>;
};

struct WeightedMask : LCSTypedefs {
  Mask mask{0};
  int negatives{0};
  int positives{0};

  WeightedMask() = default;
  WeightedMask(const Mask& m, const Mask& pattern) : mask(m) {
    recalc(pattern);
  }
  WeightedMask(const Mask& m) : mask(m) {}

  void recalc(const Mask& pattern) {
    negatives = positives = 0;
    for (size_t i = 0; i < TypesSize; ++i) {
      if (mask[i] < pattern[i]) {
        negatives += pattern[i] - mask[i];
      } else if (mask[i] > pattern[i]) {
        positives += mask[i] - pattern[i];
      }
    }
  }

  WeightedMask merge(const Mask& rhs) {
    Mask dst(mask);
    auto it = rhs.begin();
    for (auto& coeff : dst)
      coeff += *it++;
    return dst;
  }
};

struct CombinedResult : LCSTypedefs {
  Candidates candidates;
  WeightedMask result;
  CombinedResult() = default;
  CombinedResult(const WeightedMask& mask) : result{mask} {}

  void judge(const CombinedResult& upstream) {
    judge(upstream.result, upstream.candidates);
  }
  void judge(const WeightedMask& mask, const Candidates& list) {
    /* A better result is one, that either has less negative weight,
    or at very least does not introduce new zeros and has less positive
    weight. A similar result has the same amount of zeros and the same
    pos weight. Or:

         \ POS |    <   |   ==   |   >
        NEG\   |        |        |
        -------+--------+--------+--------
           <   | BETTER | BETTER | BETTER
        -------+--------+--------+--------
           ==  | BETTER | EQUIV  | WORSE
        -------+--------+--------+--------
           >   | WORSE  | WORSE  | WORSE

    Additionaly, equivalent result will only saved, if the resulting player
    list is shorter. We do not want save equal-length list, as this would
    prevent us from selecting oldest-possible players.
    */

    if (mask.negatives > result.negatives)
      return;

    if (mask.negatives < result.negatives) {
      candidates = list;
      result = mask;
      return;
    }

    if (mask.positives > result.positives)
      return;

    if (mask.positives < result.positives) {
      candidates = list;
      result = mask;
      return;
    }

    if (candidates.empty() || candidates.size() > list.size()) {
      candidates = list;
      result = mask;
    }
  }
};

// This will look for the smallest number
// of oldest RUNNING players, which cover
// the missing capability in descriptor
class LongestRunningPlayerDfsSearch : public LCSTypedefs {
 public:
  void push_back(Candidate&& cand) { candidates_.push_back(std::move(cand)); }

  void sort() {
    // sorts the candidates based on number of covered
    // capabilites DESC first, and the timestamp ASC second
    std::sort(candidates_.begin(), candidates_.end(),
              [](const Candidate& lhs, const Candidate& rhs) {
                auto strength_left = lhs.descriptor.strength();
                auto strength_right = rhs.descriptor.strength();
                if (strength_left != strength_right)
                  return strength_left > strength_right;
                return lhs.timepoint < rhs.timepoint;
              });
  }

  Candidates::iterator begin() { return candidates_.begin(); }
  Candidates::iterator end() { return candidates_.end(); }

  // Entry point for DSF
  Candidates select(const platform::DescriptorType& descr) {
    auto pattern = descr2mask(descr);
    WeightedMask initial, result;
    for (size_t i = 0; i < TypesSize; ++i)
      initial.mask[i] = 0;
    initial.recalc(pattern);
    auto out = select(0, initial, pattern);
    out.result.recalc(pattern);
    // if there are any zeros still,
    // reject the result...
    if (out.result.negatives)
      return {};
    return std::move(out.candidates);
  }

 private:
  Candidates candidates_;

  static Mask descr2mask(const platform::DescriptorType& descr) {
    return descr2mask(descr, base::BuildSequence<TypesSize>{});
  }

  template <size_t... Index>
  static Mask descr2mask(const platform::DescriptorType& descr,
                         base::Sequence<Index...>) {
    Mask out;
    // This is better with C++17 "(Pack, ...)".
    // If the compiler ever supports C++17, then the next two
    // lines should be replaced with:
    //   ((out[Index] = descr.get<Index>() ? 1 : 0), ...);
    int ignore[] = {((out[Index] = descr.get<Index>() ? 1 : 0), 0)...};
    ALLOW_UNUSED_LOCAL(ignore);
    return out;
  }

  CombinedResult select(size_t lower,
                        WeightedMask initial,
                        const Mask& pattern,
                        Candidates in = {}) {
    CombinedResult result{initial};

    auto size = candidates_.size();
    for (size_t index = lower; index < size; ++index) {
      in.push_back(candidates_[index]);
      // add coefficients for this descriptor
      auto local = initial.merge(descr2mask(candidates_[index].descriptor));
      local.recalc(pattern);
      result.judge(local, in);

      // If there are no nulls, adding any other player to the mix
      // will only increase any ones we already have. So, we only go
      // deeper, if the are still some uncovered slot types.
      if (local.negatives) {
        auto res = select(index + 1, local, pattern, in);
        result.judge(res);
      }
      in.pop_back();
    }

    return result;
  }
};

}  // namespace detail

void LongestConflictStrategy::OnAdded(MediaPlayerInterfaceEfl* player) {
  LOG(INFO) << "Adding " << player << Info();
}

void LongestConflictStrategy::OnRemoved(MediaPlayerInterfaceEfl* player) {
  auto it = find(player);
  if (it != info_.end())
    info_.erase(it);
  LOG(INFO) << "Removing " << player << Info();
}

void LongestConflictStrategy::OnActivated(MediaPlayerInterfaceEfl* player) {
  auto now = Clock::now();
  auto it = find(player);
  if (it != info_.end()) {
    it->timepoint = now;
  } else {
    info_.push_back({player, now});
  }
  std::sort(info_.begin(), info_.end(),
            [](const ExtraInfo& lhs, const ExtraInfo& rhs) {
              return lhs.timepoint < rhs.timepoint;
            });
  LOG(INFO) << "Activating " << player << Info();
}

void LongestConflictStrategy::OnDeactivated(MediaPlayerInterfaceEfl* player) {
  auto it = find(player);
  if (it != info_.end())
    info_.erase(it);
  LOG(INFO) << "Deactivating " << player << Info();
}

LongestConflictStrategy::Resolution LongestConflictStrategy::Resolve(
    PlayerList& players,
    const SlotsType& slots,
    const DescriptorType& descr) {
  // OnPoked makes sure, active is sorted from oldest to newest
  Resolution out;

  // 0. Make a copy of descriptor, with all readily available
  //    capabilites unchecked. This will allow to locate a
  //    good partialy-compatible candidate.
  auto partial_descr = descr;
  detail::RemoveIfAvailable{}(slots, partial_descr);

  // 1. Find the oldest player with the same descriptor
  auto whole = players.end();
  for (auto it = players.begin(), end = players.end(); it != end; ++it) {
    const auto& item = it->second;
    if (item.state != PlayerState::RUNNING)
      continue;

    if (item.descriptor == descr || item.descriptor == partial_descr) {
      whole = it;
      break;
    }
  }

  // 1a. if found, evict and finish
  if (whole != players.end()) {
    out.push_back(whole);
    return out;
  }

  // 2. Lookup all partial matches
  detail::LongestRunningPlayerDfsSearch candidates;
  constexpr DescriptorType empty;

  for (auto it = players.begin(), end = players.end(); it != end; ++it) {
    const auto& item = it->second;
    if (item.state != PlayerState::RUNNING)
      continue;

    auto partial = item.descriptor & descr;
    if (partial != empty) {
      auto running = find(it->first);
      auto timepoint =
          running == info_.end() ? Timepoint::max() : running->timepoint;
      candidates.push_back({it, timepoint, partial});
    }
  }

  // 3. Sort them according to how many
  //    Capabilities are in common, then by time stamp
  candidates.sort();

  // 4. Search for minimal non-zero linear combination
  auto selected = candidates.select(descr);

  // 5. Place all selected in output;
  out.reserve(selected.size());
  for (auto const& sel : selected)
    out.push_back(sel.iter);

  return out;
}
}  // namespace media
