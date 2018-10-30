// Copyright 2014-2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWEB_CONTEXT_H
#define EWEB_CONTEXT_H

#include "base/files/file_path.h"
#include "browser/web_cache_efl/web_cache_manager_efl.h"
#include "private/ewk_cookie_manager_private.h"
#include "public/ewk_context.h"
#include "public/ewk_context_product.h"
#include "public/ewk_notification_internal.h"
#include "devtools_delegate_efl.h"

class CookieManager;
class Ewk_Wrt_Message_Data;
class EwkFaviconDatabase;

namespace content {
class BrowserContextEfl;
struct LocalStorageUsageInfo;
}

class EWebContextNotificationCallback {
 public:
  EWebContextNotificationCallback(
      Ewk_Context* context,
      Ewk_Context_Notification_Show_Callback show_cb,
      Ewk_Context_Notification_Cancel_Callback cancel_cb,
      void* data)
    : context_(context)
    , show_cb_(show_cb)
    , cancel_cb_(cancel_cb)
    , user_data_(data) { }

  bool HasShowCallback() const { return show_cb_ != nullptr; }
  bool HasCancelCallback() const { return cancel_cb_ != nullptr; }
  /* LCOV_EXCL_START */
  bool RunShowCallback(Ewk_Notification* request) {
    if (context_ && HasShowCallback()) {
      show_cb_(context_, request, user_data_);
      return true;
    }
    return false;
  }
  bool RunCancelCallback(uint64_t notification_id) {
    if (context_ && HasCancelCallback()) {
      cancel_cb_(context_, notification_id, user_data_);
      return true;
    }
    return false;
  }
  /* LCOV_EXCL_STOP */

 private:
  Ewk_Context* context_;
  Ewk_Context_Notification_Show_Callback show_cb_;
  Ewk_Context_Notification_Cancel_Callback cancel_cb_;
  void* user_data_;
};

class EwkDidStartDownloadCallback {
 public:
  EwkDidStartDownloadCallback(Ewk_Context_Did_Start_Download_Callback callback,
                              void* user_data)
      : callback_(callback),
        user_data_(user_data)  // LCOV_EXCL_LINE
  {}
  void TriggerCallback(const std::string& url);
 private:
  Ewk_Context_Did_Start_Download_Callback callback_;
  void* user_data_;
};

class EwkMimeOverrideCallback {
 public:
  EwkMimeOverrideCallback(Ewk_Context_Override_Mime_For_Url_Callback callback,
                          void* user_data)
      : callback_(callback), user_data_(user_data) {  // LCOV_EXCL_LINE
  }
  bool TriggerCallback(const std::string& url_spec,
                       const std::string& mime_type,
                       char** new_mime_type) const;
 private:
  Ewk_Context_Override_Mime_For_Url_Callback callback_;
  void* user_data_;
};

#if defined(OS_TIZEN_TV_PRODUCT)
class EwkCheckAccessiblePathCallback {
 public:
  EwkCheckAccessiblePathCallback(
      Ewk_Context_Check_Accessible_Path_Callback callback,
      void* user_data)
      : callback_(callback), user_data_(user_data) {}
  bool TriggerCallback(const std::string& url_spec,
                       const std::string& tizen_app_id) const;

 private:
  Ewk_Context_Check_Accessible_Path_Callback callback_;
  void* user_data_;
};
#endif

class EwkPushMessageCallback {
 public:
  EwkPushMessageCallback(Ewk_Push_Message_Cb callback, void* user_data)
      : callback_(callback), user_data_(user_data) {}
  void Trigger(const std::string& sender_id,
               const std::string& push_data) const;

 private:
  Ewk_Push_Message_Cb callback_;
  void* user_data_;
};

class EWebContext {
 public:
  content::BrowserContextEfl* browser_context() const { return browser_context_.get(); }

  void ClearNetworkCache();
  void ClearWebkitCache();
  void SetCertificatePath(const std::string& certificate_path);
  const std::string& GetCertificatePath() const;

  void SetCacheModel(Ewk_Cache_Model);
  Ewk_Cache_Model GetCacheModel() const;
  void SetNetworkCacheEnable(bool enable);
  bool GetNetworkCacheEnable() const { return cache_enable_; };

  void NotifyLowMemory();

  bool HTTPCustomHeaderAdd(const std::string& name, const std::string& value);
  bool HTTPCustomHeaderRemove(const std::string& name);
  void HTTPCustomHeaderClear();

