// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_context_private.h"

#include "browser_context_efl.h"
#include "eweb_context.h"

using content::BrowserContextEfl;

static Ewk_Context* default_context_ = nullptr;
static Ewk_Context* incognito_context_ = nullptr;

// static
Ewk_Context* Ewk_Context::DefaultContext() {
  if (!default_context_)
    default_context_ = Ewk_Context::Create(false);

  return default_context_;
}

// static
Ewk_Context* Ewk_Context::IncognitoContext() {
  if (!incognito_context_)
    incognito_context_ = Ewk_Context::Create(true);

  return incognito_context_;
}

// static
void Ewk_Context::DefaultContextRelease() {
  if (default_context_) {
    // this method should be called from ewk_shutdown and ewk_shutdown should
    // be called only when all webviews are closed. This won't check if all
    // webviews are destroyed but we will be sure that all webviews using
    // default web context are destroyed.
    if (!default_context_->HasOneRef())
      LOG(ERROR) << "Client didn't destroy all WebView objects"
          << " before calling ewk_shutdown";
    default_context_->Release();
  }
}

/* LCOV_EXCL_START */
// static
Ewk_Context* Ewk_Context::Create(bool incognito) {
  Ewk_Context* context = new Ewk_Context(incognito);
  if (!incognito)
    context->AddRef();
  return context;
}

// static
Ewk_Context* Ewk_Context::Create(bool incognito,
                                 const std::string& injectedBundlePath) {
  Ewk_Context* context = new Ewk_Context(incognito, injectedBundlePath);
  context->AddRef();
  return context;
}

// static
void Ewk_Context::Delete(Ewk_Context* context) {
  if (context) {
    if (context == default_context_ && context->HasOneRef()) {
      // With chromium engine there is only single context
      // which is default context hence this delete
      // function will not be implemented
      NOTIMPLEMENTED();
      return;
    }
    context->Release();
  }
}
/* LCOV_EXCL_STOP */

Ewk_Context::Ewk_Context(bool incognito)
    : impl(new EWebContext(incognito, std::string())) {}

/* LCOV_EXCL_START */
Ewk_Context::Ewk_Context(bool incognito, const std::string& injectedBundlePath)
    : impl(new EWebContext(incognito, injectedBundlePath)) {}
/* LCOV_EXCL_STOP */

Ewk_Context::~Ewk_Context() {
  if (this == default_context_)
    default_context_ = nullptr;
  else if (this == incognito_context_)
    incognito_context_ = nullptr;
  delete impl;
}

content::BrowserContextEfl* Ewk_Context::browser_context() const {
  return impl->browser_context();
}

Ewk_Cookie_Manager* Ewk_Context::ewkCookieManager() const {
  return impl->ewkCookieManager();
}

/* LCOV_EXCL_START */
scoped_refptr<CookieManager> Ewk_Context::cookieManager() const {
  return impl->cookieManager();
}

void Ewk_Context::SetDidStartDownloadCallback(Ewk_Context_Did_Start_Download_Callback callback, void* user_data) {
  impl->SetDidStartDownloadCallback(callback, user_data);
}

void Ewk_Context::SetCertificatePath(const char* certificate_path) {
  impl->SetCertificatePath(certificate_path);
}

const char* Ewk_Context::GetCertificatePath() const {
  return impl->GetCertificatePath().c_str();
}

void Ewk_Context::SetProxy(const char* uri, const char* bypass_rule) {
  impl->SetProxy(uri, bypass_rule);
}

const char* Ewk_Context::GetProxyBypassRule() const {
  return impl->GetProxyBypassRule();
}

const char* Ewk_Context::GetProxyUri() const {
  return impl->GetProxyUri();
}

#if defined(OS_TIZEN_TV_PRODUCT)
void Ewk_Context::SetProxyDefaultAuth(const char* username,
                                      const char* password) {
  impl->SetProxyDefaultAuth(username, password);
}
#endif

void Ewk_Context::NotifyLowMemory() {
  impl->NotifyLowMemory();
}

bool Ewk_Context::HTTPCustomHeaderAdd(const std::string& name, const std::string& value) {
  return impl->HTTPCustomHeaderAdd(name, value);
}

bool Ewk_Context::HTTPCustomHeaderRemove(const std::string& name) {
  return impl->HTTPCustomHeaderRemove(name);
}

void Ewk_Context::HTTPCustomHeaderClear() {
  impl->HTTPCustomHeaderClear();
}
/* LCOV_EXCL_STOP */

void Ewk_Context::SetCacheModel(Ewk_Cache_Model cm) {
  impl->SetCacheModel(cm);
}

