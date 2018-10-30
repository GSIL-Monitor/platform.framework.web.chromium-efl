// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/notification/notification_controller_efl.h"

#include "base/files/file_enumerator.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "common/web_contents_utils.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_event_dispatcher.h"
#include "content/public/common/notification_resources.h"
#include "content/public/common/persistent_notification_status.h"
#include "content/public/common/platform_notification_data.h"
#include "eweb_view.h"
#include "private/ewk_notification_private.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "ui/gfx/codec/png_codec.h"

#if defined(OS_TIZEN)
#include <app_control_internal.h>
#include <app_manager.h>
#include <notification_internal.h>
#endif

using web_contents_utils::WebViewFromWebContents;
using blink::mojom::PermissionStatus;

namespace content {

static const char kPermissionDatabaseName[] = "permissions_db";

void OnEventDispatchComplete(PersistentNotificationStatus status) {
  LOG(INFO) << "Notifications.PersistentWebNotificationClickResult " << status;
}

#if defined(OS_TIZEN)
bool SetNotificationImage(notification_h noti,
                          notification_image_type_e type,
                          const SkBitmap& bitmap,
                          const base::FilePath& path) {
  std::vector<unsigned char> data;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false, &data)) {
    LOG(ERROR) << "Bitmap color type " << bitmap.colorType();
    return false;
  }

  FILE* f = base::OpenFile(path, "wb");
  if (!f)
    return false;

  size_t written = fwrite(&*data.begin(), 1, data.size(), f);
  base::CloseFile(f);
  if (written == data.size()) {
    if (notification_set_image(noti, type, path.value().data()) ==
        NOTIFICATION_ERROR_NONE) {
      return true;
    }
    LOG(ERROR) << "Failed to set notification image." << path.value();
  } else {
    LOG(ERROR) << "Failed to write image file. " << path.value()
               << ", written size:" << written << "/" << data.size();
  }
  base::DeleteFile(path, false);
  return false;
}
#endif  // defined(OS_TIZEN)

NotificationData::NotificationData(const GURL& origin,
                                   const std::string& non_persistent_id)
    : origin(origin),
      non_persistent_id(non_persistent_id) {}

NotificationData::~NotificationData() {}

PersistentNotificationData::PersistentNotificationData(
    BrowserContext* browser_context,
    GURL origin,
    const std::string& persistent_id)
    : browser_context_(browser_context),
      origin_(origin),
      persistent_id_(persistent_id) {}

PersistentNotificationData::~PersistentNotificationData() {}

NotificationControllerEfl::NotificationControllerEfl()
    : notification_show_callback_(nullptr),
      notification_cancel_callback_(nullptr),
      notification_callback_user_data_(nullptr),
      web_view_(nullptr),
      weak_factory_(this) {}

NotificationControllerEfl::~NotificationControllerEfl() {
#if defined(OS_TIZEN)
  NotificationControllerEfl::RemoveImageFiles(std::string());
#endif
  notifications_map_.Clear();
  persistent_notification_map_.Clear();
  key_mapper_.clear();
}

bool NotificationControllerEfl::InitializePermissionDatabase(bool clear) {
  if (permissions_db_)
    return true;

  base::FilePath db_path;
  if (!PathService::Get(PathsEfl::WEB_DATABASE_DIR, &db_path)) {
    LOG(ERROR) << "Could not get web database directory.";
    return false;
  }

  db_path = db_path.Append(FILE_PATH_LITERAL(kPermissionDatabaseName));
  if (clear)
    base::DeleteFile(db_path, true);

  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  leveldb::Status status =
      leveldb::DB::Open(options, db_path.AsUTF8Unsafe(), &db);
  if (!status.ok()) {
    LOG(ERROR) << "Could not open permissions db. " << db_path.AsUTF8Unsafe();
    return false;
  }

  permissions_db_.reset(db);
  // Load permissions from db.
  if (!clear) {
    std::unique_ptr<leveldb::Iterator> it(
        permissions_db_->NewIterator(leveldb::ReadOptions()));
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
      GURL origin(it->key().ToString());
      bool allowed = *(const bool*)(it->value().data());
      permissions_map_[origin] = allowed;
    }
  }

  return (permissions_db_ != nullptr);
}

