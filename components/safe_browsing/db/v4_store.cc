// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/db/v4_store.h"

#include "base/base64.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/sparse_histogram.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "components/safe_browsing/db/v4_rice.h"
#include "components/safe_browsing/db/v4_store.pb.h"
#include "components/safe_browsing/proto/webui.pb.h"
#include "crypto/secure_hash.h"
#include "crypto/sha2.h"

using base::TimeTicks;

namespace safe_browsing {

namespace {

// UMA related strings.
// Part 1: Represent the overall operation being performed.
const char kProcessFullUpdate[] = "SafeBrowsing.V4ProcessFullUpdate";
const char kProcessPartialUpdate[] = "SafeBrowsing.V4ProcessPartialUpdate";
const char kReadFromDisk[] = "SafeBrowsing.V4ReadFromDisk";
// Part 2: Represent the sub-operation being performed as part of the larger
// operation from part 1.
const char kApplyUpdate[] = ".ApplyUpdate";
const char kDecodeAdditions[] = ".DecodeAdditions";
const char kDecodeRemovals[] = ".DecodeRemovals";
const char kMergeUpdate[] = ".MergeUpdate";
// Part 3: Represent the unit of value being measured and logged.
const char kResult[] = ".Result";
const char kTime[] = ".Time";
// Part 4 (optional): Represent the name of the list for which the metric is
// being logged. For instance, ".UrlSoceng".
// UMA metric names for this file are generated by appending one value each,
// in order, from parts [1, 2, and 3], or [1, 2, 3, and 4]. For example:
// SafeBrowsing.V4ProcessPartialUpdate.ApplyUpdate.Result, or
// SafeBrowsing.V4ProcessPartialUpdate.ApplyUpdate.Result.UrlSoceng

const uint32_t kFileMagic = 0x600D71FE;
const uint32_t kFileVersion = 9;

void RecordTimeWithAndWithoutSuffix(const std::string& metric,
                                    base::TimeDelta time,
                                    const base::FilePath& file_path) {
  // The histograms below are a modified expansion of the
  // UMA_HISTOGRAM_LONG_TIMES macro adapted to allow for a dynamically suffixed
  // histogram name.
  // Note: The factory creates and owns the histogram.
  const int kBucketCount = 100;
  base::HistogramBase* histogram = base::Histogram::FactoryTimeGet(
      metric + kTime, base::TimeDelta::FromMilliseconds(1),
      base::TimeDelta::FromMinutes(1), kBucketCount,
      base::HistogramBase::kUmaTargetedHistogramFlag);
  if (histogram) {
    histogram->AddTime(time);
  }

  std::string suffix = GetUmaSuffixForStore(file_path);
  base::HistogramBase* histogram_suffix = base::Histogram::FactoryTimeGet(
      metric + kTime + suffix, base::TimeDelta::FromMilliseconds(1),
      base::TimeDelta::FromMinutes(1), kBucketCount,
      base::HistogramBase::kUmaTargetedHistogramFlag);
  if (histogram_suffix) {
    histogram_suffix->AddTime(time);
  }
}

void RecordEnumWithAndWithoutSuffix(const std::string& metric,
                                    int32_t value,
                                    int32_t maximum,
                                    const base::FilePath& file_path) {
  // The histograms below are an expansion of the UMA_HISTOGRAM_ENUMERATION
  // macro adapted to allow for a dynamically suffixed histogram name.
  // Note: The factory creates and owns the histogram.
  base::HistogramBase* histogram = base::LinearHistogram::FactoryGet(
      metric + kResult, 1, maximum, maximum + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag);
  if (histogram) {
    histogram->Add(value);
  }

  std::string suffix = GetUmaSuffixForStore(file_path);
  base::HistogramBase* histogram_suffix = base::LinearHistogram::FactoryGet(
      metric + kResult + suffix, 1, maximum, maximum + 1,
      base::HistogramBase::kUmaTargetedHistogramFlag);
  if (histogram_suffix) {
    histogram_suffix->Add(value);
  }
}

void RecordAddUnlumpedHashesTime(base::TimeDelta time) {
  UMA_HISTOGRAM_LONG_TIMES("SafeBrowsing.V4AddUnlumpedHashes.Time", time);
}

void RecordApplyUpdateResult(const std::string& base_metric,
                             ApplyUpdateResult result,
                             const base::FilePath& file_path) {
  RecordEnumWithAndWithoutSuffix(base_metric + kApplyUpdate, result,
                                 APPLY_UPDATE_RESULT_MAX, file_path);
}

void RecordApplyUpdateTime(const std::string& base_metric,
                           base::TimeDelta time,
                           const base::FilePath& file_path) {
  RecordTimeWithAndWithoutSuffix(base_metric + kApplyUpdate, time, file_path);
}

void RecordDecodeAdditionsResult(const std::string& base_metric,
                                 V4DecodeResult result,
                                 const base::FilePath& file_path) {
  RecordEnumWithAndWithoutSuffix(base_metric + kDecodeAdditions, result,
                                 DECODE_RESULT_MAX, file_path);
}

void RecordDecodeAdditionsTime(const std::string& base_metric,
                               base::TimeDelta time,
                               const base::FilePath& file_path) {
  RecordTimeWithAndWithoutSuffix(base_metric + kDecodeAdditions, time,
                                 file_path);
}

void RecordDecodeRemovalsResult(const std::string& base_metric,
                                V4DecodeResult result,
                                const base::FilePath& file_path) {
  RecordEnumWithAndWithoutSuffix(base_metric + kDecodeRemovals, result,
                                 DECODE_RESULT_MAX, file_path);
}

void RecordDecodeRemovalsTime(const std::string& base_metric,
                              base::TimeDelta time,
                              const base::FilePath& file_path) {
  RecordTimeWithAndWithoutSuffix(base_metric + kDecodeRemovals, time,
                                 file_path);
}

void RecordMergeUpdateTime(const std::string& base_metric,
                           base::TimeDelta time,
                           const base::FilePath& file_path) {
  RecordTimeWithAndWithoutSuffix(base_metric + kMergeUpdate, time, file_path);
}

void RecordStoreReadResult(StoreReadResult result) {
  UMA_HISTOGRAM_ENUMERATION("SafeBrowsing.V4StoreRead.Result", result,
                            STORE_READ_RESULT_MAX);
}

void RecordStoreWriteResult(StoreWriteResult result) {
  UMA_HISTOGRAM_ENUMERATION("SafeBrowsing.V4StoreWrite.Result", result,
                            STORE_WRITE_RESULT_MAX);
}

// Returns the name of the temporary file used to buffer data for
// |filename|.  Exported for unit tests.
const base::FilePath TemporaryFileForFilename(const base::FilePath& filename) {
  return base::FilePath(filename.value() + FILE_PATH_LITERAL("_new"));
}

}  // namespace

using ::google::protobuf::RepeatedField;
using ::google::protobuf::RepeatedPtrField;
using ::google::protobuf::int32;

std::ostream& operator<<(std::ostream& os, const V4Store& store) {
  os << store.DebugString();
  return os;
}

std::unique_ptr<V4Store> V4StoreFactory::CreateV4Store(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const base::FilePath& store_path) {
  auto new_store = base::MakeUnique<V4Store>(task_runner, store_path);
  new_store->Initialize();
  return new_store;
}

void V4Store::Initialize() {
  // If a state already exists, don't re-initilize.
  DCHECK(state_.empty());

  StoreReadResult store_read_result = ReadFromDisk();
  has_valid_data_ = (store_read_result == READ_SUCCESS);
  RecordStoreReadResult(store_read_result);
}

bool V4Store::HasValidData() const {
  return has_valid_data_;
}

V4Store::V4Store(const scoped_refptr<base::SequencedTaskRunner>& task_runner,
                 const base::FilePath& store_path,
                 const int64_t old_file_size)
    : file_size_(old_file_size),
      has_valid_data_(false),
      store_path_(store_path),
      task_runner_(task_runner) {}

V4Store::~V4Store() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
}