  Ewk_Cookie_Manager* ewkCookieManager();
  scoped_refptr<CookieManager> cookieManager()
  { return ewkCookieManager()->cookieManager(); }
  void SetProxy(const char* uri, const char* bypass_rule);
  /* LCOV_EXCL_START */
  const char* GetProxyUri() const {
    return proxy_uri_.c_str();
  }
  const char* GetProxyBypassRule() const {
    return proxy_bypass_rule_.c_str();
  }
  /* LCOV_EXCL_STOP */
  // download start callback handlers
  void SetDidStartDownloadCallback(Ewk_Context_Did_Start_Download_Callback callback,
                                   void* user_data);
  EwkDidStartDownloadCallback* DidStartDownloadCallback();
  void DeleteAllApplicationCache();
  void DeleteApplicationCacheForSite(const GURL&);
  void GetAllOriginsWithApplicationCache(Ewk_Web_Application_Cache_Origins_Get_Callback callback,
                                         void* user_data);
  void GetApplicationCacheUsage(
      const GURL& url,
      Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback callback,
      void* user_data);
  void GetLocalStorageUsageForOrigin(
      const GURL& origin,
      Ewk_Web_Storage_Usage_Get_Callback callback,
      void* user_data);
  void GetAllOriginsWithWebDB(Ewk_Web_Database_Origins_Get_Callback callback, void* user_data);
  void WebDBDelete(const GURL& host);
  void GetWebDBUsageForOrigin(const GURL& origin,
                              Ewk_Web_Database_Usage_Get_Callback callback,
                              void* user_data);
  void IndexedDBDelete();
  void WebStorageDelete();
  void WebStorageDeleteForOrigin(const GURL& origin);
  void WebStorageOriginsAllGet(Ewk_Web_Storage_Origins_Get_Callback callback, void* user_data);
  void FileSystemDelete(const GURL& host);
  void GetAllOriginsWithFileSystem(Ewk_Local_File_System_Origins_Get_Callback callback, void* user_data) const;
  bool SetFaviconDatabasePath(const base::FilePath& path);
  Evas_Object *AddFaviconObject(const char *uri, Evas *canvas) const;

  void SetWidgetInfo(const std::string& tizen_app_id, double scale, const std::string &theme, const std::string &encoded_bundle);
  bool SetAppVersion(const std::string& tizen_app_version);
  void SendWrtMessage(const Ewk_Wrt_Message_Data& message);

  void SetMimeOverrideCallback(Ewk_Context_Override_Mime_For_Url_Callback callback,
                               void* user_data);
  bool OverrideMimeForURL(const std::string& url_spec, const std::string& mime_type,
                          std::string& new_mime_type) const;

  void SetPixmap(int pixmap) { m_pixmap = pixmap; }  // LCOV_EXCL_LINE
  int Pixmap() const { return m_pixmap; }

  void ClearCandidateData();
  void ClearPasswordData();
  void ClearPasswordDataForUrl(const char* url);
  void GetPasswordDataList(
      Ewk_Context_Form_Password_Data_List_Get_Callback callback,
      void* user_data);

  EwkFaviconDatabase* GetFaviconDatabase() const {
    return ewk_favicon_database_.get();  // LCOV_EXCL_LINE
  }

  unsigned int InspectorServerStart(unsigned int port);
  bool InspectorServerStop();

  void SetNotificationCallbacks(
      Ewk_Context* context,
      Ewk_Context_Notification_Show_Callback show_callback,
      Ewk_Context_Notification_Cancel_Callback cancel_callback,
      void* user_data);
  bool HasNotificationCallbacks() const;
  bool NotificationShowCallback(Ewk_Notification* notification);
  bool NotificationCancelCallback(uint64_t notification_id);

  const std::string& GetInjectedBundlePath() const { return injected_bundle_path_; }
  /* LCOV_EXCL_START */
  const std::string& GetTizenAppId() const { return tizen_app_id_; }
  const std::string& GetWidgetTheme() const { return widget_theme_; }
  const std::string& GetWidgetEncodedBundle() const { return widget_encoded_bundle_; }
  double GetWidgetScale() const { return widget_scale_; }
  /* LCOV_EXCL_STOP */
  const std::string& GetTizenAppVersion() const { return tizen_app_version_; }

  bool SetExtensibleAPI(const std::string& api_name, bool enable);
  bool GetExtensibleAPI(const std::string& api_name);

