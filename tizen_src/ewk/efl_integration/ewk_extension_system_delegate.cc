// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk/efl_integration/ewk_extension_system_delegate.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chromium_impl/build/tizen_version.h"
#include "ppapi/cpp/samsung/extension_system_samsung_wrt.h"
#include "private/ewk_value_private.h"

#if defined(OS_TIZEN_TV_PRODUCT) && defined(USE_WAYLAND)
#include <Ecore_Evas.h>
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
#include <Ecore_Wl2.h>
#else
#include <Ecore_Wayland.h>
#endif
#include <tizen-extension-client-protocol.h>
#endif  // defined(OS_TIZEN_TV_PRODUCT) && defined(USE_WAYLAND)

namespace {
static const char kTypeWebBrowserName[] = "WebBrowser";
static const char kTypeTizenWrtName[] = "TizenWRT";
static const char kTypeHbbTvName[] = "HbbTV";
static const char kTypeUnrecognized[] = "UnrecognizedEmbedder";

static base::LazyInstance<std::string>::Leaky current_embedder_name =
    LAZY_INSTANCE_INITIALIZER;

#if defined(OS_TIZEN_TV_PRODUCT) && defined(USE_WAYLAND)

struct WLRegistryDestroy {
  void operator()(void* x) const {
    wl_registry* registry = static_cast<wl_registry*>(x);
    wl_registry_destroy(registry);
  }
};

struct TizenSurfaceDestroy {
  void operator()(void* x) const {
    tizen_surface* surface = static_cast<tizen_surface*>(x);
    tizen_surface_destroy(surface);
  }
};

struct TizenResourceDestroy {
  void operator()(void* x) const {
    tizen_resource* resource = static_cast<tizen_resource*>(x);
    tizen_resource_destroy(resource);
  }
};

struct WLEventQueueDestroy {
  void operator()(void* x) const {
    wl_event_queue* queue = static_cast<wl_event_queue*>(x);
    wl_event_queue_destroy(queue);
  }
};

static void HandleGlobal(void* data,
                         wl_registry* registry,
                         uint32_t name,
                         const char* interface,
                         uint32_t version) {
  if (data && strcmp(interface, "tizen_surface") == 0) {
    void** surface = reinterpret_cast<void**>(data);
    *surface =
        wl_registry_bind(registry, name, &tizen_surface_interface, version);
  }
}

static void RemoveGlobal(void* data,
                         struct wl_registry* wl_registry,
                         uint32_t name) {}

const struct wl_registry_listener registry_listener = {HandleGlobal,
                                                       RemoveGlobal};

void HandleResourceId(void* data, tizen_resource* tizen_resource, uint32_t id) {
  if (data)
    *reinterpret_cast<int*>(data) = id;
}

const struct tizen_resource_listener tz_resource_listener = {
    HandleResourceId,
};

int GetSurfaceResourceId(wl_surface* surface, wl_display* display) {
  using ScopedWLRegistry = std::unique_ptr<wl_registry, WLRegistryDestroy>;
  using ScopedWLEventQueue =
      std::unique_ptr<wl_event_queue, WLEventQueueDestroy>;
  using ScopedTizenSurface =
      std::unique_ptr<tizen_surface, TizenSurfaceDestroy>;
  using ScopedTizenResource =
      std::unique_ptr<tizen_resource, TizenResourceDestroy>;

  DCHECK(surface && display);
  if (surface == nullptr || display == nullptr)
    return 0;

  ScopedWLEventQueue event_queue(wl_display_create_queue(display));
  DCHECK(event_queue.get());
  if (event_queue == nullptr)
    return 0;

  ScopedWLRegistry registry(wl_display_get_registry(display));
  DCHECK(registry.get());
  if (registry == nullptr)
    return 0;
  wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(registry.get()),
                     event_queue.get());

  tizen_surface* tz_surface_raw = nullptr;
  wl_registry_add_listener(registry.get(), &registry_listener, &tz_surface_raw);
  wl_display_dispatch_queue(display, event_queue.get());
  wl_display_roundtrip_queue(display, event_queue.get());
  DCHECK(tz_surface_raw);
  if (tz_surface_raw == nullptr)
    return 0;
  ScopedTizenSurface tz_surface(tz_surface_raw);

  ScopedTizenResource tz_resource(
      tizen_surface_get_tizen_resource(tz_surface.get(), surface));
  DCHECK(tz_resource.get());
  if (tz_resource.get() == nullptr)
    return 0;
  wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(tz_resource.get()),
                     event_queue.get());

  int wl_surface_id = 0;
  tizen_resource_add_listener(tz_resource.get(), &tz_resource_listener,
                              &wl_surface_id);
  wl_display_roundtrip_queue(display, event_queue.get());
  return wl_surface_id;
}

