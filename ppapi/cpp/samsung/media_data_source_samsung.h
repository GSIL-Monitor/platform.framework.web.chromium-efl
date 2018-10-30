// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_MEDIA_DATA_SOURCE_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_MEDIA_DATA_SOURCE_SAMSUNG_H_

#include "ppapi/cpp/resource.h"

/// @file
/// This file defines the abstract <code>MediaDataSource_Samsung<code> type,
/// which provide basic class for data sources able to feed
/// <code>MediaPlayer_Samsung</code> with media clip data.
///
/// Part of Pepper Media Player interfaces (Samsung's extension).
namespace pp {

class InstanceHandle;

/// Abstract Media Data Source being base of data sources able to feed
/// <code>MediaPlayer_Samsung</code> with media clip data.
class MediaDataSource_Samsung : public Resource {
 public:
  MediaDataSource_Samsung(const MediaDataSource_Samsung& other);
  MediaDataSource_Samsung& operator=(const MediaDataSource_Samsung& other);
  virtual ~MediaDataSource_Samsung();

 protected:
  MediaDataSource_Samsung();
  explicit MediaDataSource_Samsung(const Resource& data_source);
  explicit MediaDataSource_Samsung(PP_Resource resource);
  MediaDataSource_Samsung(PassRef, PP_Resource resource);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_MEDIA_DATA_SOURCE_SAMSUNG_H_
