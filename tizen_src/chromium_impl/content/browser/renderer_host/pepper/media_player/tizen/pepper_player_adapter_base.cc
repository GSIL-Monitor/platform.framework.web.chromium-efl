// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_base.h"

#include "ppapi/c/pp_errors.h"

#include "base/logging.h"

namespace content {

PepperPlayerAdapterBase::PepperPlayerAdapterBase()
    : elementary_stream_listeners_(PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_MAX) {}

PepperPlayerAdapterBase::~PepperPlayerAdapterBase() = default;

int32_t PepperPlayerAdapterBase::Reset() {
  ResetCallbackWrappers();
  return PP_OK;
}

void PepperPlayerAdapterBase::ResetCallbackWrappers() {
  for (auto& callback : callback_wrappers_)
    callback.reset();
}

ElementaryStreamListenerPrivateWrapper*
PepperPlayerAdapterBase::ListenerWrapper(
    PP_ElementaryStream_Type_Samsung type) {
  if (!IsValidStreamType(type))
    return nullptr;

  if (!elementary_stream_listeners_[type])
    elementary_stream_listeners_[type].reset(
        new ElementaryStreamListenerPrivateWrapper());

  return elementary_stream_listeners_[type].get();
}

bool PepperPlayerAdapterBase::IsValidStreamType(
    PP_ElementaryStream_Type_Samsung type) {
  return type >= 0 && type < PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_MAX;
}

void PepperPlayerAdapterBase::SetListener(
    PP_ElementaryStream_Type_Samsung type,
    base::WeakPtr<ElementaryStreamListenerPrivate> listener) {
  auto wrapper = ListenerWrapper(type);
  if (!wrapper)
    return;

  wrapper->SetListener(listener);
  RegisterMediaCallbacks(type);
}

void PepperPlayerAdapterBase::RemoveListener(
    PP_ElementaryStream_Type_Samsung type,
    ElementaryStreamListenerPrivate* listener) {
  if (!IsValidStreamType(type))
    return;

  auto wrapper = elementary_stream_listeners_[type].get();
  if (!wrapper)
    return;

  wrapper->RemoveListener(listener);
}

int32_t PepperPlayerAdapterBase::NotifyCurrentTimeListener() {
  if (!GetMediaEventsListener())
    return PP_ERROR_BADRESOURCE;

  PP_TimeTicks current_time;
  int32_t status = GetCurrentTime(&current_time);
  if (status != PP_OK)
    return status;

  if (media_events_listener_)
    media_events_listener_->OnTimeUpdate(current_time);

  return PP_OK;
}

int32_t PepperPlayerAdapterBase::SetDisplayRectUpdateCallback(
    const base::Callback<void()>& callback) {
  SetCallback<PepperPlayerCallbackType::DisplayRectUpdateCallback>(callback);
  return PP_OK;
}

void PepperPlayerAdapterBase::SetBufferingListener(
    BufferingListenerPrivate* listener) {
  bool is_set = !!buffering_listener_;
  bool will_set = !!listener;
  if (is_set != will_set) {
    RegisterToBufferingEvents(will_set);
  }
  buffering_listener_.reset(listener);
}

void PepperPlayerAdapterBase::SetMediaEventsListener(
    MediaEventsListenerPrivate* listener) {
  bool is_set = !!media_events_listener_;
  bool will_set = !!listener;
  if (is_set != will_set) {
    RegisterToMediaEvents(will_set);
  }
  media_events_listener_.reset(listener);
}

void PepperPlayerAdapterBase::SetSubtitleListener(
    SubtitleListenerPrivate* listener) {
  bool is_set = !!subtitle_listener_;
  bool will_set = !!listener;
  if (is_set != will_set) {
    RegisterToSubtitleEvents(will_set);
  }
  subtitle_listener_.reset(listener);
}

}  // namespace content
