// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef popup_controller_efl_h
#define popup_controller_efl_h

#include <Evas.h>

#include "eweb_view.h"

namespace content {

class PopupControllerEfl {
 public:
  enum Content_Type {
    EMAIL,
    PHONE,
    UNKNOWN
  };

  enum Context_Option {
    ADD_CONTACT,
    CALL,
    COPY,
    SEND_EMAIL,
    SEND_MESSAGE
  };

  PopupControllerEfl(EWebView*);
  void openPopup(const char*);
  void closePopup();
  Evas_Object* popup() { return popup_; }
  Content_Type popupContentType() { return content_type_; }
  const std::string& content() { return content_; }
  const std::string& fullContent() { return full_content_; }
  void SetPopupSize(int width, int height);

 private:
  static char* contextMenuGenlistTextSet(
      void* data,
      Evas_Object* obj,
      const char* part);
  static void itemSelected(void *data, Evas_Object *obj, void *event_info);
  static std::vector<Context_Option> options_;

  bool createPopup();
  void addOptions();
  void appendItems();
  void addCallbacks();
  void showPopup();

  EWebView* web_view_;
  Evas_Object* top_widget_;
  Evas_Object* popup_;
  Content_Type content_type_;
  std::string content_;
  std::string full_content_;
};

} // namespace

#endif // popup_controller_efl_h
