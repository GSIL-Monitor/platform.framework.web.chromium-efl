// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_TIZEN_RESOURCE_MANAGER_IMPL_H_
#define CONTENT_COMMON_TIZEN_RESOURCE_MANAGER_IMPL_H_

#include "content/common/tizen_resource_manager.h"

#include <map>
#include <vector>

#include "rm_api.h"

#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"

namespace content {

class TizenResourceManagerImpl : public TizenResourceManager {
 public:
  // TizenResourceManager implementation. Can be called on any thread.
  // These are blocking calls that does IPC with platform's RM service.
  bool AllocateResources(ResourceClient* client,
                         const std::vector<TizenRequestedResource>& req,
                         TizenResourceList* ret) override;
  bool ReleaseResources(ResourceClient* client,
                        const std::vector<int>& ids) override;

  static TizenResourceManagerImpl* GetInstance();
  rm_cb_result RMResourceCallback(int handle,
                                  rm_callback_type event,
                                  rm_device_request_s* info);

 private:
  struct AllocatedResource {
    ResourceClient* client;
    AllocatedResource() : client(nullptr) {}
    explicit AllocatedResource(ResourceClient* _client) : client(_client) {}
  };

  // This lock protects all members of this class
  base::Lock rm_lock_;

  int rm_handle_;
  typedef std::map<int, AllocatedResource> ClientMap;
  ClientMap client_map_;

  TizenResourceManagerImpl();
  virtual ~TizenResourceManagerImpl();

  bool RegisterIfNeeded(void);
  void Unregister(void);

  bool AppendToRequest(const TizenRequestedResource& req,
                       rm_category_request_s* rm_req);
  bool InternalRequestCheck(const TizenRequestedResource& req_item);
  bool IssueRequest(ResourceClient* client,
                    const rm_category_request_s* req,
                    TizenResourceList* ret);
  void ParseAllocatedDevices(ResourceClient* client,
                             const rm_device_return_s* allocated,
                             TizenResourceList* ret);
  bool DoReleaseResources(const ResourceClient* client,
                          const std::vector<int>& ids);
  bool DoReleaseResource(const ResourceClient* client, int id);
  ResourceClient* GetClientOfResource(int id);
  rm_cb_result ClientResourceConflict(ResourceClient* client,
                                      TizenResourceConflictType type,
                                      const std::vector<int> ids);

  friend struct base::DefaultSingletonTraits<TizenResourceManagerImpl>;
};

}  // namespace content

#endif  // CONTENT_COMMON_TIZEN_RESOURCE_MANAGER_IMPL_H_
