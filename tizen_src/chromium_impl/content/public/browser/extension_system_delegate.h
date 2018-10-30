// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_EXTENSION_SYSTEM_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_EXTENSION_SYSTEM_DELEGATE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "base/values.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace content {

/*
 * ExtensionSystemDelegate is an interface for comunication with the EWebView
 */
class ExtensionSystemDelegate {
 public:
  virtual ~ExtensionSystemDelegate() {}
  virtual std::string GetEmbedderName() const = 0;
  virtual std::unique_ptr<base::Value> GetExtensionInfo() const = 0;
  virtual std::unique_ptr<base::Value> GenericSyncCall(
      const std::string& name,
      const base::Value& data) = 0;
  virtual int GetWindowId() const = 0;
};

/*
 * ExtensionSystemDelegateManager is a singleton for accessing the
 * ExtensionSystemDelegate interfaces that were registered by the EWebView
 */
class ExtensionSystemDelegateManager {
 public:
  // Struct that acts as a key for the interfaces map
  struct RenderFrameID {
    int render_process_id;
    int render_frame_id;
    bool operator==(const RenderFrameID& rhs) const;
  };

  static ExtensionSystemDelegateManager* GetInstance();

  // This function is called by the Host, it returns weak_ptr to the interface
  ExtensionSystemDelegate* GetDelegateForFrame(const RenderFrameID& id);

  // This set of functions should be accessed only by the EWebView
  // in order to manage the interface life
  void RegisterDelegate(const RenderFrameID& id,
                        std::unique_ptr<ExtensionSystemDelegate> delegate);
  bool UnregisterDelegate(const RenderFrameID& id);

 private:
  struct RenderFrameIDHasher {
    std::size_t operator()(const RenderFrameID& k) const;
  };

  friend struct base::DefaultSingletonTraits<ExtensionSystemDelegateManager>;

  ExtensionSystemDelegateManager();
  ~ExtensionSystemDelegateManager();

  std::unordered_map<RenderFrameID,
                     std::unique_ptr<ExtensionSystemDelegate>,
                     RenderFrameIDHasher> delegates_;


  DISALLOW_COPY_AND_ASSIGN(ExtensionSystemDelegateManager);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_EXTENSION_SYSTEM_DELEGATE_H_
