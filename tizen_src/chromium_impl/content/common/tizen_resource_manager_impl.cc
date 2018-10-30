// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/tizen_resource_manager_impl.h"

#include <stdlib.h>

#include <utility>

#include "ri-api.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"

namespace {

// Entry point for resource conflict callback. Called on private RM thread.
rm_cb_result RMResourceCallbackEP(int handle,
                                  rm_callback_type event,
                                  rm_device_request_s* info,
                                  void* data) {
  content::TizenResourceManagerImpl* mgr =
      content::TizenResourceManagerImpl::GetInstance();
  return mgr->RMResourceCallback(handle, event, info);
}

const int kDefaultFramerate = 60;
const int kDefaultBpp = 8;
const int kDefaultSamplingFormat = 1;

/* Maps chromium specific TizenResourceCategoryId to RM resource type
 * rm_rsc_category_e. */
const rm_rsc_category_e kResourceCategoryMap[content::RESOURCE_CATEGORY_MAX] = {
    RM_CATEGORY_VIDEO_DECODER, RM_CATEGORY_VIDEO_DECODER_SUB,
    RM_CATEGORY_SCALER, RM_CATEGORY_SCALER_SUB,
};

/* Maps chromium specific TizenResourceState to RM resource state type
 * rm_requests_resource_state_e. */
const rm_requests_resource_state_e
    kResourceStateMap[content::RESOURCE_STATE_MAX] = {
        RM_STATE_PASSIVE, RM_STATE_SHARABLE, RM_STATE_EXCLUSIVE,
        RM_STATE_EXCLUSIVE_CONDITIONAL, RM_STATE_EXCLUSIVE_AUTO};

}  // namespace