bool NotificationControllerEfl::NotificationClosed(uint64_t notification_id,
                                                   bool by_user) {
  NotificationData* saved_data = notifications_map_.Lookup(notification_id);
  if (saved_data) {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNonPersistentCloseEvent(saved_data->non_persistent_id);
    notifications_map_.Remove(notification_id);
    return true;
  }

  auto saved_persistent_notification_data =
      persistent_notification_map_.Lookup(notification_id);
  if (saved_persistent_notification_data) {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNotificationCloseEvent(
            saved_persistent_notification_data->browser_context_,
            saved_persistent_notification_data->persistent_id_,
            saved_persistent_notification_data->origin_,
            by_user, base::Bind(OnEventDispatchComplete));
    persistent_notification_map_.Remove(notification_id);
    return true;
  }

  return false;
}

void NotificationControllerEfl::NotificationCancelled(
    uint64_t notification_id) {
  NotificationClosed(notification_id, false);
  if (notification_cancel_callback_) {
    notification_cancel_callback_(notification_id,
                                  notification_callback_user_data_);
  } else {
    DefaultCancelCallback(notification_id);
  }
}

bool NotificationControllerEfl::PersistentNotificationClicked(
    BrowserContext* browser_context,
    const char* id,
    const char* origin) {
  if (!browser_context)
    return false;

  content::NotificationEventDispatcher::GetInstance()
      ->DispatchNotificationClickEvent(
          browser_context, std::string(id), GURL(base::StringPiece(origin)),
          base::Optional<int>(), base::Optional<base::string16>(),
          base::Bind(OnEventDispatchComplete));
  // TODO: Currently if we click notification in quick panel, it will be
  // deleted automatically by platform. If we can keep the notification
  // after click, we don't need to call |DispatchNotificationCloseEvent| here.
  content::NotificationEventDispatcher::GetInstance()
      ->DispatchNotificationCloseEvent(
            browser_context, std::string(id), GURL(base::StringPiece(origin)),
            false, base::Bind(OnEventDispatchComplete));
  return true;
}

bool NotificationControllerEfl::NotificationClicked(uint64_t notification_id) {
  auto saved_persistent_notification_data =
      persistent_notification_map_.Lookup(notification_id);
  if (saved_persistent_notification_data) {
    persistent_notification_map_.Remove(notification_id);
    return true;
  }

  auto saved_notification_data = notifications_map_.Lookup(notification_id);
  if (saved_notification_data) {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNonPersistentClickEvent(
            saved_notification_data->non_persistent_id);

    // TODO: Currently if we click notification in quick panel, it will be
    // deleted automatically by platform. If we can keep the notification
    // after click, we don't need to call |NotificationClosed| here.
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNonPersistentCloseEvent(
            saved_notification_data->non_persistent_id);
    notifications_map_.Remove(notification_id);
    return true;
  }

  return false;
}

bool NotificationControllerEfl::NotificationDisplayed(
    uint64_t notification_id) {
  auto saved_notification_data = notifications_map_.Lookup(notification_id);
  if (!saved_notification_data)
    return false;

  content::NotificationEventDispatcher::GetInstance()
      ->DispatchNonPersistentShowEvent(
          saved_notification_data->non_persistent_id);
  return true;
}

PermissionStatus NotificationControllerEfl::CheckPermissionOnUIThread(
    BrowserContext* browser_context,
    const GURL& origin,
    int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return CheckPermissionForOrigin(origin);
}

PermissionStatus NotificationControllerEfl::CheckPermissionOnIOThread(
    ResourceContext* resource_context,
    const GURL& origin,
    int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return CheckPermissionForOrigin(origin);
}

