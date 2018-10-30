// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BLINK_BUFFERED_DATA_SOURCE_HOST_IMPL_H_
#define MEDIA_BLINK_BUFFERED_DATA_SOURCE_HOST_IMPL_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/containers/circular_deque.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "media/base/ranges.h"
#include "media/blink/interval_map.h"
#include "media/blink/media_blink_export.h"

namespace media {

// Interface for testing purposes.
class MEDIA_BLINK_EXPORT BufferedDataSourceHost {
 public:
  // Notify the host of the total size of the media file.
  virtual void SetTotalBytes(int64_t total_bytes) = 0;

  // Notify the host that byte range [start,end] has been buffered.
  // TODO(fischman): remove this method when demuxing is push-based instead of
  // pull-based.  http://crbug.com/131444
  virtual void AddBufferedByteRange(int64_t start, int64_t end) = 0;

 protected:
  virtual ~BufferedDataSourceHost() {}
};

// Provides an implementation of BufferedDataSourceHost that translates the
// buffered byte ranges into estimated time ranges.
class MEDIA_BLINK_EXPORT BufferedDataSourceHostImpl
    : public BufferedDataSourceHost {
 public:
  BufferedDataSourceHostImpl(base::Closure progress_cb,
                             base::TickClock* tick_clock);
  ~BufferedDataSourceHostImpl() override;

  // BufferedDataSourceHost implementation.
  void SetTotalBytes(int64_t total_bytes) override;
  void AddBufferedByteRange(int64_t start, int64_t end) override;

  // Translate the byte ranges to time ranges and append them to the list.
  // TODO(sandersd): This is a confusing name, find something better.
  void AddBufferedTimeRanges(
      Ranges<base::TimeDelta>* buffered_time_ranges,
      base::TimeDelta media_duration) const;

  bool DidLoadingProgress();

  // Returns true if we have enough buffered bytes to play from now
  // until the end of media
  bool CanPlayThrough(base::TimeDelta current_position,
                      base::TimeDelta media_duration,
                      double playback_rate) const;

  // Caller must make sure |tick_clock| is valid for lifetime of this object.
  void SetTickClockForTest(base::TickClock* tick_clock);

 private:
  // Returns number of bytes not yet loaded in the given interval.
  int64_t UnloadedBytesInInterval(const Interval<int64_t>& interval) const;

  // Returns an estimate of the download rate.
  // Returns 0.0 if an estimate cannot be made.
  double DownloadRate() const;

  // Total size of the data source.
  int64_t total_bytes_;

  // List of buffered byte ranges for estimating buffered time.
  // The InterValMap value is 1 for bytes that are buffered, 0 otherwise.
  IntervalMap<int64_t, int> buffered_byte_ranges_;

  // True when AddBufferedByteRange() has been called more recently than
  // DidLoadingProgress().
  bool did_loading_progress_;

  // Contains how much we had downloaded at a given time.
  // Pruned to contain roughly the last 10 seconds of data.
  base::circular_deque<std::pair<base::TimeTicks, uint64_t>> download_history_;
  base::Closure progress_cb_;

  base::TickClock* tick_clock_;

  FRIEND_TEST_ALL_PREFIXES(BufferedDataSourceHostImplTest, CanPlayThrough);
  FRIEND_TEST_ALL_PREFIXES(BufferedDataSourceHostImplTest,
                           CanPlayThroughSmallAdvances);
  DISALLOW_COPY_AND_ASSIGN(BufferedDataSourceHostImpl);
};

}  // namespace media

#endif  // MEDIA_BLINK_BUFFERED_DATA_SOURCE_HOST_IMPL_H_
