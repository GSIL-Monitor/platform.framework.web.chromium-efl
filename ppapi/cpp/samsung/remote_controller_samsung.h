// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_REMOTE_CONTROLLER_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_REMOTE_CONTROLLER_SAMSUNG_H_

#include <string>
#include <vector>

#include "ppapi/c/samsung/ppb_remote_controller_samsung.h"
#include "ppapi/cpp/instance_handle.h"

/// @file
/// This file defines API related to TV Remote Controller

namespace pp {

class RemoteControllerSamsung {
 public:
  /// A constructor for creating a <code>RemoteControllerSamsung</code> class
  /// for given instance of a module.
  explicit RemoteControllerSamsung(const InstanceHandle& instance);

  /// RegisterKeys() function registers given key arrays to be grabbed by
  /// the application/widget containing pepper plugin calling this method.
  ///
  /// <strong>Note:</strong>
  /// After registering for grabbing keys, events related to that key
  /// will be delivered directly to the application/widget.
  ///
  /// <strong>Note:</strong>
  /// For some embedders, we can`t tell if key that we try to register have
  /// failed because it have been already registered. So if at least one key
  /// have been successfully processed, we assume that other keys that failed,
  /// have been already registered before this call.
  ///
  /// @param[in] key_count A number of keys which will be grabbed.
  /// @param[in] keys An array containing list of keys which should be grabbed.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>.
  /// Returns <code>PP_ERROR_BADARGUMENT</code> if <code>key_count</code> is
  /// equal 0 or one of <code>keys</code> is not supported anymore.
  /// Returns <code>PP_ERROR_NOTSUPPORTED</code> if currently launched embedder
  /// doesn`t support key registering.
  /// Returns <code>PP_ERROR_FAILED</code> if registering all <code>keys</code>
  /// have failed.
  /// Returns <code>PP_OK</code> if at lest one key from <code>keys</code> have
  /// been registered.
  int32_t RegisterKeys(uint32_t key_count, const char* keys[]);

  /// RegisterKeys() function registers given key arrays to be grabbed by
  /// the application/widget containing pepper plugin calling this method.
  ///
  /// <strong>Note:</strong>
  /// After registering for grabbing keys, events related to that key
  /// will be delivered directly to the application/widget.
  ///
  /// <strong>Note:</strong>
  /// For some embedders, we can`t tell if key that we try to register have
  /// failed because it have been already registered. So if at least one key
  /// have been successfully processed, we assume that other keys that failed,
  /// have been already registered before this call.
  ///
  /// @param[in] keys A vector containing list of keys which should be grabbed.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>.
  /// Returns <code>PP_ERROR_BADARGUMENT</code> if <code>key_count</code> is
  /// equal 0 or one of <code>keys</code> is not supported anymore.
  /// Returns <code>PP_ERROR_NOTSUPPORTED</code> if currently launched embedder
  /// doesn`t support key registering.
  /// Returns <code>PP_ERROR_FAILED</code> if registering all <code>keys</code>
  /// have failed.
  /// Returns <code>PP_OK</code> if at lest one key from <code>keys</code> have
  /// been registered.
  int32_t RegisterKeys(const std::vector<std::string>& keys);

  /// UnregisterKeys() function unregisters given key arrays from being grabbed
  /// by the application/widget containing pepper plugin calling this method.
  ///
  /// <strong>Note:</strong>
  /// For some embedders, we can`t tell if key that we try to unregister have
  /// failed because it have been already unregistered. So if at least one key
  /// have been successfully processed, we assume that other keys that failed,
  /// have been already unregistered before this call.
  ///
  /// @param[in] key_count A number of keys which will be grabbed.
  /// @param[in] keys An array containing list of keys which should be grabbed.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>.
  /// Returns <code>PP_ERROR_BADARGUMENT</code> if <code>key_count</code> is
  /// equal 0 or one of <code>keys</code> is not supported anymore.
  /// Returns <code>PP_ERROR_NOTSUPPORTED</code> if currently launched embedder
  /// doesn`t support key registering.
  /// Returns <code>PP_ERROR_FAILED</code> if registering all <code>keys</code>
  /// have failed.
  /// Returns <code>PP_OK</code> if at lest one key from <code>keys</code> have
  /// been registered.
  int32_t UnRegisterKeys(uint32_t key_count, const char* keys[]);

  /// UnregisterKeys() function unregisters given key arrays from being grabbed
  /// by the application/widget containing pepper plugin calling this method.
  ///
  /// <strong>Note:</strong>
  /// For some embedders, we can`t tell if key that we try to unregister have
  /// failed because it have been already unregistered. So if at least one key
  /// have been successfully processed, we assume that other keys that failed,
  /// have been already unregistered before this call.
  ///
  /// @param[in] keys A vector containing list of keys which should be grabbed.
  ///
  /// @return An int32_t containing an error code from <code>pp_errors.h</code>.
  /// Returns <code>PP_ERROR_BADARGUMENT</code> if <code>key_count</code> is
  /// equal 0 or one of <code>keys</code> is not supported anymore.
  /// Returns <code>PP_ERROR_NOTSUPPORTED</code> if currently launched embedder
  /// doesn`t support key registering.
  /// Returns <code>PP_ERROR_FAILED</code> if registering all <code>keys</code>
  /// have failed.
  /// Returns <code>PP_OK</code> if at lest one key from <code>keys</code> have
  /// been registered.
  int32_t UnRegisterKeys(const std::vector<std::string>& keys);

 private:
  InstanceHandle instance_;
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_REMOTE_CONTROLLER_SAMSUNG_H_