std::string V4Store::DebugString() const {
  std::string state_base64;
  base::Base64Encode(state_, &state_base64);

  return base::StringPrintf("path: %" PRIsFP "; state: %s",
                            store_path_.value().c_str(), state_base64.c_str());
}

// static
void V4Store::Destroy(std::unique_ptr<V4Store> v4_store) {
  V4Store* v4_store_raw = v4_store.release();
  if (v4_store_raw) {
    v4_store_raw->task_runner_->DeleteSoon(FROM_HERE, v4_store_raw);
  }
}

void V4Store::Reset() {
  expected_checksum_.clear();
  hash_prefix_map_.clear();
  state_ = "";
}

ApplyUpdateResult V4Store::ProcessPartialUpdateAndWriteToDisk(
    const std::string& metric,
    const HashPrefixMap& hash_prefix_map_old,
    std::unique_ptr<ListUpdateResponse> response) {
  DCHECK(response->has_response_type());
  DCHECK_EQ(ListUpdateResponse::PARTIAL_UPDATE, response->response_type());

  ApplyUpdateResult result = ProcessUpdate(
      metric, hash_prefix_map_old, response, false /* delay_checksum check */);
  if (result == APPLY_UPDATE_SUCCESS) {
    Checksum checksum = response->checksum();
    response.reset();
    RecordStoreWriteResult(WriteToDisk(checksum));
  }
  return result;
}

