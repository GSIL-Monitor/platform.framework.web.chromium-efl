// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_url_data_source_private_tizen.h"

#include "base/message_loop/message_loop.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_player_dispatcher.h"
#include "ppapi/c/pp_errors.h"

namespace content {

// static
scoped_refptr<PepperURLDataSourcePrivateTizen>
PepperURLDataSourcePrivateTizen::Create(const std::string& url) {
  return new PepperURLDataSourcePrivateTizen(url);
}

PepperURLDataSourcePrivateTizen::PepperURLDataSourcePrivateTizen(
    const std::string& url)
    : url_(url), weak_ptr_factory_(this) {}

PepperURLDataSourcePrivateTizen::~PepperURLDataSourcePrivateTizen() = default;

PepperMediaDataSourcePrivate::PlatformContext&
PepperURLDataSourcePrivateTizen::GetContext() {
  return platform_context_;
}

void PepperURLDataSourcePrivateTizen::SourceAttached(
    const base::Callback<void(int32_t)>& callback) {
  DCHECK(platform_context_.dispatcher());
  PepperTizenPlayerDispatcher::DispatchOperation(
      platform_context_.dispatcher(), FROM_HERE,
      base::Bind(&PepperURLDataSourcePrivateTizen::AttachOnPlayerThread, this),
      callback);
}

void PepperURLDataSourcePrivateTizen::SourceDetached(
    PlatformDetachPolicy clean_up) {
  if (clean_up == PlatformDetachPolicy::kDetachFromPlayer) {
    DCHECK(platform_context_.dispatcher());
    PepperTizenPlayerDispatcher::DispatchOperation(
        platform_context_.dispatcher(), FROM_HERE,
        base::Bind(&PepperURLDataSourcePrivateTizen::DetachOnPlayerThread,
                   this));
  }
}

void PepperURLDataSourcePrivateTizen::GetDuration(
    const base::Callback<void(int32_t, PP_TimeDelta)>& callback) {
  if (platform_context_.dispatcher()) {
    PepperTizenPlayerDispatcher::DispatchOperationWithResult(
        platform_context_.dispatcher(), FROM_HERE,
        base::Bind(&PepperURLDataSourcePrivateTizen::GetDurationOnPlayerThread,
                   this),
        callback);
  } else {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_ERROR_FAILED),
                              static_cast<PP_TimeDelta>(0)));
  }
}

void PepperURLDataSourcePrivateTizen::SetStreamingProperty(
    PP_StreamingProperty property,
    const std::string& data,
    const base::Callback<void(int32_t)>& callback) {
  if (platform_context_.dispatcher()) {
    PepperTizenPlayerDispatcher::DispatchOperation(
        platform_context_.dispatcher(), FROM_HERE,
        base::Bind(&PepperURLDataSourcePrivateTizen::
                       SetStreamingPropertyOnPlayerThread,
                   this, property, data),
        callback);
  } else {
    properties_[property] = data;
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_OK)));
  }
}

void PepperURLDataSourcePrivateTizen::GetStreamingProperty(
    PP_StreamingProperty property,
    const base::Callback<void(int32_t, const std::string&)>& callback) {
  if (platform_context_.dispatcher()) {
    PepperTizenPlayerDispatcher::DispatchOperationWithResult(
        platform_context_.dispatcher(), FROM_HERE,
        base::Bind(&PepperURLDataSourcePrivateTizen::
                       GetStreamingPropertyOnPlayerThread,
                   this, property),
        callback);
  } else {
    auto it = properties_.find(property);
    int32_t pp_error = PP_ERROR_FAILED;
    std::string data;
    if (it != properties_.end()) {
      pp_error = PP_OK;
      data = it->second;
    }

    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, pp_error, data));
  }
}

int32_t PepperURLDataSourcePrivateTizen::AttachOnPlayerThread(
    PepperPlayerAdapterInterface* player) {
  for (const auto e : properties_) {
    int32_t pp_error =
        SetStreamingPropertyOnPlayerThread(e.first, e.second, player);
    if (pp_error != PP_OK)
      return pp_error;
  }
  properties_.clear();

  return player->PrepareURL(url_);
}

void PepperURLDataSourcePrivateTizen::DetachOnPlayerThread(
    PepperPlayerAdapterInterface* player) {
  player->Unprepare();
}

int32_t PepperURLDataSourcePrivateTizen::GetDurationOnPlayerThread(
    PepperPlayerAdapterInterface* player,
    PP_TimeDelta* out_duration) {
  DCHECK(out_duration != nullptr);
  return player->GetDuration(out_duration);
}

int32_t PepperURLDataSourcePrivateTizen::SetStreamingPropertyOnPlayerThread(
    PP_StreamingProperty property,
    const std::string& data,
    PepperPlayerAdapterInterface* player) {
  return player->SetStreamingProperty(property, data);
}

int32_t PepperURLDataSourcePrivateTizen::GetStreamingPropertyOnPlayerThread(
    PP_StreamingProperty property,
    PepperPlayerAdapterInterface* player,
    std::string* property_value) {
  DCHECK(property_value != nullptr);
  return player->GetStreamingProperty(property, property_value);
}

int32_t PepperURLDataSourcePrivateTizen::GetAvailableBitrates(
    PepperPlayerAdapterInterface* player,
    std::string* result) {
  return player->GetAvailableBitrates(result);
}

}  // namespace content