Ewk_Cache_Model Ewk_Context::GetCacheModel() const {
  return impl->GetCacheModel();
}

/* LCOV_EXCL_START */
void Ewk_Context::SetNetworkCacheEnable(bool enable) {
  impl->SetNetworkCacheEnable(enable);
}

bool Ewk_Context::GetNetworkCacheEnable() const {
  return impl->GetNetworkCacheEnable();
}
/* LCOV_EXCL_STOP */

void Ewk_Context::ClearNetworkCache() {
  impl->ClearNetworkCache();
}

void Ewk_Context::ClearWebkitCache() {
  impl->ClearWebkitCache();
}

/* LCOV_EXCL_START */
void Ewk_Context::GetAllOriginsWithApplicationCache(Ewk_Web_Application_Cache_Origins_Get_Callback callback, void* user_data) {
  impl->GetAllOriginsWithApplicationCache(callback, user_data);
}

void Ewk_Context::GetApplicationCacheUsage(const GURL& url, Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback callback, void* user_data) {
  impl->GetApplicationCacheUsage(url, callback, user_data);
}
/* LCOV_EXCL_STOP */

void Ewk_Context::GetLocalStorageUsageForOrigin(
    const GURL& origin,
    Ewk_Web_Storage_Usage_Get_Callback callback,
    void* user_data) {
  impl->GetLocalStorageUsageForOrigin(origin, callback, user_data);
}

void Ewk_Context::DeleteAllApplicationCache() {
  impl->DeleteAllApplicationCache();
}

/* LCOV_EXCL_START */
void Ewk_Context::DeleteApplicationCacheForSite(const GURL& url) {
  impl->DeleteApplicationCacheForSite(url);
}

void Ewk_Context::GetAllOriginsWithWebDB(Ewk_Web_Database_Origins_Get_Callback callback, void* user_data) {
  impl->GetAllOriginsWithWebDB(callback, user_data);
}

void Ewk_Context::WebDBDelete(const GURL& host) {
  impl->WebDBDelete(host);
}
/* LCOV_EXCL_STOP */

void Ewk_Context::GetWebDBUsageForOrigin(
    const GURL& origin,
    Ewk_Web_Database_Usage_Get_Callback callback,
    void* userData) {
  impl->GetWebDBUsageForOrigin(origin, callback, userData);
}

void Ewk_Context::IndexedDBDelete() {
  impl->IndexedDBDelete();
}

/* LCOV_EXCL_START */
void Ewk_Context::WebStorageOriginsAllGet(Ewk_Web_Storage_Origins_Get_Callback callback, void* user_data) {
  impl->WebStorageOriginsAllGet(callback, user_data);
}
/* LCOV_EXCL_STOP */

void Ewk_Context::WebStorageDelete() {
  impl->WebStorageDelete();
}

/* LCOV_EXCL_START */
void Ewk_Context::WebStorageDelete(const GURL& origin) {
  impl->WebStorageDeleteForOrigin(origin);
}

void Ewk_Context::GetAllOriginsWithFileSystem(Ewk_Local_File_System_Origins_Get_Callback callback, void* user_data) const {
  impl->GetAllOriginsWithFileSystem(callback, user_data);
}

void Ewk_Context::FileSystemDelete(const GURL& host) {
  impl->FileSystemDelete(host);
}

bool Ewk_Context::SetFaviconDatabasePath(const base::FilePath& path) {
  return impl->SetFaviconDatabasePath(path);
}

EwkFaviconDatabase* Ewk_Context::GetFaviconDatabase() const {
  return impl->GetFaviconDatabase();
}
/* LCOV_EXCL_STOP */

Evas_Object * Ewk_Context::AddFaviconObject(const char *uri, Evas *canvas) const {
  return impl->AddFaviconObject(uri, canvas);
}

/* LCOV_EXCL_START */
void Ewk_Context::SetWidgetInfo(const std::string& tizen_app_id,
                                double scale,
                                const std::string& theme,
                                const std::string& encoded_bundle) {
  impl->SetWidgetInfo(tizen_app_id, scale, theme, encoded_bundle);
}

bool Ewk_Context::SetAppVersion(const std::string& tizen_app_version) {
  return impl->SetAppVersion(tizen_app_version);
}

int Ewk_Context::Pixmap() const {
  return impl->Pixmap();
}

void Ewk_Context::SetPixmap(int pixmap) {
  impl->SetPixmap(pixmap);
}

void Ewk_Context::SetMimeOverrideCallback(Ewk_Context_Override_Mime_For_Url_Callback callback,
                                         void* user_data) const {
  impl->SetMimeOverrideCallback(callback, user_data);
}
/* LCOV_EXCL_STOP */