ApplyUpdateResult V4Store::ProcessFullUpdateAndWriteToDisk(
    const std::string& metric,
    std::unique_ptr<ListUpdateResponse> response) {
  ApplyUpdateResult result =
      ProcessFullUpdate(metric, response, false /* delay_checksum check */);
  if (result == APPLY_UPDATE_SUCCESS) {
    Checksum checksum = response->checksum();
    response.reset();
    RecordStoreWriteResult(WriteToDisk(checksum));
  }
  return result;
}

ApplyUpdateResult V4Store::ProcessFullUpdate(
    const std::string& metric,
    const std::unique_ptr<ListUpdateResponse>& response,
    bool delay_checksum_check) {
  DCHECK(response->has_response_type());
  DCHECK_EQ(ListUpdateResponse::FULL_UPDATE, response->response_type());
  // TODO(vakh): For a full update, we don't need to process the update in
  // lexographical order to store it, but we do need to do that for calculating
  // checksum. It might save some CPU cycles to store the full update as-is and
  // walk the list of hash prefixes in lexographical order only for checksum
  // calculation.
  return ProcessUpdate(metric, HashPrefixMap(), response, delay_checksum_check);
}

ApplyUpdateResult V4Store::ProcessUpdate(
    const std::string& metric,
    const HashPrefixMap& hash_prefix_map_old,
    const std::unique_ptr<ListUpdateResponse>& response,
    bool delay_checksum_check) {
  const RepeatedField<int32>* raw_removals = nullptr;
  RepeatedField<int32> rice_removals;
  size_t removals_size = response->removals_size();
  DCHECK_LE(removals_size, 1u);
  if (removals_size == 1) {
    const ThreatEntrySet& removal = response->removals().Get(0);
    const CompressionType compression_type = removal.compression_type();
    if (compression_type == RAW) {
      raw_removals = &removal.raw_indices().indices();
    } else if (compression_type == RICE) {
      DCHECK(removal.has_rice_indices());

      const RiceDeltaEncoding& rice_indices = removal.rice_indices();
      TimeTicks before = TimeTicks::Now();
      V4DecodeResult decode_result = V4RiceDecoder::DecodeIntegers(
          rice_indices.first_value(), rice_indices.rice_parameter(),
          rice_indices.num_entries(), rice_indices.encoded_data(),
          &rice_removals);

      RecordDecodeRemovalsResult(metric, decode_result, store_path_);
      if (decode_result != DECODE_SUCCESS) {
        return RICE_DECODING_FAILURE;
      }
      RecordDecodeRemovalsTime(metric, TimeTicks::Now() - before, store_path_);
      raw_removals = &rice_removals;
    } else {
      NOTREACHED() << "Unexpected compression_type type: " << compression_type;
      return UNEXPECTED_COMPRESSION_TYPE_REMOVALS_FAILURE;
    }
  }

  HashPrefixMap hash_prefix_map;
  ApplyUpdateResult apply_update_result = UpdateHashPrefixMapFromAdditions(
      metric, response->additions(), &hash_prefix_map);
  if (apply_update_result != APPLY_UPDATE_SUCCESS) {
    return apply_update_result;
  }

  std::string expected_checksum;
  if (response->has_checksum() && response->checksum().has_sha256()) {
    expected_checksum = response->checksum().sha256();
  }

  TimeTicks before = TimeTicks::Now();
  if (delay_checksum_check) {
    DCHECK(hash_prefix_map_old.empty());
    DCHECK(!raw_removals);
    // We delay the checksum check at startup to be able to load the DB
    // quickly. In this case, the |hash_prefix_map_old| should be empty, so just
    // copy over the |hash_prefix_map|.
    hash_prefix_map_ = hash_prefix_map;

    // Calculate the checksum asynchronously later and if it doesn't match,
    // reset the store.
    expected_checksum_ = expected_checksum;

    apply_update_result = APPLY_UPDATE_SUCCESS;
  } else {
    apply_update_result = MergeUpdate(hash_prefix_map_old, hash_prefix_map,
                                      raw_removals, expected_checksum);
    if (apply_update_result != APPLY_UPDATE_SUCCESS) {
      return apply_update_result;
    }
  }
  RecordMergeUpdateTime(metric, TimeTicks::Now() - before, store_path_);

  state_ = response->new_client_state();
  return APPLY_UPDATE_SUCCESS;
}

