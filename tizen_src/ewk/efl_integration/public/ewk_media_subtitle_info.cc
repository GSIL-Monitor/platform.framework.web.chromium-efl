// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_media_subtitle_info_product.h"
#include "private/ewk_private.h"

struct _Ewk_Media_Subtitle_Info {
  int id;
  const char* url;
  const char* lang;
  const char* label;
};

struct _Ewk_Media_Subtitle_Data {
  int id;
  double timestamp;
  unsigned size;
  const void* data;
};

int ewk_media_subtitle_info_id_get(Ewk_Media_Subtitle_Info *data)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);
  return data->id;
#else
  return 0;
#endif
}

const char* ewk_media_subtitle_info_url_get(Ewk_Media_Subtitle_Info *data)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, NULL);
  return data->url;
#else
  return NULL;
#endif
}

const char* ewk_media_subtitle_info_lang_get(Ewk_Media_Subtitle_Info *data)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, NULL);
  return data->lang;
#else
  return NULL;
#endif
}

const char* ewk_media_subtitle_info_label_get(Ewk_Media_Subtitle_Info *data)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, NULL);
  return data->label;
#else
  return NULL;
#endif
}

int ewk_media_subtitle_data_id_get(Ewk_Media_Subtitle_Data *data)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);
  return data->id;
#else
  return 0;
#endif
}

double ewk_media_subtitle_data_timestamp_get(Ewk_Media_Subtitle_Data *data)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);
  return data->timestamp;
#else
  return 0;
#endif
}

unsigned ewk_media_subtitle_data_size_get(Ewk_Media_Subtitle_Data *data)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, 0);
  return data->size;
#else
  return 0;
#endif

}

const void* ewk_media_subtitle_data_get(Ewk_Media_Subtitle_Data *data)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(data, NULL);
  return data->data;
#else
  return NULL;
#endif

}

#if defined(OS_TIZEN_TV_PRODUCT)
Ewk_Media_Subtitle_Info* ewkMediaSubtitleInfoCreate(int id, const char* url, const char* lang, const char* label)
{
  Ewk_Media_Subtitle_Info* subtitleInfo = new Ewk_Media_Subtitle_Info;
  subtitleInfo->id = id;
  subtitleInfo->url = eina_stringshare_add(url ? url : "");
  subtitleInfo->lang = eina_stringshare_add(lang ? lang : "");
  subtitleInfo->label = eina_stringshare_add(label ? label : "");
  return subtitleInfo;
}

void ewkMediaSubtitleInfoDelete(Ewk_Media_Subtitle_Info* data)
{
  if (!data)
    return;
  if (data->url)
    eina_stringshare_del(data->url);
  if (data->lang)
    eina_stringshare_del(data->lang);
  if (data->label)
    eina_stringshare_del(data->label);
  delete data;
}

Ewk_Media_Subtitle_Data* ewkMediaSubtitleDataCreate(int id, double timestamp, const void* data,unsigned size)
{
  Ewk_Media_Subtitle_Data* subtitleData = new Ewk_Media_Subtitle_Data;
  subtitleData->id = id;
  subtitleData->timestamp = timestamp;
  subtitleData->size = size;
  subtitleData->data = eina_binshare_add_length(data, size);
  return subtitleData;
}

void ewkMediaSubtitleDataDelete(Ewk_Media_Subtitle_Data* data)
{
  if (!data)
    return;
  if (data->data)
    eina_binshare_del(data->data);
  delete data;
}
#endif
