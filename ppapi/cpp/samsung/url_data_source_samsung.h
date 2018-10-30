// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_URL_DATA_SOURCE_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_URL_DATA_SOURCE_SAMSUNG_H_

#include <string>

#include "ppapi/c/samsung/ppb_media_data_source_samsung.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/resource.h"
#include "ppapi/cpp/samsung/media_data_source_samsung.h"
#include "ppapi/cpp/var.h"

/// @file
/// This file defines the <code>URLDataSource_Samsung<code> type,
/// which represents data source which can feed media clip data from
/// URL provided.
///
/// Part of Pepper Media Player interfaces (Samsung's extension).
namespace pp {

class InstanceHandle;

/// Class representing Data Source handling media passed as URLs.
///
/// After creation data source can be attached to the
/// <code>MediaPlayer_Samsung</code>. When attaching to the player passed URL
/// is validated and buffering starts.
class URLDataSource_Samsung : public MediaDataSource_Samsung {
 public:
  /// Creates Data Source which is able to download data form media clip
  /// from |url| provided.
  URLDataSource_Samsung(const InstanceHandle& instance,
                        const std::string& url);

  URLDataSource_Samsung(const URLDataSource_Samsung& other);

  explicit URLDataSource_Samsung(PP_Resource resource);
  URLDataSource_Samsung(PassRef, PP_Resource resource);

  URLDataSource_Samsung& operator=(const URLDataSource_Samsung& other);

  virtual ~URLDataSource_Samsung();

  /// Gets the value for specific Feature in the HTTP, MMS & Streaming Engine
  /// (Smooth Streaming, HLS, DASH, DivX Plus Streaming, Widevine).
  ///
  /// @param[in] type A <code>PP_StreamingProperty</code> to gett
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code>
  /// to be called upon completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t GetStreamingProperty(
      PP_StreamingProperty type,
      const CompletionCallbackWithOutput<pp::Var>& callback);

  /// Sets the value for specific Feature in the HTTP, MMS & Streaming Engine
  /// (Smooth Streaming, HLS, DASH, DivX Plus Streaming, Widevine).
  ///
  /// When special setting is required in Streaming Engine for the
  /// Start Bitrate setting or specific Content Protection (CP),
  /// the CUSTOM_MESSAGE Property can be set.
  ///
  /// @param[in] type A <code>PP_StreamingProperty</code> to set
  /// @param[in] value New value of the property, see desciprion of
  /// <code>PP_StreamingProperty</code> what is requested format.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t SetStreamingProperty(
      PP_StreamingProperty type,
      const Var& value,
      const CompletionCallback& callback);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_URL_DATA_SOURCE_SAMSUNG_H_
