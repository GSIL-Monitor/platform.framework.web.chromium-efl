// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/subtitle_listener_samsung.h"

#include "ppapi/c/samsung/ppb_media_player_samsung.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/resource.h"
#include "ppapi/cpp/samsung/media_player_samsung.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_MediaPlayer_Samsung_1_0>() {
  return PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_0;
}

void ListenerWrapper_OnShowSubtitle(PP_TimeDelta duration,
    const char* text,
    void* user_data) {
  if (!user_data) return;

  SubtitleListener_Samsung* listener =
      static_cast<SubtitleListener_Samsung*>(user_data);
  listener->OnShowSubtitle(duration, text);
}

const PPP_SubtitleListener_Samsung* GetListenerWrapper() {
  static const PPP_SubtitleListener_Samsung listener = {
      &ListenerWrapper_OnShowSubtitle,
  };
  return &listener;
}

}  // namespace

SubtitleListener_Samsung::~SubtitleListener_Samsung() {
  Detach();
}

SubtitleListener_Samsung::SubtitleListener_Samsung() : player_(0) {
}

SubtitleListener_Samsung::SubtitleListener_Samsung(MediaPlayer_Samsung* player)
    : player_(0) {
  AttachTo(player);
}

void SubtitleListener_Samsung::AttachTo(MediaPlayer_Samsung* player) {
  if (!player) {
    Detach();
    return;
  }

  if (player_ == player->pp_resource()) return;

  // TODO(samsung): Is assumption to allow only one listener per application
  //                convenient?
  if (player_) Detach();

  if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
    get_interface<PPB_MediaPlayer_Samsung_1_0>()->SetSubtitleListener(
        player->pp_resource(), GetListenerWrapper(), this);
  }

  player_ = player->pp_resource();
}

void SubtitleListener_Samsung::Detach() {
  if (!player_) return;

  if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
    get_interface<PPB_MediaPlayer_Samsung_1_0>()->SetSubtitleListener(
        player_, NULL, NULL);
  }

  player_ = 0;
}

}  // namespace pp