void V4Store::ApplyUpdate(
    std::unique_ptr<ListUpdateResponse> response,
    const scoped_refptr<base::SingleThreadTaskRunner>& callback_task_runner,
    UpdatedStoreReadyCallback callback) {
  std::unique_ptr<V4Store> new_store(
      new V4Store(task_runner_, store_path_, file_size_));
  ApplyUpdateResult apply_update_result;
  std::string metric;
  TimeTicks before = TimeTicks::Now();
  if (response->response_type() == ListUpdateResponse::PARTIAL_UPDATE) {
    metric = kProcessPartialUpdate;
    apply_update_result = new_store->ProcessPartialUpdateAndWriteToDisk(
        metric, hash_prefix_map_, std::move(response));
  } else if (response->response_type() == ListUpdateResponse::FULL_UPDATE) {
    metric = kProcessFullUpdate;
    apply_update_result =
        new_store->ProcessFullUpdateAndWriteToDisk(metric, std::move(response));
  } else {
    apply_update_result = UNEXPECTED_RESPONSE_TYPE_FAILURE;
    NOTREACHED() << "Failure: Unexpected response type: "
                 << response->response_type();
  }

  if (apply_update_result == APPLY_UPDATE_SUCCESS) {
    new_store->has_valid_data_ = true;
    new_store->last_apply_update_result_ = apply_update_result;
    new_store->last_apply_update_time_millis_ = base::Time::Now();
    new_store->checks_attempted_ = checks_attempted_;
    RecordApplyUpdateTime(metric, TimeTicks::Now() - before, store_path_);
  } else {
    new_store.reset();
    DLOG(WARNING) << "Failure: ApplyUpdate: reason: " << apply_update_result
                  << "; store: " << *this;
  }

  // Record the state of the update to be shown in the Safe Browsing page.
  last_apply_update_result_ = apply_update_result;

  RecordApplyUpdateResult(metric, apply_update_result, store_path_);

  // Posting the task should be the last thing to do in this function.
  // Otherwise, the posted task can end up running in parallel. If that
  // happens, the old store will get destoyed and can lead to use-after-free in
  // this function.
  callback_task_runner->PostTask(
      FROM_HERE, base::Bind(callback, base::Passed(&new_store)));
}

ApplyUpdateResult V4Store::UpdateHashPrefixMapFromAdditions(
    const std::string& metric,
    const RepeatedPtrField<ThreatEntrySet>& additions,
    HashPrefixMap* additions_map) {
  for (const auto& addition : additions) {
    ApplyUpdateResult apply_update_result = APPLY_UPDATE_SUCCESS;
    const CompressionType compression_type = addition.compression_type();
    if (compression_type == RAW) {
      DCHECK(addition.has_raw_hashes());
      DCHECK(addition.raw_hashes().has_raw_hashes());

      apply_update_result =
          AddUnlumpedHashes(addition.raw_hashes().prefix_size(),
                            addition.raw_hashes().raw_hashes(), additions_map);
    } else if (compression_type == RICE) {
      DCHECK(addition.has_rice_hashes());

      const RiceDeltaEncoding& rice_hashes = addition.rice_hashes();
      std::vector<uint32_t> raw_hashes;
      TimeTicks before = TimeTicks::Now();
      V4DecodeResult decode_result = V4RiceDecoder::DecodePrefixes(
          rice_hashes.first_value(), rice_hashes.rice_parameter(),
          rice_hashes.num_entries(), rice_hashes.encoded_data(), &raw_hashes);
      RecordDecodeAdditionsResult(metric, decode_result, store_path_);
      if (decode_result != DECODE_SUCCESS) {
        return RICE_DECODING_FAILURE;
      } else {
        RecordDecodeAdditionsTime(metric, TimeTicks::Now() - before,
                                  store_path_);
        char* raw_hashes_start = reinterpret_cast<char*>(raw_hashes.data());
        size_t raw_hashes_size = sizeof(uint32_t) * raw_hashes.size();

        // Rice-Golomb encoding is used to send compressed compressed 4-byte
        // hash prefixes. Hash prefixes longer than 4 bytes will not be
        // compressed, and will be served in raw format instead.
        // Source: https://developers.google.com/safe-browsing/v4/compression
        const PrefixSize kPrefixSize = 4;
        apply_update_result = AddUnlumpedHashes(kPrefixSize, raw_hashes_start,
                                                raw_hashes_size, additions_map);
      }
    } else {
      NOTREACHED() << "Unexpected compression_type type: " << compression_type;
      return UNEXPECTED_COMPRESSION_TYPE_ADDITIONS_FAILURE;
    }

    if (apply_update_result != APPLY_UPDATE_SUCCESS) {
      // If there was an error in updating the map, discard the update entirely.
      return apply_update_result;
    }
  }

  return APPLY_UPDATE_SUCCESS;
}