#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
int GetWaylandWindowId(Ecore_Wl2_Window* ew) {
  wl_surface* surface = ecore_wl2_window_surface_get(ew);
  Ecore_Wl2_Display *wl2_display = ecore_wl2_connected_display_get(NULL);
  wl_display* display = ecore_wl2_display_get(wl2_display);
  return GetSurfaceResourceId(surface, display);
}
#else
int GetWaylandWindowId(Ecore_Wl_Window* ew) {
  wl_surface* surface = ecore_wl_window_surface_get(ew);
  wl_display* display = static_cast<wl_display*>(ecore_wl_display_get());
  return GetSurfaceResourceId(surface, display);
}
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)

#endif  // defined(OS_TIZEN_TV_PRODUCT) && defined(USE_WAYLAND)

}  // namespace

EwkExtensionSystemDelegate::EwkExtensionSystemDelegate()
    : info_(nullptr), cb_(nullptr), cb_data_(nullptr), window_id_(0) {}

EwkExtensionSystemDelegate::~EwkExtensionSystemDelegate() {
  ewk_value_unref(info_);
}

std::string EwkExtensionSystemDelegate::GetEmbedderName() const {
  return current_embedder_name.Get();
}

std::unique_ptr<base::Value> EwkExtensionSystemDelegate::GetExtensionInfo()
    const {
  base::AutoLock guard{access_lock_};
  if (!info_)
    return base::MakeUnique<base::Value>();
  return static_cast<const EwkValuePrivate*>(info_)
      ->GetValue()
      ->CreateDeepCopy();
}

int EwkExtensionSystemDelegate::GetWindowId() const {
  return window_id_;
}

std::unique_ptr<base::Value> EwkExtensionSystemDelegate::GenericSyncCall(
    const std::string& name,
    const base::Value& data) {
  base::AutoLock guard{access_lock_};
  if (!cb_)
    return base::MakeUnique<base::Value>();

  Ewk_Value arg = static_cast<Ewk_Value>(
      new EwkValuePrivate(std::unique_ptr<base::Value>(data.DeepCopy())));
  ewk_value_ref(arg);
  Ewk_Value result = cb_(name.c_str(), arg, cb_data_);
  ewk_value_unref(arg);
  if (!result)
    return base::MakeUnique<base::Value>();
  std::unique_ptr<base::Value> ret =
      static_cast<const EwkValuePrivate*>(result)->GetValue()->CreateDeepCopy();
  ewk_value_unref(result);
  return ret;
}

void EwkExtensionSystemDelegate::SetEmbedderName(
    const Ewk_Application_Type embedder_type) {
  switch (embedder_type) {
    case EWK_APPLICATION_TYPE_WEBBROWSER:
      current_embedder_name.Get() = kTypeWebBrowserName;
      break;
    case EWK_APPLICATION_TYPE_HBBTV:
      current_embedder_name.Get() = kTypeHbbTvName;
      break;
    case EWK_APPLICATION_TYPE_TIZENWRT:
      current_embedder_name.Get() = kTypeTizenWrtName;
      break;
    default:
      current_embedder_name.Get() = kTypeUnrecognized;
      break;
  }
}

void EwkExtensionSystemDelegate::SetExtensionInfo(Ewk_Value info) {
  base::AutoLock guard{access_lock_};

  ewk_value_ref(info);
  // Unref current info_, if already set
  ewk_value_unref(info_);
  info_ = info;
}

void EwkExtensionSystemDelegate::SetGenericSyncCallback(
    Generic_Sync_Call_Callback cb,
    void* data) {
  base::AutoLock guard{access_lock_};
  cb_ = cb;
  cb_data_ = data;
}

void EwkExtensionSystemDelegate::SetWindowId(const Evas_Object* main_window) {
#if defined(OS_TIZEN_TV_PRODUCT)
  Evas* evas = evas_object_evas_get(main_window);
  if (!evas) {
    LOG(ERROR) << "Evas can`t be acquired for main window";
    return;
  }
  const Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas);
  if (!ee) {
    LOG(ERROR) << "Ecore_Evas can`t be acquired from main window Evas";
    return;
  }
#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
  Ecore_Wl2_Window* ww = ecore_evas_wayland2_window_get(ee);
#else
  Ecore_Wl_Window* ww = ecore_evas_wayland_window_get(ee);
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
  if (!ww) {
    LOG(ERROR) << "Can`t get Wl window";
    return;
  }
  window_id_ = GetWaylandWindowId(ww);
#else
  Ecore_X_Window ww = ecore_evas_gl_x11_window_get(ee);
  window_id_ = static_cast<int>(ww);
#endif  // defined(USE_WAYLAND)
#endif  // defined(OS_TIZEN_TV_PRODUCT)
}
