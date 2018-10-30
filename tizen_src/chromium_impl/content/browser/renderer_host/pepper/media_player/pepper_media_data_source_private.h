// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_DATA_SOURCE_PRIVATE_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_DATA_SOURCE_PRIVATE_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "ppapi/c/pp_time.h"

namespace content {

// Platform depended part of Media Data Source PPAPI implementation,
// see ppapi/api/samsung/ppb_media_data_source_samsung.idl.

class PepperMediaDataSourcePrivate
    : public base::RefCountedThreadSafe<PepperMediaDataSourcePrivate> {
 public:
  // Structure containing a platform-specific context related to the data
  // source which needed to be publicly available for an implementation.
  // Definition of this structure will be provided by a plafrom-specific
  // implementation selected at compile time.
  class PlatformContext;
  virtual PlatformContext& GetContext() = 0;

  // Policy deciding whether detaching this data source should trigger
  // detaching from platform player.
  // Not detaching from plaform player may happen mostly during whole
  // Player Private object destruction
  enum class PlatformDetachPolicy { kDetachFromPlayer, kRetainPlayer };

  // Data Source has been detached from the player.
  virtual void SourceDetached() {
    SourceDetached(PlatformDetachPolicy::kDetachFromPlayer);
  }

  // Data Source has been detached from the player.
  // Perform platform player clean-up if necessary
  virtual void SourceDetached(PlatformDetachPolicy) = 0;

  // Data Source has been attached to the player.
  virtual void SourceAttached(
      const base::Callback<void(int32_t)>& callback) = 0;

  virtual void GetDuration(
      const base::Callback<void(int32_t, PP_TimeDelta)>& callback) = 0;

 protected:
  friend class base::RefCountedThreadSafe<PepperMediaDataSourcePrivate>;
  virtual ~PepperMediaDataSourcePrivate() {}
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_PEPPER_MEDIA_DATA_SOURCE_PRIVATE_H_
