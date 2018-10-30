// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

private:
  void Initialize();
  void UseCreditCardHelper(bool use_credit_card_helper) override;
  void UseUpiHelper(bool use_upi_helper) override;

 public:
  void SetAboveAnchor(JNIEnv* env,
                      const base::android::JavaParamRef<jobject>& obj,
                      jboolean above);

private:
  bool use_credit_card_helper_;
  bool use_upi_helper_;