// static
ApplyUpdateResult V4Store::AddUnlumpedHashes(PrefixSize prefix_size,
                                             const std::string& raw_hashes,
                                             HashPrefixMap* additions_map) {
  return AddUnlumpedHashes(prefix_size, raw_hashes.data(), raw_hashes.size(),
                           additions_map);
}

// static
ApplyUpdateResult V4Store::AddUnlumpedHashes(PrefixSize prefix_size,
                                             const char* raw_hashes_begin,
                                             const size_t raw_hashes_length,
                                             HashPrefixMap* additions_map) {
  if (prefix_size < kMinHashPrefixLength) {
    NOTREACHED();
    return PREFIX_SIZE_TOO_SMALL_FAILURE;
  }
  if (prefix_size > kMaxHashPrefixLength) {
    NOTREACHED();
    return PREFIX_SIZE_TOO_LARGE_FAILURE;
  }
  if (raw_hashes_length % prefix_size != 0) {
    return ADDITIONS_SIZE_UNEXPECTED_FAILURE;
  }

  TimeTicks before = TimeTicks::Now();
  // TODO(vakh): Figure out a way to avoid the following copy operation.
  (*additions_map)[prefix_size] =
      std::string(raw_hashes_begin, raw_hashes_begin + raw_hashes_length);
  RecordAddUnlumpedHashesTime(TimeTicks::Now() - before);
  return APPLY_UPDATE_SUCCESS;
}

// static
bool V4Store::GetNextSmallestUnmergedPrefix(
    const HashPrefixMap& hash_prefix_map,
    const IteratorMap& iterator_map,
    HashPrefix* smallest_hash_prefix) {
  HashPrefix current_hash_prefix;
  bool has_unmerged = false;

  for (const auto& iterator_pair : iterator_map) {
    PrefixSize prefix_size = iterator_pair.first;
    HashPrefixes::const_iterator start = iterator_pair.second;
    const HashPrefixes& hash_prefixes = hash_prefix_map.at(prefix_size);
    if (prefix_size <=
        static_cast<PrefixSize>(std::distance(start, hash_prefixes.end()))) {
      current_hash_prefix = HashPrefix(start, start + prefix_size);
      if (!has_unmerged || *smallest_hash_prefix > current_hash_prefix) {
        has_unmerged = true;
        smallest_hash_prefix->swap(current_hash_prefix);
      }
    }
  }
  return has_unmerged;
}

// static
void V4Store::InitializeIteratorMap(const HashPrefixMap& hash_prefix_map,
                                    IteratorMap* iterator_map) {
  for (const auto& map_pair : hash_prefix_map) {
    (*iterator_map)[map_pair.first] = map_pair.second.begin();
  }
}

// static
void V4Store::ReserveSpaceInPrefixMap(const HashPrefixMap& other_prefixes_map,
                                      HashPrefixMap* prefix_map_to_update) {
  for (const auto& pair : other_prefixes_map) {
    PrefixSize prefix_size = pair.first;
    size_t prefix_length_to_add = pair.second.length();

    const HashPrefixes& existing_prefixes =
        (*prefix_map_to_update)[prefix_size];
    size_t existing_capacity = existing_prefixes.capacity();

    (*prefix_map_to_update)[prefix_size].reserve(existing_capacity +
                                                 prefix_length_to_add);
  }
}

