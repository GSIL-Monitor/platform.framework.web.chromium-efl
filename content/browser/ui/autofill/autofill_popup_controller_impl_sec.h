// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

public:
  // Suggestion Popup control functions
  void ShowSuggestionPopup(bool hasNoSuggestion);
  void DestroySuggestionPopup();
  void UseCreditCardHelper(bool use_credit_card_helper);
  void UseUpiHelper(bool use_upi_helper);
  void UpdateSuggestionPopupAnchor(float x,
                                   float y,
                                   float width,
                                   float height,
                                   bool isFullMode,
                                   bool is_scroll);
  void DestroyPopupControllerImpl();
  std::string GetRegisteredUpiVpa() const;

  void OpenAutofillCreditCard(JNIEnv* env, jobject obj);
  void AutofillUpiVpa(JNIEnv* env, jobject obj, jstring upi_vpa);
  void ChangePopupShownStatus(JNIEnv* env, jobject obj, jboolean status);
  bool IsAboveAnchor(JNIEnv* env, jobject obj);
  void SetAboveAnchor(bool above) override;

private:
  bool is_above_anchor_;
  bool suggestion_popup_shown_;
  bool use_credit_card_helper_;
  bool use_upi_helper_;
  std::string upi_vpa_registered_;

  // The corresponding java object.
  base::android::ScopedJavaGlobalRef<jobject> java_object_;

  // Popup view
  ui::ViewAndroid::ScopedAnchorView suggestion_popup_view_;
