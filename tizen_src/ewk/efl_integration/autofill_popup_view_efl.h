// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AUTOFILL_POPUP_EFL_H
#define AUTOFILL_POPUP_EFL_H

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include <Ecore.h>
#include "ecore_x_wayland_wrapper.h"
#include <Edje.h>
#include <Eina.h>
#include <Elementary.h>
#include <Evas.h>

#include "components/autofill/core/browser/autofill_popup_delegate.h"
#include "components/autofill/core/browser/suggestion.h"
#include "components/password_manager/core/browser/password_form_manager.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"

class EWebView;

namespace gfx {
class RectF;
}

namespace autofill {

class AutofillPopupViewEfl {
 public:
  AutofillPopupViewEfl(EWebView* view);
  ~AutofillPopupViewEfl();
  void Show();
  void Hide();
  void ShowSavePasswordPopup(
      std::unique_ptr<password_manager::PasswordFormManager> form_to_save);
  void UpdateFormDataPopup(const gfx::RectF& bounds);
  void UpdateLocation(const gfx::RectF& focused_element_bounds);
  void InitFormData(
      const std::vector<autofill::Suggestion>& suggestions,
      base::WeakPtr<AutofillPopupDelegate> delegate);
  void AcceptSuggestion(size_t index);
  void AcceptPasswordSuggestion(int option);
  void SetSelectedLine(size_t index);
  bool IsVisible() const { return evas_object_visible_get(autofill_popup_); }
  static char* getItemLabel(void* data, Evas_Object* obj, const char* part);
  static void itemSelectCb(void* data, Evas_Object* obj, void* event_info);
  static void savePasswordNeverCb(void *data, Evas_Object *obj, void *event_info);
  static void savePasswordYesCb(void *data, Evas_Object *obj, void *event_info);
  static void savePasswordNotNowCb(void *data, Evas_Object *obj, void *event_info);

 private:
  EWebView* webview_;
  Evas_Object* autofill_popup_;
  Evas_Object* autofill_list_;
  Evas_Object* password_popup_;
  std::vector<autofill::Suggestion> values_;
  size_t selected_line_;
  base::WeakPtr<AutofillPopupDelegate> delegate_;
  std::unique_ptr<password_manager::PasswordFormManager> form_manager_;
};

} //namespace autofill

#endif //TIZEN_AUTOFILL_SUPPORT

#endif // autofill_popup_efl_h
