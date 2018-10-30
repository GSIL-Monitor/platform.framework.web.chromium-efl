// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_SUBTITLE_LISTENER_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_SUBTITLE_LISTENER_SAMSUNG_H_

#include "ppapi/c/pp_resource.h"
#include "ppapi/c/samsung/ppp_media_player_samsung.h"

/// @file
/// This file defines a <code>SubtitleListener_Samsung</code> type which allows
/// plugin to receive events that indicate a subtitle should be displayed at
/// the current playback position.
namespace pp {

class MediaPlayer_Samsung;

/// Listener for receiving subtitle updates provided by the player's internal
/// subtitle parser. This listener will be notified every time active and
/// visible text track contains a subtitle that should be displayed at the
/// current playback position.
///
/// All event methods will be called on the same thread. If this class was
/// created on a thread with a running message loop, the event methods will run
/// on this thread. Otherwise, they will run on the main thread.
class SubtitleListener_Samsung {
 public:
  virtual ~SubtitleListener_Samsung();

  /// Event sent when a subtitle should be displayed.
  ///
  /// @param[in] duration Duration for which the text should be displayed.
  /// @param[in] text The UTF-8 encoded string that contains a subtitle text
  /// that should be displayed. Please note text encoding will be UTF-8
  /// independently from the source subtitle file encoding.
  virtual void OnShowSubtitle(PP_TimeDelta duration, const char* text) = 0;

 protected:
  /// The default constructor which creates listener not attached
  /// to the player
  SubtitleListener_Samsung();

  /// Constructor which creates listener attached to the |player|.
  explicit SubtitleListener_Samsung(MediaPlayer_Samsung* player);

  /// Attaches listener to the |player|.
  void AttachTo(MediaPlayer_Samsung* player);

 private:
  void Detach();
  PP_Resource player_;

  // Disallow copy and assign
  SubtitleListener_Samsung(const SubtitleListener_Samsung&);
  SubtitleListener_Samsung& operator=(
      const SubtitleListener_Samsung&);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_SUBTITLE_LISTENER_SAMSUNG_H_