  void SetDefaultZoomFactor(double zoom_factor);
  double GetDefaultZoomFactor() const;

  void SetPushMessageCallback(Ewk_Push_Message_Cb callback, void* user_data);
  bool HasPushMessageCallback();
  bool NotifyPushMessage(const std::string& sender_id,
                         const std::string& push_data);
  bool DeliverPushMessage(const std::string& push_data);

#if defined(OS_TIZEN_TV_PRODUCT)
  bool ShouldAllowRequest(const net::URLRequest& request);
  void SetCheckAccessiblePathCallback(
      Ewk_Context_Check_Accessible_Path_Callback callback,
      void* user_data);
  void SetApplicationType(const Ewk_Application_Type application_type);
  Ewk_Application_Type GetApplicationType() const { return application_type_; }
  bool SetPWAInfo(const std::string& pwa_app_id);
  void RegisterURLSchemesAsCORSEnabled(const Eina_List* schemes_list);
  void RegisterJSPluginMimeTypes(const Eina_List*);
  void SetTimeOffset(double time_offset);
  void SetTimeZoneOffset(double time_zone_offset, double daylight_saving_time);

  void SetEMPCertificatePath(const std::string& emp_ca_path,
                             const std::string& default_ca_path);
  // To control applocation only show once inspector port info
  bool ShowInspectorPortInfoState();
  bool GetInspectorServerState();
  void SetMaxURLChars(size_t max_chars);
  void SetInterceptRequestCancelCallback(
      Ewk_Context* ewk_context,
      Ewk_Context_Intercept_Request_Callback callback,
      void* user_data);
  void SetProxyDefaultAuth(const char* username, const char* password);
  const std::string& GetProxyUsername() const { return proxy_username_; }
  const std::string& GetProxyPassword() const { return proxy_password_; }
#endif
  void SetInterceptRequestCallback(
      Ewk_Context* ewk_context,
      Ewk_Context_Intercept_Request_Callback callback,
      void* user_data);

  void SetAudioLatencyMode(Ewk_Audio_Latency_Mode latency_mode);
  Ewk_Audio_Latency_Mode GetAudioLatencyMode() const;
  void EnableAppControl(bool enabled);

  void SetMaxRefreshRate(int max_refresh_rate);

 private:
  EWebContext(bool incognito, const std::string& injectedBundlePath);
  ~EWebContext();
  void SetNetworkCacheEnableOnIOThread(
      bool enable,
      net::URLRequestContextGetter* url_context);

#if defined(OS_TIZEN_TV_PRODUCT)
  // For processes which are preloaded and with unknown app id
  void UpdateStoragePartitionPath();
#endif

  friend class Ewk_Context;
  static void LocalStorageUsageForOrigin(
      const GURL& origin,
      Ewk_Web_Storage_Usage_Get_Callback callback,
      void* user_data,
      const std::vector<content::LocalStorageUsageInfo>& local_storage);

  std::unique_ptr<WebCacheManagerEfl> web_cache_manager_;
  std::unique_ptr<content::BrowserContextEfl> browser_context_;
  std::unique_ptr<Ewk_Cookie_Manager> ewk_cookie_manager_;
  std::unique_ptr<EwkFaviconDatabase> ewk_favicon_database_;
  std::string proxy_uri_;
  std::string proxy_bypass_rule_;
  std::string injected_bundle_path_;
  bool cache_enable_;

#if defined(OS_TIZEN)
  void* injected_bundle_handle_;
#endif

  // widget info
  std::string tizen_app_id_;
  std::string widget_theme_;
  std::string widget_encoded_bundle_;
  double widget_scale_;

  std::string tizen_app_version_;

  std::unique_ptr<EwkDidStartDownloadCallback> start_download_callback_;
  std::unique_ptr<EwkMimeOverrideCallback> mime_override_callback_;
  int m_pixmap;
#if defined(OS_TIZEN_TV_PRODUCT)
  std::string proxy_username_;
  std::string proxy_password_;

  Ewk_Application_Type application_type_;
  std::unique_ptr<EwkCheckAccessiblePathCallback>
      check_accessible_path_callback_;
  bool m_isShowingMappingInfo;
#endif
  std::unique_ptr<EWebContextNotificationCallback> notification_cb_;
  std::unique_ptr<EwkPushMessageCallback> push_message_callback_;
};

#endif
