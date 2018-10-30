// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/ui/autofill/autofill_popup_view_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "content/browser/ui/autofill/autofill_popup_controller.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace autofill {

void AutofillPopupViewAndroid::Initialize() {
  use_credit_card_helper_ = false;
  use_upi_helper_ = false;
}

void AutofillPopupViewAndroid::SetAboveAnchor(JNIEnv* env,
      const JavaParamRef<jobject>& obj, jboolean above) {
  if (controller_)
    controller_->SetAboveAnchor((bool)(above == JNI_TRUE));
}

void AutofillPopupViewAndroid::UseCreditCardHelper(
    bool use_credit_card_helper) {
  use_credit_card_helper_ = use_credit_card_helper;
}

void AutofillPopupViewAndroid::UseUpiHelper(bool use_upi_helper) {
  use_upi_helper_ = use_upi_helper;
}
}
