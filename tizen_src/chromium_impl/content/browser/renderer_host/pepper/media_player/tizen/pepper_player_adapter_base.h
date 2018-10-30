// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_PLAYER_ADAPTER_BASE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_PLAYER_ADAPTER_BASE_H_

#include <array>

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_interface.h"

namespace content {

class PepperPlayerAdapterBase : public PepperPlayerAdapterInterface {
 public:
  ~PepperPlayerAdapterBase() override;

  int32_t Reset() override;
  void SetListener(
      PP_ElementaryStream_Type_Samsung type,
      base::WeakPtr<ElementaryStreamListenerPrivate> listener) override;
  void RemoveListener(PP_ElementaryStream_Type_Samsung type,
                      ElementaryStreamListenerPrivate* listener) override;
  void SetMediaEventsListener(MediaEventsListenerPrivate* listener) override;
  void SetSubtitleListener(SubtitleListenerPrivate* listener) override;
  void SetBufferingListener(BufferingListenerPrivate* listener) override;
  int32_t NotifyCurrentTimeListener() override;

  int32_t SetDisplayRectUpdateCallback(
      const base::Callback<void()>& callback) override;

 protected:
  PepperPlayerAdapterBase();

  enum class PepperPlayerCallbackType {
    DisplayRectUpdateCallback,
    SeekCompletedCallback,
    ErrorCallback,
    CallbacksCount
  };

  template <PepperPlayerCallbackType type>
  struct PepperCallbackTraits {
    // default callback type
    typedef base::Closure CallbackType;
  };

  class PepperPlayerCallbackWrapperBase {
   public:
    virtual ~PepperPlayerCallbackWrapperBase() {}
  };

  template <typename T>
  class PepperPlayerCallbackWrapper : public PepperPlayerCallbackWrapperBase {
   public:
    explicit PepperPlayerCallbackWrapper(const T& callback)
        : callback_(callback) {}
    const T& GetCallback() const { return callback_; }

   private:
    T callback_;
  };

  template <PepperPlayerCallbackType type, typename... Args>
  void RunCallback(Args&&... args);

  template <PepperPlayerCallbackType type, typename... Args>
  void RunAndClearCallback(Args&&... args);

  template <PepperPlayerCallbackType type>
  void SetCallback(
      const typename PepperCallbackTraits<type>::CallbackType& callback);

  template <PepperPlayerCallbackType type>
  void ClearCallback();

  virtual void RegisterMediaCallbacks(
      PP_ElementaryStream_Type_Samsung type) = 0;

  ElementaryStreamListenerPrivateWrapper* ListenerWrapper(
      PP_ElementaryStream_Type_Samsung type);

  BufferingListenerPrivate* GetBufferingListener() {
    return buffering_listener_.get();
  }

  MediaEventsListenerPrivate* GetMediaEventsListener() {
    return media_events_listener_.get();
  }

  SubtitleListenerPrivate* GetSubtitleListener() {
    return subtitle_listener_.get();
  }

  virtual void RegisterToBufferingEvents(bool should_register) = 0;
  virtual void RegisterToMediaEvents(bool should_register) = 0;
  virtual void RegisterToSubtitleEvents(bool should_register) = 0;

 private:
  void ResetCallbackWrappers();

  bool IsValidStreamType(PP_ElementaryStream_Type_Samsung type);

  std::vector<std::unique_ptr<ElementaryStreamListenerPrivateWrapper>>
      elementary_stream_listeners_;

  std::unique_ptr<MediaEventsListenerPrivate> media_events_listener_;
  std::unique_ptr<SubtitleListenerPrivate> subtitle_listener_;
  std::unique_ptr<BufferingListenerPrivate> buffering_listener_;

  std::array<std::unique_ptr<PepperPlayerCallbackWrapperBase>,
             static_cast<int>(PepperPlayerCallbackType::CallbacksCount)>
      callback_wrappers_;

  DISALLOW_COPY_AND_ASSIGN(PepperPlayerAdapterBase);
};

template <>
struct PepperPlayerAdapterBase::PepperCallbackTraits<
    PepperPlayerAdapterBase::PepperPlayerCallbackType::ErrorCallback> {
  typedef base::Callback<void(PP_MediaPlayerError)> CallbackType;
};

template <PepperPlayerAdapterBase::PepperPlayerCallbackType type,
          typename... Args>
inline void PepperPlayerAdapterBase::RunCallback(Args&&... args) {
  auto callback_wrapper = static_cast<PepperPlayerCallbackWrapper<
      typename PepperCallbackTraits<type>::CallbackType>*>(
      callback_wrappers_[static_cast<int>(type)].get());

  if (!callback_wrapper) {
    LOG(ERROR) << "Callback of type " << static_cast<int>(type)
               << " is not registered";
    return;
  }

  auto callback = callback_wrapper->GetCallback();

  if (callback.is_null()) {
    LOG(ERROR) << "Callback of type " << static_cast<int>(type) << " is null";
    return;
  }

  callback.Run(std::forward<Args>(args)...);
}

template <PepperPlayerAdapterBase::PepperPlayerCallbackType type,
          typename... Args>
inline void PepperPlayerAdapterBase::RunAndClearCallback(Args&&... args) {
  RunCallback<type>(args...);
  ClearCallback<type>();
}

template <PepperPlayerAdapterBase::PepperPlayerCallbackType type>
inline void PepperPlayerAdapterBase::SetCallback(
    const typename PepperCallbackTraits<type>::CallbackType& callback) {
  callback_wrappers_[static_cast<int>(type)].reset(
      new PepperPlayerCallbackWrapper<
          typename PepperCallbackTraits<type>::CallbackType>(callback));
}

template <PepperPlayerAdapterBase::PepperPlayerCallbackType type>
inline void PepperPlayerAdapterBase::ClearCallback() {
  callback_wrappers_[static_cast<int>(type)].reset(nullptr);
}

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_PLAYER_ADAPTER_BASE_H_
