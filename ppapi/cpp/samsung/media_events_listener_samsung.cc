// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/media_events_listener_samsung.h"

#include "ppapi/c/samsung/ppb_media_player_samsung.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/resource.h"
#include "ppapi/cpp/samsung/media_player_samsung.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_MediaPlayer_Samsung_1_0>() {
  return PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_0;
}

void ListenerWrapper_OnTimeUpdate(PP_TimeTicks time,
                                  void* user_data) {
  if (!user_data) return;

  MediaEventsListener_Samsung* listener =
      static_cast<MediaEventsListener_Samsung*>(user_data);
  listener->OnTimeUpdate(time);
}

void ListenerWrapper_OnEnded(void* user_data) {
  if (!user_data) return;

  MediaEventsListener_Samsung* listener =
      static_cast<MediaEventsListener_Samsung*>(user_data);
  listener->OnEnded();
}

void ListenerWrapper_OnError(PP_MediaPlayerError error, void* user_data) {
  if (!user_data) return;

  MediaEventsListener_Samsung* listener =
      static_cast<MediaEventsListener_Samsung*>(user_data);
  listener->OnError(error);
}

const PPP_MediaEventsListener_Samsung* GetListenerWrapper() {
  static const PPP_MediaEventsListener_Samsung listener = {
      &ListenerWrapper_OnTimeUpdate,
      &ListenerWrapper_OnEnded,
      &ListenerWrapper_OnError,
  };

  return &listener;
}

}  // namespace

MediaEventsListener_Samsung::~MediaEventsListener_Samsung() {
  Detach();
}

void MediaEventsListener_Samsung::OnTimeUpdate(PP_TimeTicks /* time */) {
}

void MediaEventsListener_Samsung::OnEnded() {
}

void MediaEventsListener_Samsung::OnError(PP_MediaPlayerError /* error */) {
}

MediaEventsListener_Samsung::MediaEventsListener_Samsung() : player_(0) {
}

MediaEventsListener_Samsung::MediaEventsListener_Samsung(
    MediaPlayer_Samsung* player)
    : player_(0) {
  AttachTo(player);
}

void MediaEventsListener_Samsung::AttachTo(MediaPlayer_Samsung* player) {
  if (!player) {
    Detach();
    return;
  }

  if (player_ == player->pp_resource()) return;

  // TODO(samsung) Is assumption to allow only one listener per application
  //               convenient?
  if (player_) Detach();

  if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
    get_interface<PPB_MediaPlayer_Samsung_1_0>()->SetMediaEventsListener(
        player->pp_resource(), GetListenerWrapper(), this);
  }

  player_ = player->pp_resource();
}

void MediaEventsListener_Samsung::Detach() {
  if (!player_) return;

  if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
    get_interface<PPB_MediaPlayer_Samsung_1_0>()->SetMediaEventsListener(
        player_, NULL, NULL);
  }

  player_ = 0;
}

}  // namespace pp
