// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_custom_handlers_private.h"

Ewk_Custom_Handlers_Data::Ewk_Custom_Handlers_Data()
  : target(0)
  , base_url(0)
  , url(0)
  , result(EWK_CUSTOM_HANDLERS_NEW) {
}

Ewk_Custom_Handlers_Data::Ewk_Custom_Handlers_Data(const char* protocol, const char* baseUrl, const char* full_url)
  : target(eina_stringshare_add(protocol))
  , base_url(eina_stringshare_add(baseUrl))
  , url(eina_stringshare_add(full_url))
  , result(EWK_CUSTOM_HANDLERS_NEW) {
}

Ewk_Custom_Handlers_Data::~Ewk_Custom_Handlers_Data() {
  eina_stringshare_del(target);
  eina_stringshare_del(base_url);
  eina_stringshare_del(url);
}

Eina_Stringshare* Ewk_Custom_Handlers_Data::getTarget() const {
  return target;
}

Eina_Stringshare* Ewk_Custom_Handlers_Data::getBaseUrl() const {
  return base_url;
}

Eina_Stringshare* Ewk_Custom_Handlers_Data::getUrl() const {
  return url;
}

Ewk_Custom_Handlers_State Ewk_Custom_Handlers_Data::getResult() const {
  return result;
}

void Ewk_Custom_Handlers_Data::setResult(Ewk_Custom_Handlers_State result_) {
  result=result_;
}
