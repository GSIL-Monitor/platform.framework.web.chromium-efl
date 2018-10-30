// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_custom_handlers_private_h
#define ewk_custom_handlers_private_h

#include "public/ewk_custom_handlers_internal.h"

struct Ewk_Custom_Handlers_Data {
 public:
  Ewk_Custom_Handlers_Data();
  Ewk_Custom_Handlers_Data(const char* protocol, const char* baseUrl,
      const char* full_url);
  ~Ewk_Custom_Handlers_Data();

  Eina_Stringshare* getTarget() const;
  Eina_Stringshare* getBaseUrl() const;
  Eina_Stringshare* getUrl() const;
  Ewk_Custom_Handlers_State getResult() const;
  void setResult(Ewk_Custom_Handlers_State result_);

 private:
  Eina_Stringshare* target;
  Eina_Stringshare* base_url;
  Eina_Stringshare* url;
  Ewk_Custom_Handlers_State result;
};


#endif // ewk_custom_handlers_private_h