ApplyUpdateResult V4Store::MergeUpdate(const HashPrefixMap& old_prefixes_map,
                                       const HashPrefixMap& additions_map,
                                       const RepeatedField<int32>* raw_removals,
                                       const std::string& expected_checksum) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK(hash_prefix_map_.empty());

  bool calculate_checksum = !expected_checksum.empty();
  if (calculate_checksum &&
      (expected_checksum.size() != crypto::kSHA256Length)) {
    return CHECKSUM_MISMATCH_FAILURE;
  }

  hash_prefix_map_.clear();
  ReserveSpaceInPrefixMap(old_prefixes_map, &hash_prefix_map_);
  ReserveSpaceInPrefixMap(additions_map, &hash_prefix_map_);

  IteratorMap old_iterator_map;
  HashPrefix next_smallest_prefix_old;
  InitializeIteratorMap(old_prefixes_map, &old_iterator_map);
  bool old_has_unmerged = GetNextSmallestUnmergedPrefix(
      old_prefixes_map, old_iterator_map, &next_smallest_prefix_old);

  IteratorMap additions_iterator_map;
  HashPrefix next_smallest_prefix_additions;
  InitializeIteratorMap(additions_map, &additions_iterator_map);
  bool additions_has_unmerged = GetNextSmallestUnmergedPrefix(
      additions_map, additions_iterator_map, &next_smallest_prefix_additions);

  // Classical merge sort.
  // The two constructs to merge are maps: old_prefixes_map, additions_map.
  // At least one of the maps still has elements that need to be merged into the
  // new store.

  std::unique_ptr<crypto::SecureHash> checksum_ctx(
      crypto::SecureHash::Create(crypto::SecureHash::SHA256));

  // Keep track of the number of elements picked from the old map. This is used
  // to determine which elements to drop based on the raw_removals. Note that
  // picked is not the same as merged. A picked element isn't merged if its
  // index is on the raw_removals list.
  int total_picked_from_old = 0;
  const int* removals_iter = raw_removals ? raw_removals->begin() : nullptr;
  while (old_has_unmerged || additions_has_unmerged) {
    // If the same hash prefix appears in the existing store and the additions
    // list, something is clearly wrong. Discard the update.
    if (old_has_unmerged && additions_has_unmerged &&
        next_smallest_prefix_old == next_smallest_prefix_additions) {
      return ADDITIONS_HAS_EXISTING_PREFIX_FAILURE;
    }

    // Select which map to pick the next hash prefix from to keep the result in
    // lexographically sorted order.
    bool pick_from_old =
        old_has_unmerged &&
        (!additions_has_unmerged ||
         (next_smallest_prefix_old < next_smallest_prefix_additions));

    PrefixSize next_smallest_prefix_size;
    if (pick_from_old) {
      next_smallest_prefix_size = next_smallest_prefix_old.size();

      // Update the iterator map, which means that we have merged one hash
      // prefix of size |next_smallest_prefix_size| from the old store.
      old_iterator_map[next_smallest_prefix_size] += next_smallest_prefix_size;

      if (!raw_removals || removals_iter == raw_removals->end() ||
          *removals_iter != total_picked_from_old) {
        // Append the smallest hash to the appropriate list.
        hash_prefix_map_[next_smallest_prefix_size] += next_smallest_prefix_old;

        if (calculate_checksum) {
          checksum_ctx->Update(next_smallest_prefix_old.data(),
                               next_smallest_prefix_size);
        }
      } else {
        // Element not added to new map. Move the removals iterator forward.
        removals_iter++;
      }

      total_picked_from_old++;

      // Find the next smallest unmerged element in the old store's map.
      old_has_unmerged = GetNextSmallestUnmergedPrefix(
          old_prefixes_map, old_iterator_map, &next_smallest_prefix_old);
    } else {
      next_smallest_prefix_size = next_smallest_prefix_additions.size();

      // Append the smallest hash to the appropriate list.
      hash_prefix_map_[next_smallest_prefix_size] +=
          next_smallest_prefix_additions;

      if (calculate_checksum) {
        checksum_ctx->Update(next_smallest_prefix_additions.data(),
                             next_smallest_prefix_size);
      }

      // Update the iterator map, which means that we have merged one hash
      // prefix of size |next_smallest_prefix_size| from the update.
      additions_iterator_map[next_smallest_prefix_size] +=
          next_smallest_prefix_size;

      // Find the next smallest unmerged element in the additions map.
      additions_has_unmerged =
          GetNextSmallestUnmergedPrefix(additions_map, additions_iterator_map,
                                        &next_smallest_prefix_additions);
    }
  }

  if (raw_removals && removals_iter != raw_removals->end()) {
    return REMOVALS_INDEX_TOO_LARGE_FAILURE;
  }

  if (calculate_checksum) {
    char checksum[crypto::kSHA256Length];
    checksum_ctx->Finish(checksum, sizeof(checksum));
    for (size_t i = 0; i < crypto::kSHA256Length; i++) {
      if (checksum[i] != expected_checksum[i]) {
#if DCHECK_IS_ON()
        std::string checksum_b64, expected_checksum_b64;
        base::Base64Encode(base::StringPiece(checksum, arraysize(checksum)),
                           &checksum_b64);
        base::Base64Encode(expected_checksum, &expected_checksum_b64);
        DVLOG(1) << "Failure: Checksum mismatch: calculated: " << checksum_b64
                 << "; expected: " << expected_checksum_b64
                 << "; store: " << *this;
#endif
        return CHECKSUM_MISMATCH_FAILURE;
      }
    }
  }

  return APPLY_UPDATE_SUCCESS;
}