void NotificationControllerEfl::DisplayNotification(
    BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    const PlatformNotificationData& notification_data,
    const NotificationResources& notification_resources,
    base::Closure* cancel_callback) {
  BrowserContextEfl* browser_context_efl =
      static_cast<BrowserContextEfl*>(browser_context);
  CHECK(browser_context_efl);
  EWebContext* ctx = browser_context_efl->WebContext();
  CHECK(ctx);

  bool has_callbacks =
      ctx->HasNotificationCallbacks() ||
      (notification_show_callback_ && notification_cancel_callback_);

  uint64_t replace_unique_id = 0;
  if (IsNotificationPresent(origin, notification_id, replace_unique_id)) {
    if (!has_callbacks)
      DefaultCancelCallback(replace_unique_id);
    else if (!ctx->NotificationCancelCallback(replace_unique_id))
      NotificationCancelled(replace_unique_id);
    NotificationClosed(replace_unique_id, false);
  }

  uint64_t notification_unique_id = base::Hash(notification_id);

  NotificationData* new_notification(
      new NotificationData(origin, notification_id));
  notifications_map_.AddWithID(new_notification, notification_unique_id);

  if (cancel_callback) {
    *cancel_callback =
        base::Bind(&NotificationControllerEfl::NotificationCancelled,
                   weak_factory_.GetWeakPtr(), notification_unique_id);
  }

  Ewk_Notification notification(
      base::UTF16ToUTF8(notification_data.body),
      base::UTF16ToUTF8(notification_data.title),
      notification_resources.notification_icon, notification_data.silent,
      notification_unique_id, origin);

  if (!has_callbacks) {
    DefaultShowCallback(notification_unique_id, notification_id, origin,
                        notification.GetTitle(), notification.GetBody(),
                        notification_resources.notification_icon,
                        notification_resources.badge, false);
    NotificationDisplayed(notification_unique_id);
  } else if (!ctx->NotificationShowCallback(&notification)) {
    notification_show_callback_(&notification,
                                notification_callback_user_data_);
  }
}

void NotificationControllerEfl::DisplayPersistentNotification(
    BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& service_worker_origin,
    const GURL& origin,
    const PlatformNotificationData& notification_data,
    const NotificationResources& notification_resources) {
  BrowserContextEfl* browser_context_efl =
      static_cast<BrowserContextEfl*>(browser_context);
  CHECK(browser_context_efl);
  EWebContext* ctx = browser_context_efl->WebContext();
  CHECK(ctx);

  bool has_callbacks =
      ctx->HasNotificationCallbacks() ||
      (notification_show_callback_ && notification_cancel_callback_);

  uint64_t replace_unique_id = 0;
  if (IsPersistentNotificationPresent(
        origin, notification_id, replace_unique_id)) {
    if (!has_callbacks)
      DefaultCancelCallback(replace_unique_id);
    else if (!ctx->NotificationCancelCallback(replace_unique_id))
      NotificationCancelled(replace_unique_id);
    NotificationClosed(replace_unique_id, false);
  }

  uint64_t notification_unique_id = base::Hash(notification_id);

  PersistentNotificationData* persistent_notification_data(
      new PersistentNotificationData(browser_context, origin, notification_id));

  persistent_notification_map_.AddWithID(persistent_notification_data,
                                         notification_unique_id);

  Ewk_Notification notification(
      base::UTF16ToUTF8(notification_data.body),
      base::UTF16ToUTF8(notification_data.title),
      notification_resources.notification_icon, notification_data.silent,
      notification_unique_id, origin);

  if (!has_callbacks) {
    DefaultShowCallback(notification_unique_id, notification_id, origin,
                        notification.GetTitle(), notification.GetBody(),
                        notification_resources.notification_icon,
                        notification_resources.badge, true);
  } else if (!ctx->NotificationShowCallback(&notification)) {
    notification_show_callback_(&notification,
                                notification_callback_user_data_);
  }
}

void NotificationControllerEfl::ClosePersistentNotification(
    BrowserContext* browser_context,
    const std::string& notification_id) {
  NOTIMPLEMENTED();
}

void NotificationControllerEfl::GetDisplayedNotifications(
      BrowserContext* browser_context,
      const DisplayedNotificationsCallback& callback) {
  NOTIMPLEMENTED();
}

void NotificationControllerEfl::SetPermissionForNotification(
    Ewk_Notification_Permission_Request* notification,
    bool allowed) {
  GURL origin = notification->GetSecurityOrigin()->GetURL();

  if (web_view_) {
    Ewk_Notification_Permission permission;
    permission.origin = origin.possibly_invalid_spec().c_str();
    permission.allowed = allowed;
    web_view_->SmartCallback<EWebViewCallbacks::NotificationPermissionReply>()
        .call(&permission);
  }

  // Save decision in permissions map.
  PutPermission(origin, allowed);
}

void NotificationControllerEfl::PutPermission(const GURL& origin,
                                              bool allowed) {
  base::AutoLock locker(permissions_mutex_);
  auto it = permissions_map_.find(origin);
  if (it == permissions_map_.end() || it->second != allowed) {
    permissions_map_[origin] = allowed;
    if (InitializePermissionDatabase()) {
      permissions_db_->Put(leveldb::WriteOptions(),
                           origin.possibly_invalid_spec(),
                           leveldb::Slice((const char*)&allowed, sizeof(bool)));
    }
  }
}