namespace content {

TizenResourceManagerImpl::TizenResourceManagerImpl() : rm_handle_(-1) {}

TizenResourceManagerImpl::~TizenResourceManagerImpl() {
  base::AutoLock lock(rm_lock_);

  // Release all resources
  while (!client_map_.empty()) {
    ClientMap::iterator it = client_map_.begin();
    LOG(WARNING) << "Client " << it->second.client
                 << " has not released his resource id=" << it->first
                 << " before destroying browser process. Doing it now.";
    DoReleaseResource(it->second.client, it->first);
  }

  Unregister();
}

TizenResourceManagerImpl::ResourceClient*
TizenResourceManagerImpl::GetClientOfResource(int id) {
  std::map<int, AllocatedResource>::iterator it = client_map_.find(id);
  if (it == client_map_.end()) {
    LOG(ERROR) << "Can't find client for resource id=" << id;
    return nullptr;
  }

  return it->second.client;
}

rm_cb_result TizenResourceManagerImpl::RMResourceCallback(
    int handle,
    rm_callback_type event,
    rm_device_request_s* info) {
  if (!info)
    return RM_CB_RESULT_ERROR;

  base::AutoLock lock(rm_lock_);

  int count = info->request_num;
  rm_cb_result ret = RM_CB_RESULT_OK;
  /* We will group every conflicted id by its client */
  std::map<ResourceClient*, std::vector<int>> clients;
  std::vector<int> clientless_resources;

  for (int i = 0; i < count; ++i) {
    int id = info->device_id[i];
    ResourceClient* client = GetClientOfResource(id);
    if (!client)
      clientless_resources.push_back(id);
    else
      clients[client].push_back(id);
  }

  std::map<ResourceClient*, std::vector<int>>::iterator it = clients.begin();
  while (it != clients.end()) {
    rm_cb_result cur_ret = ClientResourceConflict(
        it->first, static_cast<TizenResourceConflictType>(event), it->second);

    if (cur_ret != RM_CB_RESULT_OK)
      ret = cur_ret;

    it++;
  }

  if (!clientless_resources.empty()) {
    // Should never happen but better be safe than sorry, so release it
    LOG(ERROR) << "There are conflicted resources without any client ids="
               << TizenResourceManager::IdsToString(clientless_resources);
    DoReleaseResources(nullptr, clientless_resources);
  }

  return ret;
}

rm_cb_result TizenResourceManagerImpl::ClientResourceConflict(
    ResourceClient* client,
    TizenResourceConflictType type,
    const std::vector<int> ids) {
  {
    base::AutoUnlock unlock(rm_lock_);
    base::WaitableEvent wait(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                             base::WaitableEvent::InitialState::NOT_SIGNALED);

    LOG(INFO) << "Notify client " << client << " with resource conflict"
              << " callback for ids=" << TizenResourceManager::IdsToString(ids);

    bool ret = client->RMResourceCallback(&wait, type, ids);
    LOG(INFO) << "Client " << client << " callback ret=" << ret;
    if (!ret) {
      wait.Wait();
    }
  }
  for (const int& id : ids) {
    // Check if resource was really released
    if (client_map_.find(id) != client_map_.end()) {
      LOG(WARNING) << "Client " << client << " has completed callback"
                   << " without really releasing resource id=" << id;
    }
  }

  return RM_CB_RESULT_OK;
}

bool TizenResourceManagerImpl::RegisterIfNeeded() {
  if (rm_handle_ != -1)
    return true;

  int ret = rm_register(&RMResourceCallbackEP, NULL, &rm_handle_, NULL);
  if (ret != RM_OK) {
    rm_handle_ = -1;
    LOG(ERROR) << "Failed on rm_register ret=" << ret;
    return false;
  }

  return true;
}

void TizenResourceManagerImpl::Unregister() {
  if (rm_handle_ == -1)
    return;

  if (!client_map_.empty()) {
    // This shouldn't ever happen. We do not have to release those resources
    // as rm_unregister() does this uncoditionally.
    LOG(ERROR) << "Trying to unregister RM handle while some resources still"
               << " allocated.";
  }

  LOG(ERROR) << "Calling unregister";
  int ret = rm_unregister(rm_handle_);
  if (ret == RM_OK)
    rm_handle_ = -1;
  else
    LOG(ERROR) << "Failed on rm_unregister err=" << ret;
}

bool TizenResourceManagerImpl::InternalRequestCheck(
    const TizenRequestedResource& req_item) {
  if (req_item.category_id == RESOURCE_CATEGORY_SCALER ||
      req_item.category_id == RESOURCE_CATEGORY_SCALER_SUB) {
    // If scaler is requested for allocation we must first check if it isn't
    // used by other chromium components.
    // When scaler is already used by some other process, platform RM will
    // post a resource conflict to this process, requesting resource release.
    // This is desired behavior, however Tizen's chromium use platform player
    // which does everything in separate process (muse_server).
    // If any player is currently active in chromium, we assume it does use
    // scaler, and we don't want to disturb it by causing resource conflict.
    // TODO(i.gorniak): implement active player checking
  }

  return true;
}

bool TizenResourceManagerImpl::AppendToRequest(
    const TizenRequestedResource& req,
    rm_category_request_s* rm_req) {
  size_t idx = rm_req->request_num;
  // Some sanity checking
  if (req.category_id < 0 || req.category_id >= RESOURCE_CATEGORY_MAX) {
    LOG(ERROR) << "Provided resource category id is beyond sane limits"
               << " idx=" << idx;
    return false;
  }

  if (req.state < 0 || req.state >= RESOURCE_STATE_MAX) {
    LOG(ERROR) << "Provided resource state is beyond sane limits"
               << " idx=" << idx;
    return false;
  }

  // It's hard to check sanity of req.category_option because rm_type.h defines
  // RM_CATEGORY_*_OPTION in non-linear way. If value is invalid then only risk
  // is failed allocation by platform's RM.

  rm_req->category_id[idx] = kResourceCategoryMap[req.category_id];
  rm_req->category_option[idx] = req.category_option;
  rm_req->state[idx] = kResourceStateMap[req.state];
  rm_req->device_node[idx] = NULL;
  rm_req->request_num++;

  return true;
}

bool TizenResourceManagerImpl::AllocateResources(
    ResourceClient* client,
    const std::vector<TizenRequestedResource>& req,
    TizenResourceList* ret) {
  if (req.empty() || req.size() > kMaxResourceRequestSize)
    return false;

  base::AutoLock lock(rm_lock_);

  RegisterIfNeeded();

  rm_category_request_s rm_req;
  rm_req.request_num = 0;

  for (const TizenRequestedResource& req_item : req)
    if (!InternalRequestCheck(req_item) || !AppendToRequest(req_item, &rm_req))
      return false;

  bool success = IssueRequest(client, &rm_req, ret);

  LOG(INFO) << "Client=" << client << " allocation ret=" << success
            << " allocated="
            << TizenResourceManager::IdsToString(
                   TizenResourceManager::ResourceListToIds(ret));

  return success;
}

void TizenResourceManagerImpl::ParseAllocatedDevices(
    ResourceClient* client,
    const rm_device_return_s* allocated,
    TizenResourceList* ret) {
  for (int i = 0; i < allocated->allocated_num; ++i) {
    int id = allocated->device_id[i];
    std::unique_ptr<TizenAllocatedResource> res(new TizenAllocatedResource(
        id, allocated->device_node[i], allocated->omx_comp_name[i]));
    ret->push_back(std::move(res));

    // We must release it on our side. See comment for rm_allocate_resources().
    free(allocated->device_node[i]);
    free(allocated->omx_comp_name[i]);

    DCHECK(client_map_.find(id) == client_map_.end());
    client_map_[id] = AllocatedResource(client);
  }
}

bool TizenResourceManagerImpl::IssueRequest(ResourceClient* client,
                                            const rm_category_request_s* req,
                                            TizenResourceList* ret) {
  if (rm_handle_ == -1)
    return false;

  rm_device_return_s devs;
  devs.allocated_num = 0;

  int err;
  {
    // We must release lock as RM could issue resource conflict callback
    // if this allocation would cause a conflict.
    // Currently RM won't issue a conflict callback to a registered user if
    // resource in question is allocated by the same user. However if this
    // policy will change for some reason, we must be safe from deadlock.
    base::AutoUnlock unlock(rm_lock_);
    err = rm_allocate_resources(rm_handle_, req, &devs);
  }

  if (err == RM_ERROR) {
    LOG(ERROR) << "Failed on rm_allocate_resources ret=" << ret
               << " error=" << devs.error_type;
    return false;
  }

  if (!devs.allocated_num) {
    LOG(ERROR) << "No devices were allocated despite successful allocation.";
    return false;
  }

  if (devs.allocated_num != req->request_num) {
    LOG(ERROR) << "RM allocated different (requested=" << devs.allocated_num
               << " got=" << req->request_num
               << ") than requested. Freeing all"
                  " resources that were allocated.";
    std::vector<int> to_release(devs.allocated_num);
    for (int i = 0; i < devs.allocated_num; ++i)
      to_release[i] = devs.device_id[i];
    DoReleaseResources(client, to_release);
    return false;
  }

  ParseAllocatedDevices(client, &devs, ret);

  return true;
}

bool TizenResourceManagerImpl::DoReleaseResource(const ResourceClient* client,
                                                 int id) {
  std::vector<int> ids{id};
  return DoReleaseResources(client, ids);
}

bool TizenResourceManagerImpl::DoReleaseResources(const ResourceClient* client,
                                                  const std::vector<int>& ids) {
  rm_device_request_s req;
  req.request_num = 0;
  for (const int& id : ids)
    req.device_id[req.request_num++] = id;

  int err = rm_deallocate_resources(rm_handle_, &req);

  for (const int& id : ids) {
    if (err != RM_OK) {
      LOG(ERROR) << "Failed on releasing resource id=" << id
                 << " for client=" << client;
      continue;
    }
    std::map<int, AllocatedResource>::iterator it = client_map_.find(id);
    if (it == client_map_.end()) {
      LOG(ERROR) << "Releasing resource that seems to not be allocated before"
                 << " id=" << id;
      continue;
    } else {
      LOG(INFO) << "Released resource id=" << id << " for client=" << client;
      client_map_.erase(it);
    }
  }

  return (err == RM_OK);
}

bool TizenResourceManagerImpl::ReleaseResources(ResourceClient* client,
                                                const std::vector<int>& ids) {
  if (ids.empty() || ids.size() > kMaxResourceRequestSize)
    return false;

  base::AutoLock lock(rm_lock_);

  if (rm_handle_ == -1) {
    LOG(ERROR) << "Resource release request while not registered in RM.";
    return false;
  }

  bool success = DoReleaseResources(client, ids);

  LOG(INFO) << "Client=" << client << " release success=" << success
            << " released=" << TizenResourceManager::IdsToString(ids);

  return success;
}

TizenResourceManagerImpl* TizenResourceManagerImpl::GetInstance() {
  return base::Singleton<TizenResourceManagerImpl>::get();
}

}  // namespace content
