// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_MEDIA_EVENTS_LISTENER_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_MEDIA_EVENTS_LISTENER_SAMSUNG_H_

#include "ppapi/c/pp_resource.h"
#include "ppapi/c/samsung/ppp_media_player_samsung.h"

/// @file
/// This file defines a <code>MediaEventsListener_Samsung</code> type which
/// allows plugin to receive player or playback related events.
namespace pp {

class MediaPlayer_Samsung;

/// Listener for receiving player or playback related events.
///
/// All event methods will be called on the same thread. If this class was
/// created on a thread with a running message loop, the event methods will run
/// on this thread. Otherwise, they will run on the main thread.
class MediaEventsListener_Samsung {
 public:
  virtual ~MediaEventsListener_Samsung();

  /// Event sent periodically during clip playback and indicates playback
  /// progress.
  ///
  /// Event will be sent at least twice per second.
  ///
  /// @param[in] time current media time.
  virtual void OnTimeUpdate(PP_TimeTicks time);

  /// Played clip has ended.
  virtual void OnEnded();

  /// Error during playback has occurred.
  ///
  /// @param[in] error An error code signalizing type of occurred error.
  virtual void OnError(PP_MediaPlayerError error);

 protected:
  /// The default constructor which creates listener not attached
  /// to the player
  MediaEventsListener_Samsung();

  /// Constructor which creates listener attached to the |player|.
  explicit MediaEventsListener_Samsung(MediaPlayer_Samsung* player);

  /// Attaches listener to the |player|.
  void AttachTo(MediaPlayer_Samsung* player);

 private:
  void Detach();
  PP_Resource player_;

  // Disallow copy and assign
  MediaEventsListener_Samsung(const MediaEventsListener_Samsung&);
  MediaEventsListener_Samsung& operator=(const MediaEventsListener_Samsung&);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_MEDIA_EVENTS_LISTENER_SAMSUNG_H_
