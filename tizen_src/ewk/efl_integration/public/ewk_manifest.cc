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
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SAMSUNG ELECTRONICS. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ewk_manifest_internal.h"

#include "private/ewk_private.h"
#include "private/ewk_manifest_private.h"
#include "private/ewk_private.h"
#include "third_party/skia/include/core/SkColor.h"

/* LCOV_EXCL_START */
const char* ewk_manifest_short_name_get(Ewk_View_Request_Manifest* manifest) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, nullptr);
  return !(manifest->short_name.empty()) ? manifest->short_name.c_str()
                                         : nullptr;
}

const char* ewk_manifest_name_get(Ewk_View_Request_Manifest* manifest) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, nullptr);
  return !(manifest->name.empty()) ? manifest->name.c_str() : nullptr;
}

const char* ewk_manifest_start_url_get(Ewk_View_Request_Manifest* manifest) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, nullptr);
  return !(manifest->start_url.empty()) ? manifest->start_url.c_str() : nullptr;
}

Ewk_View_Orientation_Type ewk_manifest_orientation_type_get(
    Ewk_View_Request_Manifest* manifest) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, WebScreenOrientationLockDefault);
  return manifest->orientation_type;
}

Ewk_View_Web_Display_Mode ewk_manifest_web_display_mode_get(
    Ewk_View_Request_Manifest* manifest) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, WebDisplayModeUndefined);
  return manifest->web_display_mode;
}

Eina_Bool ewk_manifest_theme_color_get(_Ewk_View_Request_Manifest* manifest,
                                       uint8_t* r,
                                       uint8_t* g,
                                       uint8_t* b,
                                       uint8_t* a) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, EINA_FALSE);
  if (manifest->theme_color == content::Manifest::kInvalidOrMissingColor)
    return EINA_FALSE;

  if (r)
    *r = SkColorGetR(manifest->theme_color);
  if (g)
    *g = SkColorGetG(manifest->theme_color);
  if (b)
    *b = SkColorGetB(manifest->theme_color);
  if (a)
    *a = SkColorGetA(manifest->theme_color);

  return EINA_TRUE;
  ;
}

Eina_Bool ewk_manifest_background_color_get(
    _Ewk_View_Request_Manifest* manifest,
    uint8_t* r,
    uint8_t* g,
    uint8_t* b,
    uint8_t* a) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, EINA_FALSE);
  if (manifest->background_color == content::Manifest::kInvalidOrMissingColor)
    return EINA_FALSE;

  if (r)
    *r = SkColorGetR(manifest->background_color);
  if (g)
    *g = SkColorGetG(manifest->background_color);
  if (b)
    *b = SkColorGetB(manifest->background_color);
  if (a)
    *a = SkColorGetA(manifest->background_color);

  return EINA_TRUE;
}

size_t ewk_manifest_icons_count_get(_Ewk_View_Request_Manifest* manifest) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, 0);
  return manifest->icons.size();
}

const char* ewk_manifest_icons_src_get(_Ewk_View_Request_Manifest* manifest,
                                       size_t number) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, nullptr);
  if (manifest->icons.size() <= number)
    return nullptr;

  return !(manifest->icons[number].src.empty())
             ? manifest->icons[number].src.c_str()
             : nullptr;
}

const char* ewk_manifest_icons_type_get(_Ewk_View_Request_Manifest* manifest,
                                        size_t number) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, nullptr);
  if (manifest->icons.size() <= number)
    return nullptr;

  return !(manifest->icons[number].type.empty())
             ? manifest->icons[number].type.c_str()
             : nullptr;
}

size_t ewk_manifest_icons_sizes_count_get(_Ewk_View_Request_Manifest* manifest,
                                          size_t number) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, 0);
  return !(manifest->icons.size() <= number)
             ? manifest->icons[number].sizes.size()
             : 0;
}

int ewk_manifest_icons_width_get(_Ewk_View_Request_Manifest* manifest,
                                 size_t number,
                                 size_t sizes_number) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, -1);
  if (manifest->icons.size() <= number)
    return -1;

  return !(manifest->icons[number].sizes.size() <= sizes_number)
             ? manifest->icons[number].sizes[sizes_number].width
             : -1;
}

int ewk_manifest_icons_height_get(_Ewk_View_Request_Manifest* manifest,
                                  size_t number,
                                  size_t sizes_number) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, -1);
  if (manifest->icons.size() <= number)
    return -1;

  return !(manifest->icons[number].sizes.size() <= sizes_number)
             ? manifest->icons[number].sizes[sizes_number].height
             : -1;
}

const char* ewk_manifest_push_sender_id_get(
    Ewk_View_Request_Manifest* manifest) {
  EINA_SAFETY_ON_NULL_RETURN_VAL(manifest, nullptr);
  return manifest->spp_sender_id.c_str();
}
/* LCOV_EXCL_STOP */
