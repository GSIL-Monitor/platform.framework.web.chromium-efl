// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/buffering_listener_samsung.h"

#include "ppapi/c/samsung/ppb_media_player_samsung.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/resource.h"
#include "ppapi/cpp/samsung/media_player_samsung.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_MediaPlayer_Samsung_1_0>() {
  return PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_0;
}

void ListenerWrapper_OnBufferingStart(void* user_data) {
  if (!user_data) return;

  BufferingListener_Samsung* listener =
      static_cast<BufferingListener_Samsung*>(user_data);
  listener->OnBufferingStart();
}

void ListenerWrapper_OnBufferingProgress(uint32_t percent,
                                         void* user_data) {
  if (!user_data) return;

  BufferingListener_Samsung* listener =
      static_cast<BufferingListener_Samsung*>(user_data);
  listener->OnBufferingProgress(percent);
}

void ListenerWrapper_OnBufferingComplete(void* user_data) {
  if (!user_data) return;

  BufferingListener_Samsung* listener =
      static_cast<BufferingListener_Samsung*>(user_data);
  listener->OnBufferingComplete();
}

const PPP_BufferingListener_Samsung* GetListenerWrapper() {
  static const PPP_BufferingListener_Samsung listener = {
      &ListenerWrapper_OnBufferingStart,
      &ListenerWrapper_OnBufferingProgress,
      &ListenerWrapper_OnBufferingComplete,
  };

  return &listener;
}

}  // namespace


BufferingListener_Samsung::~BufferingListener_Samsung() {
  Detach();
}

void BufferingListener_Samsung::OnBufferingStart() {
}

void BufferingListener_Samsung::OnBufferingProgress(uint32_t /* percent */) {
}

void BufferingListener_Samsung::OnBufferingComplete() {
}

BufferingListener_Samsung::BufferingListener_Samsung() : player_(0) {
}

BufferingListener_Samsung::BufferingListener_Samsung(
    MediaPlayer_Samsung* player)
    : player_(0) {
  AttachTo(player);
}

void BufferingListener_Samsung::AttachTo(MediaPlayer_Samsung* player) {
  if (!player) {
    Detach();
    return;
  }

  if (player_ == player->pp_resource()) return;

  // TODO(samsung) Is assumption to allow only one listener per application
  //               convenient?
  if (player_) Detach();

  if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
    get_interface<PPB_MediaPlayer_Samsung_1_0>()->SetBufferingListener(
        player->pp_resource(), GetListenerWrapper(), this);
  }

  player_ = player->pp_resource();
}

void BufferingListener_Samsung::Detach() {
  if (!player_) return;

  if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
    get_interface<PPB_MediaPlayer_Samsung_1_0>()->SetBufferingListener(
        player_, NULL, NULL);
  }

  player_ = 0;
}

}  // namespace pp
