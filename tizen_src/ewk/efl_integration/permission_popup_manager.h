// Copyright 2015-2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PermissionPopupManager_h
#define PermissionPopupManager_h

#include <string>
#include <Elementary.h>

class PermissionPopup;

class PermissionPopupManager {
 public:
  PermissionPopupManager(Evas_Object* evas_object);
  ~PermissionPopupManager();

  void AddPermissionRequest(PermissionPopup* permission_popup);
  void CancelPermissionRequest();

 private:
  bool CreatePopup();
  bool SetupPopup(PermissionPopup* permission_popup);
  void DeletePopup();
  bool SetPopupText(const std::string& message);
  bool SetPopupTitle(const std::string& host);
  void AddButtonToPopup(const char* name, const char* text,
                        Evas_Smart_Cb callback, bool focus);
  void DeleteAllPermissionRequests();
  void DeletePermissionRequest(PermissionPopup* permission_popup);
  void ShowPermissionPopup(PermissionPopup* permission_popup);
  void Decide(bool decision);

  static void PermissionOkCallback(void* data, Evas_Object* obj,
                                   void* event_info);
  static void PermissionCancelCallback(void* data, Evas_Object* obj,
                                       void* event_info);

  Eina_List* permission_popups_;
  Evas_Object* evas_object_;
  Evas_Object* popup_;
  Evas_Object* top_widget_;
};

#endif // PermissionPopupManager_h
