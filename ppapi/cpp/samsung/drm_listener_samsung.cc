// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/drm_listener_samsung.h"

#include "ppapi/c/samsung/ppb_media_player_samsung.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/resource.h"
#include "ppapi/cpp/samsung/media_player_samsung.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_MediaPlayer_Samsung_1_0>() {
  return PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_0;
}

void ListenerWrapper_OnInitdataLoaded(PP_MediaPlayerDRMType drm_type,
                                      uint32_t init_data_size,
                                      const void* init_data,
                                      void* user_data) {
  if (!user_data) return;

  DRMListener_Samsung* listener =
      static_cast<DRMListener_Samsung*>(user_data);
  listener->OnInitdataLoaded(drm_type, init_data_size, init_data);
}

void ListenerWrapper_OnLicenseRequest(uint32_t request_size,
                                      const void* request,
                                      void* user_data) {
  if (!user_data) return;

  DRMListener_Samsung* listener =
      static_cast<DRMListener_Samsung*>(user_data);
  listener->OnLicenseRequest(request_size, request);
}

const PPP_DRMListener_Samsung* GetListenerWrapper() {
  static const PPP_DRMListener_Samsung listener = {
      &ListenerWrapper_OnInitdataLoaded,
      &ListenerWrapper_OnLicenseRequest,
  };

  return &listener;
}

}  // namespace

DRMListener_Samsung::~DRMListener_Samsung() {
  Detach();
}

void DRMListener_Samsung::OnInitdataLoaded(
    PP_MediaPlayerDRMType /* drm_type */,
    uint32_t /* init_data_size */,
    const void* /* init_data */) {
}

void DRMListener_Samsung::OnLicenseRequest(uint32_t /* request_size */,
                                           const void* /* request */) {
}

DRMListener_Samsung::DRMListener_Samsung() : player_(0) {
}

DRMListener_Samsung::DRMListener_Samsung(MediaPlayer_Samsung* player)
    : player_(0) {
  AttachTo(player);
}

void DRMListener_Samsung::AttachTo(MediaPlayer_Samsung* player) {
  if (!player) {
    Detach();
    return;
  }

  if (player_ == player->pp_resource()) return;

  // TODO(samsung): Is assumption to allow only one listener per application
  //                convenient?
  if (player_) Detach();

  if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
    get_interface<PPB_MediaPlayer_Samsung_1_0>()->SetDRMListener(
        player->pp_resource(), GetListenerWrapper(), this);
  }

  player_ = player->pp_resource();
}


void DRMListener_Samsung::Detach() {
  if (!player_) return;

  if (has_interface<PPB_MediaPlayer_Samsung_1_0>()) {
    get_interface<PPB_MediaPlayer_Samsung_1_0>()->SetDRMListener(
        player_, NULL, NULL);
  }

  player_ = 0;
}

}  // namespace pp
