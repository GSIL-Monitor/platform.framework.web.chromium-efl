// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_URL_DATA_SOURCE_PRIVATE_TIZEN_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_URL_DATA_SOURCE_PRIVATE_TIZEN_H_

#include <string>
#include <unordered_map>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_url_data_source_private.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_adapter_base.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/platform_context_tizen.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"

namespace content {

class PepperURLDataSourcePrivateTizen : public PepperURLDataSourcePrivate {
 public:
  static scoped_refptr<PepperURLDataSourcePrivateTizen> Create(
      const std::string& url);

  // PepperMediaDataSourcePrivate
  PlatformContext& GetContext() override;

  void SourceAttached(const base::Callback<void(int32_t)>& callback) override;
  void SourceDetached(PlatformDetachPolicy) override;
  void GetDuration(
      const base::Callback<void(int32_t, PP_TimeDelta)>& callback) override;

  void SetStreamingProperty(PP_StreamingProperty,
                            const std::string&,
                            const base::Callback<void(int32_t)>&) override;
  void GetStreamingProperty(
      PP_StreamingProperty,
      const base::Callback<void(int32_t, const std::string&)>&) override;

 protected:
  friend class base::RefCountedThreadSafe<PepperURLDataSourcePrivateTizen>;
  virtual ~PepperURLDataSourcePrivateTizen();

 private:
  explicit PepperURLDataSourcePrivateTizen(const std::string& url);

  int32_t AttachOnPlayerThread(PepperPlayerAdapterInterface* player);
  void DetachOnPlayerThread(PepperPlayerAdapterInterface* player);
  int32_t GetDurationOnPlayerThread(PepperPlayerAdapterInterface* player,
                                    PP_TimeDelta* time);
  int32_t SetStreamingPropertyOnPlayerThread(
      PP_StreamingProperty property,
      const std::string& data,
      PepperPlayerAdapterInterface* player);
  int32_t GetStreamingPropertyOnPlayerThread(
      PP_StreamingProperty property,
      PepperPlayerAdapterInterface* player,
      std::string* property_value);

  int32_t GetAvailableBitrates(PepperPlayerAdapterInterface* player,
                               std::string* bitrates);

  PlatformContext platform_context_;
  const std::string url_;

  struct StreamingPropertyHash {
    size_t operator()(PP_StreamingProperty val) const {
      return static_cast<std::size_t>(val);
    }
  };

  std::unordered_map<PP_StreamingProperty, std::string, StreamingPropertyHash>
      properties_;

  base::WeakPtrFactory<PepperURLDataSourcePrivateTizen> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_URL_DATA_SOURCE_PRIVATE_TIZEN_H_
