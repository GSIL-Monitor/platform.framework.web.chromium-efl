// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_SRC_EWK_EFL_INTEGRATION_EWK_EXTENSION_SYSTEM_DELEGATE_H_
#define TIZEN_SRC_EWK_EFL_INTEGRATION_EWK_EXTENSION_SYSTEM_DELEGATE_H_

#include <string>

#include "base/synchronization/lock.h"
#include "content/public/browser/extension_system_delegate.h"
#include "public/ewk_context_product.h"
#include "public/ewk_value_product.h"
#include "public/ewk_view_product.h"

class EwkExtensionSystemDelegate : public content::ExtensionSystemDelegate {
 public:
  EwkExtensionSystemDelegate();
  ~EwkExtensionSystemDelegate() override;

  std::string GetEmbedderName() const override;
  std::unique_ptr<base::Value> GetExtensionInfo() const override;
  std::unique_ptr<base::Value> GenericSyncCall(
      const std::string& name,
      const base::Value& data) override;

  int GetWindowId() const override;

  static void SetEmbedderName(const Ewk_Application_Type embedder_type);
  void SetExtensionInfo(Ewk_Value info);
  void SetGenericSyncCallback(Generic_Sync_Call_Callback cb, void* data);

  void SetWindowId(const Evas_Object* main_window);

 private:
  Ewk_Value info_;
  Generic_Sync_Call_Callback cb_;
  void* cb_data_;
  int window_id_;

  // TODO(a.bujalski) Check if it is possible to call WRT's cb on IO thread.
  mutable base::Lock access_lock_;
};

#endif  // TIZEN_SRC_EWK_EFL_INTEGRATION_EWK_EXTENSION_SYSTEM_DELEGATE_H_
