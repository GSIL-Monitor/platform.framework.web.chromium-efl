// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_context_private_h
#define ewk_context_private_h

#include <string>
#include <Evas.h>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "public/ewk_context_product.h"

class CookieManager;
class Ewk_Cookie_Manager;
class EWebContext;
class EwkFaviconDatabase;
class GURL;

namespace content {
class BrowserContextEfl;
};

struct Ewk_Context : public base::RefCounted<Ewk_Context> {
 public:
  static Ewk_Context* DefaultContext();
  static Ewk_Context* IncognitoContext();
  static void DefaultContextRelease();
  static Ewk_Context* Create(bool incognito = false);
  static Ewk_Context* Create(bool incognito,
                             const std::string& injectedBundlePath);
  static void Delete(Ewk_Context*);

  // Get related class
  content::BrowserContextEfl* browser_context() const;
  Ewk_Cookie_Manager* ewkCookieManager() const;
  scoped_refptr<CookieManager> cookieManager() const;

  // Set Callbacks
  void SetDidStartDownloadCallback(Ewk_Context_Did_Start_Download_Callback callback,
      void* user_data);
  //EwkDidStartDownloadCallback* DidStartDownloadCallback();

  // Certificate
  void SetCertificatePath(const char* certificate_path);
  const char* GetCertificatePath() const;

  // Proxy
  void SetProxy(const char* uri, const char* bypass_rule);
  const char* GetProxyUri() const;
  const char* GetProxyBypassRule() const;

#if defined(OS_TIZEN_TV_PRODUCT)
  void SetProxyDefaultAuth(const char* username, const char* password);
#endif

  void NotifyLowMemory();

  // HTTP Custom Header
  bool HTTPCustomHeaderAdd(const std::string& name, const std::string& value);
  bool HTTPCustomHeaderRemove(const std::string& name);
  void HTTPCustomHeaderClear();

  // Cache Model
  void SetCacheModel(Ewk_Cache_Model cm);
  Ewk_Cache_Model GetCacheModel() const;

  // Network Cache
  void SetNetworkCacheEnable(bool enable);
  bool GetNetworkCacheEnable() const;
  void ClearNetworkCache();
  void ClearWebkitCache();

  // Application Cache
  void GetAllOriginsWithApplicationCache(
      Ewk_Web_Application_Cache_Origins_Get_Callback callback, void* user_data);
  void GetApplicationCacheUsage(
      const GURL& url,
      Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback callback,
      void* user_data);
  void GetLocalStorageUsageForOrigin(
      const GURL& origin,
      Ewk_Web_Storage_Usage_Get_Callback callback,
      void* user_data);
  void DeleteAllApplicationCache();
  void DeleteApplicationCacheForSite(const GURL& url);

  // Web Database
  void GetAllOriginsWithWebDB(Ewk_Web_Database_Origins_Get_Callback callback,
      void* user_data);
  void WebDBDelete(const GURL& host);
  void GetWebDBUsageForOrigin(const GURL& origin,
                              Ewk_Web_Database_Usage_Get_Callback callback,
                              void* userData);

  // Indexed DB
  void IndexedDBDelete();

  // Web Storage
  void WebStorageOriginsAllGet(Ewk_Web_Storage_Origins_Get_Callback callback,
      void* user_data);
  void WebStorageDelete();
  void WebStorageDelete(const GURL& origin);
  void ClearCandidateData();

  // File System
  void GetAllOriginsWithFileSystem(
      Ewk_Local_File_System_Origins_Get_Callback callback, void* user_data) const;
  void FileSystemDelete(const GURL& host);

  // Favicon
  bool SetFaviconDatabasePath(const base::FilePath& path);
  Evas_Object *AddFaviconObject(const char *uri, Evas *canvas) const;

  // Widget
  void SetWidgetInfo(const std::string& tizen_app_id,
                     double scale,
                     const std::string& theme,
                     const std::string& encoded_bundle);
  bool SetAppVersion(const std::string& tizen_app_version);
  bool SetPWAInfo(const std::string& pwa_storage_path);
  //void SendWrtMessage(const Ewk_IPC_Wrt_Message_Data& message);

  // Pixmap
  int  Pixmap() const;
  void SetPixmap(int pixmap);

  // Password
  void ClearPasswordData();
  void ClearPasswordDataForUrl(const char* url);
  void GetPasswordDataList(
      Ewk_Context_Form_Password_Data_List_Get_Callback callback,
      void* user_data);

  // Extensible
  bool SetExtensibleAPI(const std::string& api_name, bool enable);
  bool GetExtensibleAPI(const std::string& api_name) const;

  // Set MIME override callback
  void SetMimeOverrideCallback(Ewk_Context_Override_Mime_For_Url_Callback callback,
                               void* user_data) const;

  // Note: Do not use outside chromium
  EWebContext* GetImpl() { return impl; }
  EwkFaviconDatabase* GetFaviconDatabase() const;

  unsigned int InspectorServerStart(unsigned int port) const;
  bool InspectorServerStop() const;

  void SetNotificationCallbacks(
    Ewk_Context_Notification_Show_Callback show_callback,
    Ewk_Context_Notification_Cancel_Callback cancel_callback,
    void* user_data);

  // default zoom factor
  void SetDefaultZoomFactor(double zoom_factor);
  double GetDefaultZoomFactor() const;

  // Audio Latency Mode
  void SetAudioLatencyMode(Ewk_Audio_Latency_Mode latency_mode);
  Ewk_Audio_Latency_Mode GetAudioLatencyMode() const;

#if defined(OS_TIZEN_TV_PRODUCT)
  // Set Check Accessible Path callback
  void SetCheckAccessiblePathCallback(
      Ewk_Context_Check_Accessible_Path_Callback callback,
      void* user_data);

  // Application Type
  void SetApplicationType(const Ewk_Application_Type application_type);
  Ewk_Application_Type GetApplicationType() const;
  // Registers url scheme as CORS enabled for HBBTV
  void RegisterURLSchemesAsCORSEnabled(const Eina_List* schemes_list);
  // Registers JS plugins mime types
  void RegisterJSPluginMimeTypes(const Eina_List*);
  // Sets time offset for HBBTV
  void SetTimeOffset(double time_offset);
  // Sets time zone offset for HBBTV
  void SetTimeZoneOffset(double time_zone_offset, double daylight_saving_time);

  void SetEMPCertificatePath(const char* emp_ca_path,
                             const char* default_ca_path);
  void SetMaxURLChars(size_t max_chars);
  void SetContextInterceptRequestCancelCallback(
      Ewk_Context_Intercept_Request_Callback callback,
      void* user_data);
#endif
  void SetContextInterceptRequestCallback(
      Ewk_Context_Intercept_Request_Callback callback,
      void* user_data);

  void EnableAppControl(bool enabled);

  void SetMaxRefreshRate(int max_refresh_rate);

 private:
  EWebContext* impl;

  Ewk_Context(bool incognito);
  Ewk_Context(bool incognito, const std::string& injectedBundlePath);
  ~Ewk_Context();
  friend class base::RefCounted<Ewk_Context>;

  DISALLOW_COPY_AND_ASSIGN(Ewk_Context);
};


#endif // ewk_context_private_h
