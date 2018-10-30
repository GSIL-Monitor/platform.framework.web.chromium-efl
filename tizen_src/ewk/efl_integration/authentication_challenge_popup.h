// Copyright 2016-2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_AUTHENTICATION_CHALLENGE_POPUP_H_
#define EWK_EFL_INTEGRATION_AUTHENTICATION_CHALLENGE_POPUP_H_

#include <Elementary.h>
#include <string>

#include "public/ewk_auth_challenge_internal.h"

class AuthenticationChallengePopup {
 public:
  ~AuthenticationChallengePopup();

  static void CreateAndShow(Ewk_Auth_Challenge* auth_challenge,
                            Evas_Object* ewk_view);

 private:
  explicit AuthenticationChallengePopup(Ewk_Auth_Challenge* auth_challenge,
                                        Evas_Object* ewk_view);

  bool ShowAuthenticationPopup(const char* url);
  static void AuthenticationLoginCallback(void* data,
                                          Evas_Object* obj,
                                          void* event_info);
#if defined(OS_TIZEN)
  static void HwBackKeyCallback(void* data, Evas_Object* obj, void* event_info);
#endif
  Evas_Object* CreateBorder(const std::string& text, Evas_Object* entry_box);
  Evas_Object* CreateContainerBox();
  Evas_Object* CreateTextBox(const char* edj_path, Evas_Object* container_box);
  bool CreateTextLabel(const char* edj_path, Evas_Object* container_box);

  Evas_Object* conformant_;
  Evas_Object* ewk_view_;
  Evas_Object* login_button_;
  Evas_Object* layout_;
  Evas_Object* password_entry_;
  Evas_Object* popup_;
  Evas_Object* user_name_entry_;
  Ewk_Auth_Challenge* auth_challenge_;
};

#endif  // EWK_EFL_INTEGRATION_AUTHENTICATION_CHALLENGE_POPUP_H_
