// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_CONTROLLER_EFL_H
#define NOTIFICATION_CONTROLLER_EFL_H

#include <map>

#include <Eina.h>

#include "base/containers/id_map.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/platform_notification_service.h"
#include "public/ewk_notification_internal.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"
#include "url/gurl.h"

class EWebView;
class SkBitmap;

namespace leveldb {
class DB;
}

namespace content {
class WebContents;

struct NotificationData {
  const GURL origin;
  const std::string non_persistent_id;

  NotificationData(const GURL& origin, const std::string& non_persistent_id);
  ~NotificationData();
};

struct PersistentNotificationData {
  BrowserContext* browser_context_;
  GURL origin_;
  const std::string persistent_id_;
  PersistentNotificationData(BrowserContext* browser_context,
                             GURL origin,
                             const std::string& persistent_id);
  ~PersistentNotificationData();
};

class NotificationControllerEfl : public PlatformNotificationService {
 public:
  NotificationControllerEfl();
  ~NotificationControllerEfl() override;

  void NotificationAdd(uint64_t notification_id,
                       const GURL& origin,
                       const base::string16& replace_id);

  bool NotificationClosed(uint64_t notification_id, bool by_user);

  void NotificationCancelled(uint64_t notification_id);

  // Notify engine when user clicked on the notification
  bool NotificationClicked(uint64_t notification_id);

  // Notification engine that notification was displayed
  bool NotificationDisplayed(uint64_t notification_id);

  bool PersistentNotificationClicked(BrowserContext* browser_context,
                                     const char* id,
                                     const char* origin);

  // sets the permission for a particular pending notification
  void SetPermissionForNotification(
      Ewk_Notification_Permission_Request* notification,
      bool allowed);

  // Adds permission to map
  void PutPermission(const GURL& origin, bool allowed);

  // Removes all stored permissions
  void ClearPermissions();

  // Removes stored permissions for given origins
  void RemovePermissions(Eina_List* origins);

  void RequestPermission(
      WebContents* web_contents,
      const GURL& requesting_frame,
      const base::Callback<void(blink::mojom::PermissionStatus)>&
          result_callback);

  void SetNotificationCallbacks(Ewk_Notification_Show_Callback show_callback,
                                Ewk_Notification_Cancel_Callback close_callback,
                                void* user_data);

  /* PlatformNotificationService */
  blink::mojom::PermissionStatus CheckPermissionOnUIThread(
      BrowserContext* browser_context,
      const GURL& origin,
      int render_process_id) override;

  // Checks if |origin| has permission to display Web Notifications. This method
  // exists to serve the synchronous IPC required by the Notification.permission
  // JavaScript getter, and should not be used for other purposes. See
  // https://crbug.com/446497 for the plan to deprecate this method.
  // This method must only be called on the IO thread.
  blink::mojom::PermissionStatus CheckPermissionOnIOThread(
      ResourceContext* resource_context,
      const GURL& origin,
      int render_process_id) override;

  // Displays the notification described in |params| to the user. A closure
  // through which the notification can be closed will be stored in the
  // |cancel_callback| argument. This method must be called on the UI thread.
  void DisplayNotification(
      BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& origin,
      const PlatformNotificationData& notification_data,
      const NotificationResources& notification_resources,
      base::Closure* cancel_callback) override;

  // Displays the persistent notification described in |notification_data| to
  // the user. This method must be called on the UI thread.
  void DisplayPersistentNotification(
      BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& service_worker_origin,
      const GURL& origin,
      const PlatformNotificationData& notification_data,
      const NotificationResources& notification_resources) override;

  // Closes the persistent notification identified by
  // |persistent_notification_id|. This method must be called on the UI thread.
  void ClosePersistentNotification(BrowserContext* browser_context,
                                   const std::string& notification_id) override;

  // Retrieves the ids of all currently displaying notifications and
  // posts |callback| with the result.
  void GetDisplayedNotifications(
      BrowserContext* browser_context,
      const DisplayedNotificationsCallback& callback) override;

#if defined(OS_TIZEN)
  uint64_t FindNotificationID(int notification_priv_id);
  static void RemoveImageFiles(const std::string& tag);
#endif

 private:
  bool InitializePermissionDatabase(bool clear = false);
  blink::mojom::PermissionStatus CheckPermissionForOrigin(const GURL& origin);

  bool IsNotificationPresent(const GURL& origin,
                             const std::string& notification_id,
                             uint64_t& replace_unique_id);

  bool IsPersistentNotificationPresent(const GURL& origin,
                                       const std::string& notification_id,
                                       uint64_t& replace_unique_id);


  bool DefaultShowCallback(uint64_t tag,
                           const std::string& notification_id,
                           const GURL& origin,
                           const char* title,
                           const char* body,
                           const SkBitmap& icon,
                           const SkBitmap& badge,
                           bool is_persistent);
  bool DefaultCancelCallback(uint64_t tag);

  // This stores the notifications displayed to the user.
  base::IDMap<NotificationData*> notifications_map_;
  base::IDMap<PersistentNotificationData*> persistent_notification_map_;

  std::map<GURL, bool> permissions_map_;
  mutable base::Lock permissions_mutex_;

  Ewk_Notification_Show_Callback notification_show_callback_;
  Ewk_Notification_Cancel_Callback notification_cancel_callback_;
  void* notification_callback_user_data_;

  std::unique_ptr<leveldb::DB> permissions_db_;

  std::map<uint64_t, int> key_mapper_;
  EWebView* web_view_;
  base::WeakPtrFactory<NotificationControllerEfl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NotificationControllerEfl);
};

}  // namespace content

#endif  // NOTIFICATION_CONTROLLER_EFL_H