StoreReadResult V4Store::ReadFromDisk() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  V4StoreFileFormat file_format;
  int64_t file_size;
  TimeTicks before = TimeTicks::Now();
  {
    // A temporary scope to make sure that |contents| get destroyed as soon as
    // we are doing using it.
    std::string contents;
    bool read_success = base::ReadFileToString(store_path_, &contents);
    if (!read_success) {
      return FILE_UNREADABLE_FAILURE;
    }

    if (contents.empty()) {
      return FILE_EMPTY_FAILURE;
    }

    if (!file_format.ParseFromString(contents)) {
      return PROTO_PARSING_FAILURE;
    }
    file_size = static_cast<int64_t>(contents.size());
  }

  if (file_format.magic_number() != kFileMagic) {
    return UNEXPECTED_MAGIC_NUMBER_FAILURE;
  }

  UMA_HISTOGRAM_SPARSE_SLOWLY("SafeBrowsing.V4StoreVersionRead",
                              file_format.version_number());
  if (file_format.version_number() != kFileVersion) {
    return FILE_VERSION_INCOMPATIBLE_FAILURE;
  }

  if (!file_format.has_list_update_response()) {
    return HASH_PREFIX_INFO_MISSING_FAILURE;
  }

  std::unique_ptr<ListUpdateResponse> response(new ListUpdateResponse);
  response->Swap(file_format.mutable_list_update_response());
  ApplyUpdateResult apply_update_result = ProcessFullUpdate(
      kReadFromDisk, response, true /* delay_checksum check */);
  RecordApplyUpdateResult(kReadFromDisk, apply_update_result, store_path_);
  last_apply_update_result_ = apply_update_result;
  if (apply_update_result != APPLY_UPDATE_SUCCESS) {
    hash_prefix_map_.clear();
    return HASH_PREFIX_MAP_GENERATION_FAILURE;
  }
  RecordApplyUpdateTime(kReadFromDisk, TimeTicks::Now() - before, store_path_);

  // Update |file_size_| now because we parsed the file correctly.
  file_size_ = file_size;

  return READ_SUCCESS;
}

StoreWriteResult V4Store::WriteToDisk(const Checksum& checksum) {
  V4StoreFileFormat file_format;
  ListUpdateResponse* lur = file_format.mutable_list_update_response();
  *(lur->mutable_checksum()) = checksum;
  lur->set_new_client_state(state_);
  lur->set_response_type(ListUpdateResponse::FULL_UPDATE);
  for (auto map_iter : hash_prefix_map_) {
    ThreatEntrySet* additions = lur->add_additions();
    // TODO(vakh): Write RICE encoded hash prefixes on disk. Not doing so
    // currently since it takes a long time to decode them on startup, which
    // blocks resource load. See: http://crbug.com/654819
    additions->set_compression_type(RAW);
    additions->mutable_raw_hashes()->set_prefix_size(map_iter.first);
    additions->mutable_raw_hashes()->set_raw_hashes(map_iter.second);
  }

  // Attempt writing to a temporary file first and at the end, swap the files.
  const base::FilePath new_filename = TemporaryFileForFilename(store_path_);

  file_format.set_magic_number(kFileMagic);
  file_format.set_version_number(kFileVersion);
  std::string file_format_string;
  file_format.SerializeToString(&file_format_string);
  size_t written = base::WriteFile(new_filename, file_format_string.data(),
                                   file_format_string.size());

  if (file_format_string.size() != written) {
    return UNEXPECTED_BYTES_WRITTEN_FAILURE;
  }

  if (!base::Move(new_filename, store_path_)) {
    return UNABLE_TO_RENAME_FAILURE;
  }

  // Update |file_size_| now because we wrote the file correctly.
  file_size_ = static_cast<int64_t>(written);

  return WRITE_SUCCESS;
}

HashPrefix V4Store::GetMatchingHashPrefix(const FullHash& full_hash) {
  // It should never be the case that more than one hash prefixes match a given
  // full hash. However, if that happens, this method returns any one of them.
  // It does not guarantee which one of those will be returned.
  DCHECK(full_hash.size() == 32u || full_hash.size() == 21u);
  checks_attempted_++;
  for (const auto& pair : hash_prefix_map_) {
    const PrefixSize& prefix_size = pair.first;
    const HashPrefixes& hash_prefixes = pair.second;
    HashPrefix hash_prefix = full_hash.substr(0, prefix_size);
    if (HashPrefixMatches(hash_prefix, hash_prefixes.begin(),
                          hash_prefixes.end())) {
      return hash_prefix;
    }
  }
  return HashPrefix();
}

