// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_ES_DATA_SOURCE_PRIVATE_TIZEN_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_ES_DATA_SOURCE_PRIVATE_TIZEN_H_

#include <player.h>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_es_data_source_private.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_base.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_player_dispatcher.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/platform_context_tizen.h"
#include "ppapi/c/pp_errors.h"

namespace content {

class PepperAudioElementaryStreamPrivateTizen;
class PepperVideoElementaryStreamPrivateTizen;

class PepperESDataSourcePrivateTizen : public PepperESDataSourcePrivate {
 public:
  virtual ~PepperESDataSourcePrivateTizen();

  static scoped_refptr<PepperESDataSourcePrivateTizen> Create();

  // PepperMediaDataSourcePrivate
  PlatformContext& GetContext() override;

  void SourceDetached(PlatformDetachPolicy) override;
  void SourceAttached(const base::Callback<void(int32_t)>& callback) override;
  void GetDuration(
      const base::Callback<void(int32_t, PP_TimeDelta)>& callback) override;

  // PepperESDataSourcePrivate
  void AddAudioStream(
      const base::Callback<
          void(int32_t, scoped_refptr<PepperAudioElementaryStreamPrivate>)>&
          callback) override;
  void AddVideoStream(
      const base::Callback<
          void(int32_t, scoped_refptr<PepperVideoElementaryStreamPrivate>)>&
          callback) override;
  void SetDuration(PP_TimeDelta duration,
                   const base::Callback<void(int32_t)>& callback) override;
  void SetEndOfStream(const base::Callback<void(int32_t)>& callback) override;

 private:
  PepperESDataSourcePrivateTizen();

  int32_t AttachOnPlayerThread(PepperPlayerAdapterInterface* player);
  void DetachOnPlayerThread(PepperPlayerAdapterInterface* player);
  int32_t SetDurationOnPlayerThread(PP_TimeDelta duration,
                                    PepperPlayerAdapterInterface* player);
  int32_t SetEndOfStreamOnPlayerThread(PepperPlayerAdapterInterface* player);
  int32_t ConfigureDrmSessions();
  void ClearDrmSessions();
  void DetachListeners();

  PlatformContext platform_context_;

  scoped_refptr<PepperAudioElementaryStreamPrivateTizen> audio_stream_;
  scoped_refptr<PepperVideoElementaryStreamPrivateTizen> video_stream_;

  PP_TimeDelta duration_;

  scoped_refptr<base::SingleThreadTaskRunner> callback_runner_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_ES_DATA_SOURCE_PRIVATE_TIZEN_H_
