// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_ELEMENTARY_STREAM_LISTENER_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_ELEMENTARY_STREAM_LISTENER_SAMSUNG_H_

#include "ppapi/c/samsung/ppp_media_data_source_samsung.h"

/// @file
/// This file defines a <code>ElementaryStreamListener_Samsung</code> type which
/// allows plugin to receive elementary stream events.
namespace pp {

/// Listener for receiving elementary stream related events.
class ElementaryStreamListener_Samsung {
 public:
  virtual ~ElementaryStreamListener_Samsung();

  /// Event is fired when internal queue is running out of data.
  /// Application should start pushing more data via
  /// <code>ElementaryStream_Samsung::AppendPacket</code> or
  /// <code>ElementaryStream_Samsung::AppendEncryptedPacket</code> function.
  virtual void OnNeedData(int32_t bytes_max) = 0;

  /// Event is fired when internal queue is full. Application should stop
  /// pushing buffers.
  virtual void OnEnoughData() = 0;

  /// Event is fired to notify application to change position of the stream.
  /// After receiving this event, application should push buffers from the new
  /// position.
  virtual void OnSeekData(PP_TimeTicks new_position) = 0;

 protected:
  /// The default constructor which creates listener not attached
  /// to the player
  ElementaryStreamListener_Samsung();

 private:
  // Disallow copy and assign
  ElementaryStreamListener_Samsung(const ElementaryStreamListener_Samsung&);
  ElementaryStreamListener_Samsung& operator=(
      const ElementaryStreamListener_Samsung&);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_ELEMENTARY_STREAM_LISTENER_SAMSUNG_H_