PermissionStatus NotificationControllerEfl::CheckPermissionForOrigin(
    const GURL& origin) {
  base::AutoLock locker(permissions_mutex_);
  InitializePermissionDatabase();
  PermissionStatus status = PermissionStatus::ASK;
  auto it = permissions_map_.find(origin);
  if (it != permissions_map_.end())
    status = it->second ? PermissionStatus::GRANTED : PermissionStatus::DENIED;
  return status;
}

void NotificationControllerEfl::ClearPermissions() {
  base::AutoLock locker(permissions_mutex_);
  permissions_map_.clear();
  permissions_db_.reset();
  InitializePermissionDatabase(true);
}

void NotificationControllerEfl::RemovePermissions(Eina_List* origins) {
  Eina_List* list = nullptr;
  void* data = nullptr;
  base::AutoLock locker(permissions_mutex_);
  if (eina_list_count(origins) > 0)
    InitializePermissionDatabase();

  EINA_LIST_FOREACH(origins, list, data) {
    _Ewk_Security_Origin* origin = static_cast<_Ewk_Security_Origin*>(data);
    if (permissions_map_.erase(origin->GetURL()) && permissions_db_) {
      permissions_db_->Delete(leveldb::WriteOptions(),
                              origin->GetURL().possibly_invalid_spec());
    }
  }
}

bool NotificationControllerEfl::IsNotificationPresent(
    const GURL& origin,
    const std::string& notification_id,
    uint64_t& replace_unique_id) {
  base::IDMap<NotificationData*>::const_iterator it(&notifications_map_);
  for (; !it.IsAtEnd(); it.Advance()) {
    if (notification_id == it.GetCurrentValue()->non_persistent_id &&
        origin == it.GetCurrentValue()->origin) {
      replace_unique_id = it.GetCurrentKey();
      return true;
    }
  }

  return false;
}

bool NotificationControllerEfl::IsPersistentNotificationPresent(
    const GURL& origin,
    const std::string& notification_id,
    uint64_t& replace_unique_id) {
  base::IDMap<PersistentNotificationData*>::const_iterator it(
      &persistent_notification_map_);
  for (; !it.IsAtEnd(); it.Advance()) {
    if (notification_id == it.GetCurrentValue()->persistent_id_ &&
        origin == it.GetCurrentValue()->origin_) {
      replace_unique_id = it.GetCurrentKey();
      return true;
    }
  }

  return false;
}


void NotificationControllerEfl::RequestPermission(
    WebContents* web_contents,
    const GURL& requesting_frame,
    const base::Callback<void(PermissionStatus)>& result_callback) {
  web_view_ = WebViewFromWebContents(web_contents);
  if (!web_view_) {
    LOG(ERROR) << "Dropping PermissionNotification request caused by lack "
                  "of the WebView.";
    result_callback.Run(PermissionStatus::DENIED);
    return;
  }

  std::unique_ptr<Ewk_Notification_Permission_Request> notification_permission(
      new Ewk_Notification_Permission_Request(result_callback,
                                              requesting_frame));

  if (!web_view_->IsNotificationPermissionCallbackSet()) {
    LOG(ERROR) << "Dropping PermissionNotification request caused by lack "
                  "of the Notification Permission Callback.";
    result_callback.Run(PermissionStatus::DENIED);
    return;
  }

  PermissionStatus web_notification_permission =
      CheckPermissionForOrigin(requesting_frame);

  switch (web_notification_permission) {
    case PermissionStatus::LAST:
      web_view_->InvokeNotificationPermissionCallback(
          notification_permission.get());
      // If policy is suspended, the API takes over the policy object lifetime
      // and policy will be deleted after decision is made.
      if (notification_permission->IsSuspended())
        ignore_result(notification_permission.release());
      break;
    case PermissionStatus::GRANTED:
      result_callback.Run(PermissionStatus::GRANTED);
      break;
    default:
      result_callback.Run(PermissionStatus::DENIED);
  }
}

void NotificationControllerEfl::SetNotificationCallbacks(
    Ewk_Notification_Show_Callback show_callback,
    Ewk_Notification_Cancel_Callback cancel_callback,
    void* user_data) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  notification_show_callback_ = show_callback;
  notification_cancel_callback_ = cancel_callback;
  notification_callback_user_data_ = user_data;
}

