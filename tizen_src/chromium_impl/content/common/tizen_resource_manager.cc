// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/tizen_resource_manager.h"

#include <map>

#include "ri-api.h"

namespace {

const int kDefaultFramerate = 60;
const int kDefaultBpp = 8;
const int kDefaultSamplingFormat = content::VIDEO_DECODER_SAMPLING_420;

}  // namespace

namespace content {

const unsigned TizenResourceManager::kMaxResourceRequestSize = 10;

const std::map<media::VideoCodec, const char*> kCodecMap = {
    {media::kCodecH264, "H264"},   {media::kCodecHEVC, "HEVC"},
    {media::kCodecVP8, "VP8"},     {media::kCodecVP9, "VP9"},
    {media::kCodecMPEG2, "MPEG2"}, {media::kCodecMPEG4, "MPEG4"},
};

TizenAllocatedResource::TizenAllocatedResource() : id(-1) {}

TizenAllocatedResource::TizenAllocatedResource(int _id,
                                               const char* node,
                                               const char* omx_comp)
    : id(_id), device_node(node), omx_comp_name(omx_comp) {}

bool TizenResourceManager::PrepareVideoDecoderRequest(
    const TizenVideoDecoderDescription* desc,
    TizenRequestedResource* ret,
    bool sub) {
  std::map<media::VideoCodec, const char*>::const_iterator codec_it =
      kCodecMap.find(desc->codec);
  if (codec_it == kCodecMap.end()) {
    LOG(ERROR) << "Codec " << desc->codec << " not supported";
    return false;
  }

  ri_video_category_option_request_s opt = {0};
  opt.codec_name = codec_it->second;
  opt.color_depth = (desc->bpp != -1 ? desc->bpp : kDefaultBpp);
  opt.framerate = (desc->framerate != -1 ? desc->framerate : kDefaultFramerate);
  opt.h_size = desc->width;
  opt.v_size = desc->height;
  opt.sampling_format = kDefaultSamplingFormat;

  int category_option = ri_get_capable_video_category_id(&opt);
  if (category_option == RI_ERROR) {
    LOG(ERROR) << "Failed on ri_get_capable_video_category_id";
    return false;
  }

  ret->category_id = (sub ? RESOURCE_CATEGORY_VIDEO_DECODER_SUB
                          : RESOURCE_CATEGORY_VIDEO_DECODER);
  ret->category_option = category_option;
  ret->state = RESOURCE_STATE_EXCLUSIVE;

  return true;
}

void TizenResourceManager::PrepareScalerRequest(TizenRequestedResource* ret,
                                                bool sub) {
  ret->category_id =
      (sub ? RESOURCE_CATEGORY_SCALER_SUB : RESOURCE_CATEGORY_SCALER);
  ret->category_option = 0;
  ret->state = RESOURCE_STATE_EXCLUSIVE;
}

std::vector<int> TizenResourceManager::ResourceListToIds(
    const TizenResourceList* list) {
  size_t size = list->size();
  std::vector<int> ids(size);

  for (size_t i = 0; i < size; ++i)
    ids[i] = list->at(i)->id;

  return ids;
}

std::string TizenResourceManager::IdsToString(const std::vector<int> ids) {
  std::string ret;
  for (const int& id : ids)
    ret += (std::to_string(id) + ", ");

  return ret;
}

}  // namespace content
