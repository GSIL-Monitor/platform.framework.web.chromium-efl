// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_BUFFERING_LISTENER_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_BUFFERING_LISTENER_SAMSUNG_H_

#include "ppapi/c/pp_resource.h"
#include "ppapi/c/samsung/ppp_media_player_samsung.h"

/// @file
/// This file defines a <code>BufferingListener_Samsung</code> type which
/// allows plugin to receive initial media buffering events.
namespace pp {

class MediaPlayer_Samsung;

/// Listener for receiving initial media buffering related events, sent
/// before playback can be started.
///
/// Those event can be used by the application to show buffering progress bar
/// to the user.
///
/// Those events are sent only when media buffering is managed by the player
/// implementation (see <code>URLDataSource_Samsung</code>), not by the user
/// (see <code>ESDataSource_Samsung</code>).
///
/// All event methods will be called on the same thread. If this class was
/// created on a thread with a running message loop, the event methods will run
/// on this thread. Otherwise, they will run on the main thread.
class BufferingListener_Samsung {
 public:
  virtual ~BufferingListener_Samsung();

  /// Initial media buffering has been started by the player.
  virtual void OnBufferingStart();

  /// Initial buffering in progress.
  ///
  ///  @param[in] percent Indicates how much of the initial data has been
  ///  buffered by the player.
  virtual void OnBufferingProgress(uint32_t percent);

  /// Initial media buffering has been completed by the player, after that
  /// event playback might be started.
  virtual void OnBufferingComplete();

 protected:
  /// The default constructor which creates listener not attached
  /// to the player
  BufferingListener_Samsung();

  /// Constructor which creates listener attached to the |player|.
  explicit BufferingListener_Samsung(MediaPlayer_Samsung* player);

  /// Attaches listener to the |player|.
  void AttachTo(MediaPlayer_Samsung* player);

 private:
  void Detach();
  PP_Resource player_;

  // Disallow copy and assign
  BufferingListener_Samsung(const BufferingListener_Samsung&);
  BufferingListener_Samsung& operator=(const BufferingListener_Samsung&);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_BUFFERING_LISTENER_SAMSUNG_H_