#if defined(OS_TIZEN)
static void NotificationEventCallback(notification_h noti,
                                      int event_type,
                                      void* userdata) {
  auto thiz = static_cast<NotificationControllerEfl*>(userdata);
  int notification_priv_id = NOTIFICATION_PRIV_ID_NONE;
  if (notification_get_id(noti, nullptr, &notification_priv_id) !=
      NOTIFICATION_ERROR_NONE) {
    return;
  }

  uint64_t notification_id = thiz->FindNotificationID(notification_priv_id);
  bool ret = false;
  if (notification_id) {
    if (event_type == NOTIFICATION_EVENT_TYPE_PRESSED) {
      ret = thiz->NotificationClicked(notification_id);
    } else {
      ret = thiz->NotificationClosed(
          notification_id,
          event_type != NOTIFICATION_EVENT_TYPE_HIDDEN_BY_TIMEOUT);
    }
  }

  const char* tag = nullptr;
  int noti_err = notification_get_tag(noti, &tag);
  if (tag && noti_err == NOTIFICATION_ERROR_NONE)
    thiz->RemoveImageFiles(tag);

  // TODO: Need to check the invalid case.
  if (!ret) {
    LOG(ERROR) << "Failed to handle notification, id:" << notification_id
               << ", event_type:" << event_type;
  }
}

uint64_t NotificationControllerEfl::FindNotificationID(
    int notification_priv_id) {
  for (auto it = key_mapper_.begin(); it != key_mapper_.end(); it++) {
    if (it->second == notification_priv_id)
      return it->first;
  }

  LOG(ERROR) << "Can't find notification ID " << notification_priv_id;
  return 0;
}

void NotificationControllerEfl::RemoveImageFiles(const std::string& tag) {
  base::FilePath image_path;
  if (!PathService::Get(PathsEfl::DIR_SHARED_NOTI_ICON, &image_path)) {
    LOG(ERROR) << "Failed to retrieve the notification resource path";
    return;
  }

  if (tag.empty()) {
    // Remove all unused icons.
    base::FileEnumerator enumerator(image_path, false,
                                    base::FileEnumerator::FILES);
    for (base::FilePath file = enumerator.Next(); !file.empty();
         file = enumerator.Next()) {
      const std::string file_str = file.BaseName().value();
      size_t dot_at = file_str.find_last_of('.');
      if (dot_at != std::string::npos) {
        notification_h noti =
            notification_load_by_tag(file_str.substr(0, dot_at).c_str());
        if (noti)
          notification_free(noti);
        else if (!base::DeleteFile(file, false))
          LOG(ERROR) << "Failed to delete icon. " << file_str;
      }
    }
    return;
  }

  if (!base::DeleteFile(image_path.AppendASCII(tag + ".icon"), false)) {
    LOG(ERROR) << "Failed to delete icon. "
               << image_path.AppendASCII(tag).value();
  }
  if (!base::DeleteFile(image_path.AppendASCII(tag + ".badge"), false)) {
    LOG(ERROR) << "Failed to delete badge. "
               << image_path.AppendASCII(tag).value();
  }
}
#endif  // defined(OS_TIZEN)

