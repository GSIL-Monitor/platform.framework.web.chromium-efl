// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_DRM_LISTENER_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_DRM_LISTENER_SAMSUNG_H_

#include "ppapi/c/pp_resource.h"
#include "ppapi/c/samsung/ppp_media_player_samsung.h"

/// @file
/// This file defines a <code>DRMListener_Samsung</code> type which allows
/// plugin to receive DRM events.
namespace pp {

class MediaPlayer_Samsung;

/// Listener for receiving DRM related events.
///
/// All event methods will be called on the same thread. If this class was
/// created on a thread with a running message loop, the event methods will run
/// on this thread. Otherwise, they will run on the main thread.
class DRMListener_Samsung {
 public:
  virtual ~DRMListener_Samsung();

  /// During parsing media container encrypted track was found.
  ///
  /// @param[in] drm_type A type of DRM system
  /// @param[in] init_data_size Size in bytes of |init_data| buffer.
  /// @param[in] init_data A pointer to the buffer containing DRM specific
  /// initialization data.
  virtual void OnInitdataLoaded(
      PP_MediaPlayerDRMType drm_type,
      uint32_t init_data_size,
      const void* init_data);

  /// Decryption license needs to be requested from the server and
  /// provided to the player.
  ///
  /// @param[in] request_size Size in bytes of |request| buffer.
  /// @param[in] request A pointer to the buffer containing DRM specific
  /// request.
  virtual void OnLicenseRequest(uint32_t request_size, const void* request);

 protected:
  /// The default constructor which creates listener not attached
  /// to the player
  DRMListener_Samsung();

  /// Constructor which creates listener attached to the |player|.
  explicit DRMListener_Samsung(MediaPlayer_Samsung* player);

  /// Attaches listener to the |player|.
  void AttachTo(MediaPlayer_Samsung* player);

 private:
  void Detach();
  PP_Resource player_;

  // Disallow copy and assign
  DRMListener_Samsung(const DRMListener_Samsung&);
  DRMListener_Samsung& operator=(const DRMListener_Samsung&);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_DRM_LISTENER_SAMSUNG_H_