// static
bool V4Store::HashPrefixMatches(const HashPrefix& hash_prefix,
                                const HashPrefixes::const_iterator& begin,
                                const HashPrefixes::const_iterator& end) {
  if (begin == end) {
    return false;
  }
  size_t distance = std::distance(begin, end);
  const PrefixSize prefix_size = hash_prefix.length();
  DCHECK_EQ(0u, distance % prefix_size);
  size_t mid_prefix_index = ((distance / prefix_size) / 2) * prefix_size;
  HashPrefixes::const_iterator mid = begin + mid_prefix_index;
  HashPrefix mid_prefix = HashPrefix(mid, mid + prefix_size);
  int result = hash_prefix.compare(mid_prefix);
  if (result == 0) {
    return true;
  } else if (result < 0) {
    return HashPrefixMatches(hash_prefix, begin, mid);
  } else {
    return HashPrefixMatches(hash_prefix, mid + prefix_size, end);
  }
}

bool V4Store::VerifyChecksum() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  if (expected_checksum_.empty()) {
    // Nothing to check here folks!
    // TODO(vakh): Do not allow empty checksums.
    return true;
  }

  IteratorMap iterator_map;
  HashPrefix next_smallest_prefix;
  InitializeIteratorMap(hash_prefix_map_, &iterator_map);
  bool has_unmerged = GetNextSmallestUnmergedPrefix(
      hash_prefix_map_, iterator_map, &next_smallest_prefix);

  std::unique_ptr<crypto::SecureHash> checksum_ctx(
      crypto::SecureHash::Create(crypto::SecureHash::SHA256));
  while (has_unmerged) {
    PrefixSize next_smallest_prefix_size;
    next_smallest_prefix_size = next_smallest_prefix.size();

    // Update the iterator map, which means that we have read one hash
    // prefix of size |next_smallest_prefix_size| from hash_prefix_map_.
    iterator_map[next_smallest_prefix_size] += next_smallest_prefix_size;

    checksum_ctx->Update(next_smallest_prefix.data(),
                         next_smallest_prefix_size);

    // Find the next smallest unmerged element in the map.
    has_unmerged = GetNextSmallestUnmergedPrefix(hash_prefix_map_, iterator_map,
                                                 &next_smallest_prefix);
  }

  char checksum[crypto::kSHA256Length];
  checksum_ctx->Finish(checksum, sizeof(checksum));
  for (size_t i = 0; i < crypto::kSHA256Length; i++) {
    if (checksum[i] != expected_checksum_[i]) {
      RecordApplyUpdateResult(kReadFromDisk, CHECKSUM_MISMATCH_FAILURE,
                              store_path_);
#if DCHECK_IS_ON()
      std::string checksum_b64, expected_checksum_b64;
      base::Base64Encode(base::StringPiece(checksum, arraysize(checksum)),
                         &checksum_b64);
      base::Base64Encode(expected_checksum_, &expected_checksum_b64);
      DVLOG(1) << "Failure: Checksum mismatch: calculated: " << checksum_b64
               << "; expected: " << expected_checksum_b64
               << "; store: " << *this;
#endif
      return false;
    }
  }
  return true;
}

int64_t V4Store::RecordAndReturnFileSize(const std::string& base_metric) {
  std::string suffix = GetUmaSuffixForStore(store_path_);
  // Histogram properties as in UMA_HISTOGRAM_COUNTS macro.
  base::HistogramBase* histogram = base::Histogram::FactoryGet(
      base_metric + suffix, 1, 1000000, 50,
      base::HistogramBase::kUmaTargetedHistogramFlag);
  if (histogram) {
    const int64_t file_size_kilobytes = file_size_ / 1024;
    histogram->Add(file_size_kilobytes);
  }
  return file_size_;
}

void V4Store::CollectStoreInfo(
    DatabaseManagerInfo::DatabaseInfo::StoreInfo* store_info,
    const std::string& base_metric) {
  store_info->set_file_name(base_metric + GetUmaSuffixForStore(store_path_));
  store_info->set_file_size_bytes(file_size_);
  store_info->set_update_status(static_cast<int>(last_apply_update_result_));
  store_info->set_checks_attempted(checks_attempted_);
  if (last_apply_update_time_millis_.ToJavaTime()) {
    store_info->set_last_apply_update_time_millis(
        last_apply_update_time_millis_.ToJavaTime());
  }
}

}  // namespace safe_browsing
