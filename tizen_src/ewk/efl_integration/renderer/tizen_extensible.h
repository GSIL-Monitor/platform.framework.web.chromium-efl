// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_RENDERER_TIZEN_EXTENSIBLE_H_
#define EWK_EFL_INTEGRATION_RENDERER_TIZEN_EXTENSIBLE_H_

#include <map>
#include <string>

#include "base/memory/singleton.h"

class TizenExtensible {
 public:
  static TizenExtensible* GetInstance() {
    return base::Singleton<TizenExtensible>::get();  // LCOV_EXCL_LINE
  }
  void UpdateTizenExtensible(const std::map<std::string, bool>& params);

  // Tizen Extensible API
  bool SetExtensibleAPI(const std::string& api_name, bool enable);
  bool GetExtensibleAPI(const std::string& api_name) const;

 private:
  TizenExtensible();
  ~TizenExtensible();

  std::map<std::string, bool> extensible_api_table_;
  bool initialized_;
  friend struct base::DefaultSingletonTraits<TizenExtensible>;
};

#endif  // EWK_EFL_INTEGRATION_RENDERER_TIZEN_EXTENSIBLE_H_
