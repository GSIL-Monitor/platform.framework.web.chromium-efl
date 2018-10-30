// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/notifications/notification_manager.h"

#include <utility>

#include "base/lazy_instance.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_local.h"
#include "content/child/notifications/notification_data_conversions.h"
#include "content/child/notifications/notification_dispatcher.h"
#include "content/child/service_worker/web_service_worker_registration_impl.h"
#include "content/child/thread_safe_sender.h"
#include "content/public/common/notification_resources.h"
#include "content/public/common/platform_notification_data.h"
#include "third_party/WebKit/public/platform/URLConversion.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/modules/notifications/WebNotificationDelegate.h"
#include "url/origin.h"

using blink::WebString;

namespace content {
namespace {

int CurrentWorkerId() {
  return WorkerThread::GetCurrentId();
}

NotificationResources ToNotificationResources(
    std::unique_ptr<blink::WebNotificationResources> web_resources) {
  NotificationResources resources;
  resources.image = web_resources->image;
  resources.notification_icon = web_resources->icon;
  resources.badge = web_resources->badge;
  for (const auto& action_icon : web_resources->action_icons)
    resources.action_icons.push_back(action_icon);
  return resources;
}

}  // namespace

static base::LazyInstance<base::ThreadLocalPointer<NotificationManager>>::Leaky
    g_notification_manager_tls = LAZY_INSTANCE_INITIALIZER;

NotificationManager::ActiveNotificationData::ActiveNotificationData(
    blink::WebNotificationDelegate* delegate,
    const GURL& origin,
    const std::string& tag)
    : delegate(delegate), origin(origin), tag(tag) {}

NotificationManager::ActiveNotificationData::~ActiveNotificationData() {}

NotificationManager::NotificationManager(
    ThreadSafeSender* thread_safe_sender,
    NotificationDispatcher* notification_dispatcher)
    : thread_safe_sender_(thread_safe_sender),
      notification_dispatcher_(notification_dispatcher) {
  g_notification_manager_tls.Pointer()->Set(this);
}

NotificationManager::~NotificationManager() {
  g_notification_manager_tls.Pointer()->Set(nullptr);
}

NotificationManager* NotificationManager::ThreadSpecificInstance(
    ThreadSafeSender* thread_safe_sender,
    NotificationDispatcher* notification_dispatcher) {
  if (g_notification_manager_tls.Pointer()->Get())
    return g_notification_manager_tls.Pointer()->Get();

  NotificationManager* manager =
      new NotificationManager(thread_safe_sender, notification_dispatcher);
  if (CurrentWorkerId())
    WorkerThread::AddObserver(manager);
  return manager;
}

void NotificationManager::WillStopCurrentWorkerThread() {
  delete this;
}

void NotificationManager::Show(
    const blink::WebSecurityOrigin& origin,
    const blink::WebNotificationData& notification_data,
    std::unique_ptr<blink::WebNotificationResources> notification_resources,
    blink::WebNotificationDelegate* delegate) {
  DCHECK_EQ(0u, notification_data.actions.size());
  DCHECK_EQ(0u, notification_resources->action_icons.size());

  GURL origin_gurl = url::Origin(origin).GetURL();

  int notification_id =
      notification_dispatcher_->GenerateNotificationId(CurrentWorkerId());

  active_page_notifications_[notification_id] = ActiveNotificationData(
      delegate, origin_gurl,
      notification_data.tag.Utf8(
          WebString::UTF8ConversionMode::kStrictReplacingErrorsWithFFFD));

  // TODO(mkwst): This is potentially doing the wrong thing with unique
  // origins. Perhaps also 'file:', 'blob:' and 'filesystem:'. See
  // https://crbug.com/490074 for detail.
  thread_safe_sender_->Send(new PlatformNotificationHostMsg_Show(
      notification_id, origin_gurl,
      ToPlatformNotificationData(notification_data),
      ToNotificationResources(std::move(notification_resources))));
}

void NotificationManager::ShowPersistent(
    const blink::WebSecurityOrigin& origin,
    const blink::WebNotificationData& notification_data,
    std::unique_ptr<blink::WebNotificationResources> notification_resources,
    blink::WebServiceWorkerRegistration* service_worker_registration,
    std::unique_ptr<blink::WebNotificationShowCallbacks> callbacks) {
  DCHECK(service_worker_registration);
  DCHECK_EQ(notification_data.actions.size(),
            notification_resources->action_icons.size());

  int64_t service_worker_registration_id =
      static_cast<WebServiceWorkerRegistrationImpl*>(
          service_worker_registration)
          ->RegistrationId();

  // Verify that the author-provided payload size does not exceed our limit.
  // This is an implementation-defined limit to prevent abuse of notification
  // data as a storage mechanism. A UMA histogram records the requested sizes,
  // which enables us to track how much data authors are attempting to store.
  //
  // If the size exceeds this limit, reject the showNotification() promise. This
  // is outside of the boundaries set by the specification, but it gives authors
  // an indication that something has gone wrong.
  size_t author_data_size = notification_data.data.size();

  UMA_HISTOGRAM_COUNTS_1000("Notifications.AuthorDataSize", author_data_size);

  if (author_data_size > PlatformNotificationData::kMaximumDeveloperDataSize) {
    callbacks->OnError();
    return;
  }

  // TODO(peter): GenerateNotificationId is more of a request id. Consider
  // renaming the method in the NotificationDispatcher if this makes sense.
  int request_id =
      notification_dispatcher_->GenerateNotificationId(CurrentWorkerId());

  pending_show_notification_requests_.AddWithID(std::move(callbacks),
                                                request_id);

  // TODO(mkwst): This is potentially doing the wrong thing with unique
  // origins. Perhaps also 'file:', 'blob:' and 'filesystem:'. See
  // https://crbug.com/490074 for detail.
  thread_safe_sender_->Send(new PlatformNotificationHostMsg_ShowPersistent(
      request_id, service_worker_registration_id, url::Origin(origin).GetURL(),
      ToPlatformNotificationData(notification_data),
      ToNotificationResources(std::move(notification_resources))));
}

void NotificationManager::GetNotifications(
    const blink::WebString& filter_tag,
    blink::WebServiceWorkerRegistration* service_worker_registration,
    std::unique_ptr<blink::WebNotificationGetCallbacks> callbacks) {
  DCHECK(service_worker_registration);
  DCHECK(callbacks);

  WebServiceWorkerRegistrationImpl* service_worker_registration_impl =
      static_cast<WebServiceWorkerRegistrationImpl*>(
          service_worker_registration);

  GURL origin = GURL(service_worker_registration_impl->Scope()).GetOrigin();
  int64_t service_worker_registration_id =
      service_worker_registration_impl->RegistrationId();

  // TODO(peter): GenerateNotificationId is more of a request id. Consider
  // renaming the method in the NotificationDispatcher if this makes sense.
  int request_id =
      notification_dispatcher_->GenerateNotificationId(CurrentWorkerId());

  pending_get_notification_requests_.AddWithID(std::move(callbacks),
                                               request_id);

  thread_safe_sender_->Send(new PlatformNotificationHostMsg_GetNotifications(
      request_id, service_worker_registration_id, origin,
      filter_tag.Utf8(
          WebString::UTF8ConversionMode::kStrictReplacingErrorsWithFFFD)));
}

void NotificationManager::Close(blink::WebNotificationDelegate* delegate) {
  for (auto& iter : active_page_notifications_) {
    if (iter.second.delegate != delegate)
      continue;

    thread_safe_sender_->Send(new PlatformNotificationHostMsg_Close(
        iter.second.origin, iter.second.tag, iter.first));
    active_page_notifications_.erase(iter.first);
    return;
  }

  // It should not be possible for Blink to call close() on a Notification which
  // does not exist in either the pending or active notification lists.
  NOTREACHED();
}

void NotificationManager::ClosePersistent(
    const blink::WebSecurityOrigin& origin,
    const blink::WebString& tag,
    const blink::WebString& notification_id) {
  thread_safe_sender_->Send(new PlatformNotificationHostMsg_ClosePersistent(
      // TODO(mkwst): This is potentially doing the wrong thing with unique
      // origins. Perhaps also 'file:', 'blob:' and 'filesystem:'. See
      // https://crbug.com/490074 for detail.
      url::Origin(origin).GetURL(),
      tag.Utf8(WebString::UTF8ConversionMode::kStrictReplacingErrorsWithFFFD),
      notification_id.Utf8(
          WebString::UTF8ConversionMode::kStrictReplacingErrorsWithFFFD)));
}

void NotificationManager::NotifyDelegateDestroyed(
    blink::WebNotificationDelegate* delegate) {
  for (auto& iter : active_page_notifications_) {
    if (iter.second.delegate != delegate)
      continue;

    active_page_notifications_.erase(iter.first);
    return;
  }
}

bool NotificationManager::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(NotificationManager, message)
    IPC_MESSAGE_HANDLER(PlatformNotificationMsg_DidShow, OnDidShow);
    IPC_MESSAGE_HANDLER(PlatformNotificationMsg_DidShowPersistent,
                        OnDidShowPersistent)
    IPC_MESSAGE_HANDLER(PlatformNotificationMsg_DidClose, OnDidClose);
    IPC_MESSAGE_HANDLER(PlatformNotificationMsg_DidClick, OnDidClick);
    IPC_MESSAGE_HANDLER(PlatformNotificationMsg_DidGetNotifications,
                        OnDidGetNotifications)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void NotificationManager::OnDidShow(int notification_id) {
  const auto& iter = active_page_notifications_.find(notification_id);
  if (iter == active_page_notifications_.end())
    return;

  iter->second.delegate->DispatchShowEvent();
}

void NotificationManager::OnDidShowPersistent(int request_id, bool success) {
  blink::WebNotificationShowCallbacks* callbacks =
      pending_show_notification_requests_.Lookup(request_id);
  DCHECK(callbacks);

  if (!callbacks)
    return;

  if (success)
    callbacks->OnSuccess();
  else
    callbacks->OnError();

  pending_show_notification_requests_.Remove(request_id);
}

void NotificationManager::OnDidClose(int notification_id) {
  const auto& iter = active_page_notifications_.find(notification_id);
  if (iter == active_page_notifications_.end())
    return;

  iter->second.delegate->DispatchCloseEvent();

  active_page_notifications_.erase(iter);
}

void NotificationManager::OnDidClick(int notification_id) {
  const auto& iter = active_page_notifications_.find(notification_id);
  if (iter == active_page_notifications_.end())
    return;

  iter->second.delegate->DispatchClickEvent();
}

void NotificationManager::OnDidGetNotifications(
    int request_id,
    const std::vector<PersistentNotificationInfo>& notification_infos) {
  blink::WebNotificationGetCallbacks* callbacks =
      pending_get_notification_requests_.Lookup(request_id);
  DCHECK(callbacks);
  if (!callbacks)
    return;

  blink::WebVector<blink::WebPersistentNotificationInfo> notifications(
      notification_infos.size());

  for (size_t i = 0; i < notification_infos.size(); ++i) {
    blink::WebPersistentNotificationInfo web_notification_info;
    web_notification_info.notification_id =
        blink::WebString::FromUTF8(notification_infos[i].first);
    web_notification_info.data =
        ToWebNotificationData(notification_infos[i].second);

    notifications[i] = web_notification_info;
  }

  callbacks->OnSuccess(notifications);

  pending_get_notification_requests_.Remove(request_id);
}

}  // namespace content