void Ewk_Context::ClearCandidateData() {
  impl->ClearCandidateData();
}

bool Ewk_Context::SetExtensibleAPI(const std::string& api_name, bool enable) {
  return impl->SetExtensibleAPI(api_name, enable);
}

bool Ewk_Context::GetExtensibleAPI(const std::string& api_name) const {
  return impl->GetExtensibleAPI(api_name);
}

void Ewk_Context::ClearPasswordData() {
  impl->ClearPasswordData();
  /* LCOV_EXCL_START */
}

void Ewk_Context::ClearPasswordDataForUrl(const char* url) {
  impl->ClearPasswordDataForUrl(url);
}

void Ewk_Context::GetPasswordDataList(
    Ewk_Context_Form_Password_Data_List_Get_Callback callback,
    void* user_data) {
  impl->GetPasswordDataList(callback, user_data);
}

unsigned int Ewk_Context::InspectorServerStart(unsigned int port) const {
  return impl->InspectorServerStart(port);
}

bool Ewk_Context::InspectorServerStop() const {
  return impl->InspectorServerStop();
}

void Ewk_Context::SetNotificationCallbacks(
    Ewk_Context_Notification_Show_Callback show_callback,
    Ewk_Context_Notification_Cancel_Callback cancel_callback,
    void* user_data) {
  impl->SetNotificationCallbacks(
      this, show_callback, cancel_callback, user_data);
}

/* LCOV_EXCL_STOP */
/* LCOV_EXCL_START */
void Ewk_Context::SetDefaultZoomFactor(double zoom_factor) {
  impl->SetDefaultZoomFactor(zoom_factor);
}

double Ewk_Context::GetDefaultZoomFactor() const {
  return impl->GetDefaultZoomFactor();
}

void Ewk_Context::SetAudioLatencyMode(Ewk_Audio_Latency_Mode latency_mode) {
  impl->SetAudioLatencyMode(latency_mode);
}

Ewk_Audio_Latency_Mode Ewk_Context::GetAudioLatencyMode() const {
  return impl->GetAudioLatencyMode();
}

#if defined(OS_TIZEN_TV_PRODUCT)
void Ewk_Context::SetCheckAccessiblePathCallback(
    Ewk_Context_Check_Accessible_Path_Callback callback,
    void* user_data) {
  impl->SetCheckAccessiblePathCallback(callback, user_data);
}

void Ewk_Context::SetApplicationType(
    const Ewk_Application_Type application_type) {
  impl->SetApplicationType(application_type);
}

Ewk_Application_Type Ewk_Context::GetApplicationType() const {
  return impl->GetApplicationType();
}

void Ewk_Context::RegisterURLSchemesAsCORSEnabled(
    const Eina_List* schemes_list) {
  impl->RegisterURLSchemesAsCORSEnabled(schemes_list);
}

void Ewk_Context::RegisterJSPluginMimeTypes(const Eina_List* mime_types) {
  impl->RegisterJSPluginMimeTypes(mime_types);
}

void Ewk_Context::SetTimeOffset(double time_offset) {
  impl->SetTimeOffset(time_offset);
}

void Ewk_Context::SetTimeZoneOffset(double time_zone_offset,
                                    double daylight_saving_time) {
  impl->SetTimeZoneOffset(time_zone_offset, daylight_saving_time);
}

void Ewk_Context::SetEMPCertificatePath(const char* emp_ca_path,
                                        const char* default_ca_path) {
  impl->SetEMPCertificatePath(emp_ca_path, default_ca_path);
}

void Ewk_Context::SetMaxURLChars(size_t max_chars) {
  impl->SetMaxURLChars(max_chars);
}
#endif
/* LCOV_EXCL_STOP */

void Ewk_Context::SetContextInterceptRequestCallback(
    Ewk_Context_Intercept_Request_Callback callback,
    void* user_data) {
  impl->SetInterceptRequestCallback(this, callback, user_data);
}

#if defined(OS_TIZEN_TV_PRODUCT)
void Ewk_Context::SetContextInterceptRequestCancelCallback(
    Ewk_Context_Intercept_Request_Callback callback,
    void* user_data) {
  impl->SetInterceptRequestCancelCallback(this, callback, user_data);
}

bool Ewk_Context::SetPWAInfo(const std::string& pwa_storage_path) {
  return impl->SetPWAInfo(pwa_storage_path);
}
#endif

void Ewk_Context::EnableAppControl(bool enabled) {
  impl->EnableAppControl(enabled);
}

void Ewk_Context::SetMaxRefreshRate(int max_refresh_rate) {
  impl->SetMaxRefreshRate(max_refresh_rate);
}

