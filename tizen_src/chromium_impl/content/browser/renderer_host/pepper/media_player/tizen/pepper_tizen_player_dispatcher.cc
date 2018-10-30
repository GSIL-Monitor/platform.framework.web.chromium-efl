// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_player_dispatcher.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/elementary_stream_listener_private_wrapper.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/samsung/pp_media_common_samsung.h"

namespace content {

namespace {
const char kPlayerThreadName[] = "PepperPlayerThread";
const char kPlayerAppendPacketThreadName[] = "PepperPlayerAppendPacketThread";
}  // anonymous namespace

// static
std::unique_ptr<PepperTizenPlayerDispatcher>
PepperTizenPlayerDispatcher::Create(
    std::unique_ptr<PepperPlayerAdapterInterface> player) {
  std::unique_ptr<PepperTizenPlayerDispatcher> dispatcher{
      new PepperTizenPlayerDispatcher(std::move(player))};
  if (dispatcher->Initialize())
    return dispatcher;
  return nullptr;
}

PepperTizenPlayerDispatcher::PepperTizenPlayerDispatcher(
    std::unique_ptr<PepperPlayerAdapterInterface> player)
    : player_thread_(kPlayerThreadName),
      append_packet_thread_(kPlayerAppendPacketThreadName),
      player_(std::move(player)) {}

PepperTizenPlayerDispatcher::~PepperTizenPlayerDispatcher() {
  Destroy();
}

bool PepperTizenPlayerDispatcher::Initialize() {
  player_thread_.Start();
  append_packet_thread_.Start();
  return player_->Initialize() == PP_OK;
}

void PepperTizenPlayerDispatcher::Destroy() {
  DestroyPlayer();
  drm_manager_.reset();
}

bool PepperTizenPlayerDispatcher::DestroyPlayer() {
  // Clear dispatcher to make sure that all API calls are done
  // before destroying platform player.
  player_thread_.Stop();
  append_packet_thread_.Stop();
  player_->Reset();
  return true;
}

bool PepperTizenPlayerDispatcher::ResetPlayer() {
  return DestroyPlayer() && Initialize();
}

}  // namespace content