bool NotificationControllerEfl::DefaultShowCallback(
    uint64_t tag,
    const std::string& notification_id,
    const GURL& origin,
    const char* title,
    const char* body,
    const SkBitmap& icon,
    const SkBitmap& badge,
    bool is_persistent) {
#if defined(OS_TIZEN)
  auto found = key_mapper_.find(tag);
  if (found != key_mapper_.end())
    DefaultCancelCallback(tag);

  notification_h noti_h =
      notification_new(NOTIFICATION_TYPE_NOTI, NOTIFICATION_GROUP_ID_DEFAULT,
                       NOTIFICATION_PRIV_ID_NONE);
  if (!noti_h) {
    LOG(ERROR) << "Can't create notification handle.";
    return false;
  }

  std::unique_ptr<std::remove_pointer<notification_h>::type,
                  decltype(notification_free)*>
      auto_release{noti_h, notification_free};

  // Set notification title.
  int ret = notification_set_text(noti_h, NOTIFICATION_TEXT_TYPE_TITLE, title,
                                  nullptr, NOTIFICATION_VARIABLE_TYPE_NONE);
  if (ret != NOTIFICATION_ERROR_NONE) {
    LOG(ERROR) << "Can't set title" << ret;
    return false;
  }

  // Set notification content.
  ret = notification_set_text(noti_h, NOTIFICATION_TEXT_TYPE_CONTENT, body,
                              nullptr, NOTIFICATION_VARIABLE_TYPE_NONE);
  if (ret != NOTIFICATION_ERROR_NONE) {
    LOG(ERROR) << "Can't set content" << ret;
    return false;
  }

  base::FilePath noti_path;
  if (PathService::Get(PathsEfl::DIR_SHARED_NOTI_ICON, &noti_path)) {
    bool has_images = false;
    const std::string tag_str = std::to_string(tag);

    if (SetNotificationImage(noti_h, NOTIFICATION_IMAGE_TYPE_ICON, icon,
                             noti_path.AppendASCII(tag_str + ".icon"))) {
      has_images = true;
    }
    if (SetNotificationImage(noti_h, NOTIFICATION_IMAGE_TYPE_ICON_FOR_INDICATOR,
                             badge,
                             noti_path.AppendASCII(tag_str + ".badge"))) {
      has_images = true;
    }

    // Set a tag to remove icon images after closing notification.
    if (has_images)
      notification_set_tag(noti_h, tag_str.c_str());
  } else {
    LOG(ERROR) << "Failed to retrieve the notification resource path";
  }

  // Set notification type.
  ret = notification_set_display_applist(
      noti_h, NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY |
                  NOTIFICATION_DISPLAY_APP_TICKER |
                  NOTIFICATION_DISPLAY_APP_INDICATOR);
  if (ret != NOTIFICATION_ERROR_NONE) {
    LOG(ERROR) << "notification_set_display_applist is failed with err " << ret;
    return false;
  }

  if (is_persistent) {
    app_control_h app_control = nullptr;
    int appc_ret = app_control_create(&app_control);
    if (appc_ret != APP_CONTROL_ERROR_NONE) {
      LOG(ERROR) << "app_control_create is failed with err " << appc_ret;
      return false;
    }

    std::unique_ptr<std::remove_pointer<app_control_h>::type,
                    decltype(app_control_destroy)*>
        auto_release{app_control, app_control_destroy};

    char* app_id = nullptr;
    int app_ret = app_manager_get_app_id(getpid(), &app_id);
    if (app_ret != APP_MANAGER_ERROR_NONE) {
      LOG(ERROR) << "app_manager_get_app_id is failed with err " << app_ret;
      return false;
    }

    app_control_set_app_id(app_control, app_id);
    if (app_id)
      free(app_id);

    app_control_add_extra_data(app_control, APP_CONTROL_DATA_BACKGROUND_LAUNCH,
                               "enable");
    app_control_add_extra_data(app_control, "notification_id",
                               notification_id.c_str());
    app_control_add_extra_data(app_control, "notification_origin",
                               origin.possibly_invalid_spec().c_str());

    ret = notification_set_launch_option(
        noti_h, NOTIFICATION_LAUNCH_OPTION_APP_CONTROL, (void*)app_control);
    if (ret != NOTIFICATION_ERROR_NONE) {
      LOG(ERROR) << "notification_set_launch_option is failed with err " << ret;
      return false;
    }

  }
  // TODO: If application has been killed, currently there is no way to launch
  // application to run service worker when notification closed. If platform
  // support new API, we don't need to use |notification_post_with_event_cb|.
  ret = notification_post_with_event_cb(noti_h, NotificationEventCallback,
                                        this);
  if (ret != NOTIFICATION_ERROR_NONE) {
    LOG(ERROR) << "notification_post_with_event_cb is failed with err " << ret;
    return false;
  }

  int notification_priv_id = NOTIFICATION_PRIV_ID_NONE;
  ret = notification_get_id(noti_h, nullptr, &notification_priv_id);
  if (ret != NOTIFICATION_ERROR_NONE) {
    LOG(ERROR) << "Can't get notification ID " << ret;
    return false;
  }

  key_mapper_[tag] = notification_priv_id;
  return true;
#else
  return false;
#endif  // defined(OS_TIZEN)
}

bool NotificationControllerEfl::DefaultCancelCallback(uint64_t tag) {
#if defined(OS_TIZEN)
  auto found = key_mapper_.find(tag);
  if (found == key_mapper_.end()) {
    LOG(ERROR) << "Can't find notification.";
    return false;
  }

  notification_delete_by_priv_id(nullptr, NOTIFICATION_TYPE_NOTI,
                                 found->second);
  key_mapper_.erase(found);
  return true;
#else
  return false;
#endif  // defined(OS_TIZEN)
}

}  // namespace content
