// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/media_data_source_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/instance_handle.h"

namespace pp {

MediaDataSource_Samsung::MediaDataSource_Samsung(
    const MediaDataSource_Samsung& other)
    : Resource(other) {
}

MediaDataSource_Samsung::MediaDataSource_Samsung(const Resource& data_source)
    : Resource(data_source) {
}

MediaDataSource_Samsung::~MediaDataSource_Samsung() {
}

MediaDataSource_Samsung::MediaDataSource_Samsung() {
}

MediaDataSource_Samsung::MediaDataSource_Samsung(PP_Resource resource)
    : Resource(resource) {
}

MediaDataSource_Samsung::MediaDataSource_Samsung(PassRef, PP_Resource resource)
    : Resource(PASS_REF, resource) {
}

MediaDataSource_Samsung& MediaDataSource_Samsung::operator=(
    const MediaDataSource_Samsung& other) {
  Resource::operator=(other);
  return *this;
}

}  // namespace pp
