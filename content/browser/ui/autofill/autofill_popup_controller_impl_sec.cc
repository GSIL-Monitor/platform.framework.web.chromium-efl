// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/ui/autofill/autofill_popup_controller_impl.h"

#include <algorithm>
#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_popup_delegate.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/suggestion.h"
#include "components/autofill/core/common/autofill_util.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "terrace/terrace_jni_headers/terrace/jni/TinAutofillSuggestionPopupBridge_jni.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/text_elider.h"
#include "ui/gfx/text_utils.h"

using base::WeakPtr;
using base::android::ScopedJavaLocalRef;

namespace autofill {

// Suggestion Popup control functions
void AutofillPopupControllerImpl::ShowSuggestionPopup(bool hasNoSuggestion) {
  // the popup is already shown
  if (suggestion_popup_shown_ || (!use_credit_card_helper_ && !use_upi_helper_))
    return;
  suggestion_popup_shown_ = true;

  // Create Autofill Suggestion Popup Bridge
  ui::ViewAndroid* view_android = controller_common_.container_view;

  suggestion_popup_view_ = view_android->AcquireAnchorView();
  const ScopedJavaLocalRef<jobject> view = suggestion_popup_view_.view();
  if (view.is_null())
    return;
  view_android->SetAnchorRect(view, controller_common_.element_bounds);

// FIXME: m62_3202 bringup, needs to be checked by author
// https://chromium-review.googlesource.com/c/chromium/src/+/558004
/*  JNIEnv* env = base::android::AttachCurrentThread();
  java_object_.Reset(Java_TinAutofillSuggestionPopupBridge_create(
      env, reinterpret_cast<intptr_t>(this), view,
      view_android->GetWindowAndroid()->GetJavaObject().obj(),
      view_android->GetViewAndroidDelegate().obj(), use_credit_card_helper_));*/

  // Just set the anchor position
  UpdateSuggestionPopupAnchor(
      element_bounds().x(), element_bounds().y(), element_bounds().width(),
      element_bounds().height(),
      hasNoSuggestion,  // if hasNoSuggestion, show full popup
      true);
}

void AutofillPopupControllerImpl::UpdateSuggestionPopupAnchor(float x,
                                                              float y,
                                                              float width,
                                                              float height,
                                                              bool isFullMode,
                                                              bool is_scroll) {
  if (!suggestion_popup_shown_ || java_object_.is_null()) return;

  // Create Autofill Suggestion Popup Bridge
  const ScopedJavaLocalRef<jobject> view = suggestion_popup_view_.view();
  if (view.is_null())
    return;

  ui::ViewAndroid* view_android = controller_common_.container_view;
  DCHECK(view_android);

  gfx::RectF bounds(x, y, width, height);
  view_android->SetAnchorRect(view, bounds);

// FIXME: m62_3202 bringup, needs to be checked by author
// https://chromium-review.googlesource.com/c/chromium/src/+/558004
/*  JNIEnv* env = base::android::AttachCurrentThread();
  Java_TinAutofillSuggestionPopupBridge_updateSuggestionPopupAnchor(
      env, java_object_.obj(), x, y, width, height, isFullMode,
      use_credit_card_helper_, is_scroll);*/
}

std::string AutofillPopupControllerImpl::GetRegisteredUpiVpa() const {
  return upi_vpa_registered_;
}

void AutofillPopupControllerImpl::DestroySuggestionPopup() {
  if (!suggestion_popup_shown_)
    return;
  suggestion_popup_shown_ = false;

  if (java_object_.is_null()) return;

// FIXME: m62_3202 bringup, needs to be checked by author
// https://chromium-review.googlesource.com/c/chromium/src/+/558004
/*  JNIEnv* env = base::android::AttachCurrentThread();
  Java_TinAutofillSuggestionPopupBridge_destroySuggestionPopup(
      env, java_object_.obj());*/
}

void AutofillPopupControllerImpl::DestroyPopupControllerImpl() {
  DestroySuggestionPopup();
  // NOTE: PopupControllerImpl's life ends here!
  delete this;
}

void AutofillPopupControllerImpl::UseCreditCardHelper(
    bool use_credit_card_helper) {
  use_credit_card_helper_ = use_credit_card_helper;
}

void AutofillPopupControllerImpl::UseUpiHelper(bool use_upi_helper) {
  use_upi_helper_ = use_upi_helper;
}

void AutofillPopupControllerImpl::OpenAutofillCreditCard(
    JNIEnv* env, jobject obj) {
  const int openFormData = -1;
  AcceptSuggestion(openFormData);
}

void AutofillPopupControllerImpl::AutofillUpiVpa(JNIEnv* env,
                                                 jobject obj,
                                                 jstring upi_vpa) {
  upi_vpa_registered_ = base::android::ConvertJavaStringToUTF8(env, upi_vpa);
  const int openFormData = -1;
  AcceptSuggestion(openFormData);
}

bool AutofillPopupControllerImpl::IsAboveAnchor(
    JNIEnv* env, jobject obj) {
  return is_above_anchor_;
}

void AutofillPopupControllerImpl::ChangePopupShownStatus(JNIEnv* env,
                                                         jobject obj,
                                                         jboolean status) {
  suggestion_popup_shown_ = status;
}

void AutofillPopupControllerImpl::SetAboveAnchor(bool above) {
  is_above_anchor_ = above;
}

}  // namespace autofill
