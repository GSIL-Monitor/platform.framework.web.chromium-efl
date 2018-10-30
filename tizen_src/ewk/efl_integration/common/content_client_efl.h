// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CLIENT_EFL_H_
#define CONTENT_CLIENT_EFL_H_

#include <string>
#include <vector>

#include "content/public/common/content_client.h"

#include "base/compiler_specific.h"
#include "components/nacl/common/features.h"

namespace IPC {
  class Message;
}

class ContentClientEfl : public content::ContentClient {
 public:
  // ContentClient implementation.
  virtual std::string GetProduct() const override;
  virtual std::string GetUserAgent() const override;
  virtual base::string16 GetLocalizedString(int message_id) const override;
  virtual base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) const override;
  virtual bool CanSendWhileSwappedOut(const IPC::Message* message) override;
  void AddPepperPlugins(
      std::vector<content::PepperPluginInfo>* plugins) override;
#if BUILDFLAG(ENABLE_NACL)
  std::string GetProcessTypeNameInEnglish(int type) override;
#endif
};

#endif  // CONTENT_CLIENT_EFL_H_
