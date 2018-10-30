/*
 * Copyright (C) 2016 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SAMSUNG ELECTRONICS. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SAMSUNG ELECTRONICS. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ewk_media_playback_info_product.h"
#include "private/ewk_private.h"

struct _Ewk_Media_Playback_Info {
  const char* media_url;
  const char* mime_type;
  unsigned char media_resource_acquired;
  const char* translated_url;
  const char* drm_info;
};

const char* ewk_media_playback_info_media_url_get(
    Ewk_Media_Playback_Info* data) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);
  return data->media_url;
}

const char* ewk_media_playback_info_mime_type_get(
    Ewk_Media_Playback_Info* data) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);
  return data->mime_type;
}

unsigned char ewk_media_playback_info_media_resource_acquired_get(
    Ewk_Media_Playback_Info* data) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, false);
  return data->media_resource_acquired;
}

const char* ewk_media_playback_info_translated_url_get(
    Ewk_Media_Playback_Info* data) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);
  return data->translated_url;
}

const char* ewk_media_playback_info_drm_info_get(
    Ewk_Media_Playback_Info* data) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);
  return data->drm_info;
}

void ewk_media_playback_info_media_resource_acquired_set(
    Ewk_Media_Playback_Info* data,
    unsigned char media_resource_acquired) {
  data->media_resource_acquired = media_resource_acquired;
}

void ewk_media_playback_info_translated_url_set(Ewk_Media_Playback_Info* data,
                                                const char* translated_url) {
  data->translated_url =
    eina_stringshare_add(translated_url ? translated_url : "");
}

void ewk_media_playback_info_drm_info_set(Ewk_Media_Playback_Info* data,
                                          const char* drm_info) {
  data->drm_info = eina_stringshare_add(drm_info ? drm_info : "");
}

Ewk_Media_Playback_Info* ewkMediaPlaybackInfoCreate(const char* url,
                                                    const char* mime_type) {
  Ewk_Media_Playback_Info* playback_info = new Ewk_Media_Playback_Info;
  playback_info->media_url = eina_stringshare_add(url ? url : "");
  playback_info->mime_type = eina_stringshare_add(mime_type ? mime_type : "");
  playback_info->media_resource_acquired = false;
  playback_info->translated_url = 0;
  playback_info->drm_info = 0;
  return playback_info;
}

void ewkMediaPlaybackInfoDelete(Ewk_Media_Playback_Info* data) {
  if (data->media_url)
    eina_stringshare_del(data->media_url);
  if (data->mime_type)
    eina_stringshare_del(data->mime_type);
  if (data->translated_url)
    eina_stringshare_del(data->translated_url);
  if (data->drm_info)
    eina_stringshare_del(data->drm_info);
  delete data;
}
