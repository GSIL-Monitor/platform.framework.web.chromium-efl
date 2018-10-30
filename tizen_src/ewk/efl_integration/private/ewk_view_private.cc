// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_view_private.h"

#include <cassert>

#include "eweb_view.h"
#include "private/ewk_context_private.h"
#include "private/webview_delegate_ewk.h"


// -------- EwkViewImpl
struct EwkViewImpl  {
  explicit EwkViewImpl(EWebView* wv) : wv_(wv) {}
  ~EwkViewImpl() { delete(wv_); }
  EWebView* GetTizenWebView() { return wv_; }
  const EWebView* GetTizenWebView() const { return wv_; }
 private:
  EWebView* wv_;
  DISALLOW_COPY_AND_ASSIGN(EwkViewImpl);
};

Evas_Smart_Class parent_smart_class_ = EVAS_SMART_CLASS_INIT_NULL;

namespace {

Evas_Smart* DefaultSmartClassInstance()
{
  static Ewk_View_Smart_Class api = EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION(EwkViewSmartClassName);
  static Evas_Smart* smart = 0;
  if (!smart) {
    InitSmartClassInterface(api);
    smart = evas_smart_class_new(&api.sc);
  }

  return smart;
}

void SmartDataChanged(Ewk_View_Smart_Data* d)
{
  assert(d);
  if (d->changed.any)
    return;
  d->changed.any = true;
  evas_object_smart_changed(d->self);
}

// Evas_Smart_Class callback interface:
void handleEvasObjectAdd(Evas_Object* evas_object)
{
  const Evas_Smart* smart = evas_object_smart_smart_get(evas_object);
  const Evas_Smart_Class* smart_class = evas_smart_class_get(smart);
  const Ewk_View_Smart_Class* api = reinterpret_cast<const Ewk_View_Smart_Class*>(smart_class);
  assert(api);

  Ewk_View_Smart_Data* d = GetEwkViewSmartDataFromEvasObject(evas_object);

  if (!d) {
    // Allocating with 'calloc' as the API contract is that it should be deleted with 'free()'.
    d = static_cast<Ewk_View_Smart_Data*>(calloc(1, sizeof(Ewk_View_Smart_Data)));
    if (!d) {
      EINA_LOG_ERR("Failed to create Smart Data");  // LCOV_EXCL_LINE
      return;                                       // LCOV_EXCL_LINE
    }
    evas_object_smart_data_set(evas_object, d);
  }
  d->self = evas_object;
  d->api = api;

  parent_smart_class_.add(evas_object);

  d->priv = 0; // Will be initialized later.
}

// Ewk_View_Smart_Class callback interface:
void handleEvasObjectDelete(Evas_Object* evas_object)
{
  Ewk_View_Smart_Data* smart_data = GetEwkViewSmartDataFromEvasObject(evas_object);
  if (smart_data) {
    delete smart_data->priv;
    smart_data->priv = NULL;
  }
  parent_smart_class_.del(evas_object);
}

/* LCOV_EXCL_START */
void handleEvasObjectShow(Evas_Object* o)
{
  Ewk_View_Smart_Data* d = GetEwkViewSmartDataFromEvasObject(o);
  EWebView* wv = GetWebViewFromSmartData(d);
  if (!wv) {
    return;
  }
  // WebKit bails here if widget accelerated compositing is used.
  // TODO: consider this when we will have AC support.
  if (evas_object_clipees_get(d->base.clipper))
    evas_object_show(d->base.clipper);
  wv->HandleShow();
}

void handleEvasObjectHide(Evas_Object* o)
{
  Ewk_View_Smart_Data* d = GetEwkViewSmartDataFromEvasObject(o);
  EWebView* wv = GetWebViewFromSmartData(d);
  if (!wv) {
    return;
  }
  evas_object_hide(d->base.clipper);
  // Deleting view by app results in calling hide method.
  // We assert that, RWHV is null only when renderer has crashed.
  /*if (!ToEWebView(d)->rwhv()) {
     DLOG_ASSERT is used because it is controlled by NDEBUG
    DLOG_ASSERT(ToEWebView(d)->renderer_crashed_);
    return;
  }*/
  wv->HandleHide();
}
/* LCOV_EXCL_STOP */

void handleEvasObjectMove(Evas_Object* o, Evas_Coord x, Evas_Coord y)
{
  Ewk_View_Smart_Data* d = GetEwkViewSmartDataFromEvasObject(o);
  EWebView* wv = GetWebViewFromSmartData(d);
  if (!wv) {
    return;
  }
  wv->HandleMove(x, y);
  SmartDataChanged(d);
}

void handleEvasObjectResize(Evas_Object* o, Evas_Coord width, Evas_Coord height)
{
  Ewk_View_Smart_Data* d = GetEwkViewSmartDataFromEvasObject(o);
  EWebView* wv = GetWebViewFromSmartData(d);
  if (!wv) {
    return;
  }

  // Set the hardcode as 1920 * 1080 for invaild size.
  if (width == 82 && height == 24) {
    LOG(ERROR) << "Set the hardcode as 1920 * 1080 for invaild size";
    width = 1920;
    height = 1080;
    evas_object_geometry_set(o, 0, 0, width, height);
  }

  d->view.w = width;
  d->view.h = height;
  wv->HandleResize(width, height);
  SmartDataChanged(d);
}

/* LCOV_EXCL_START */
void handleEvasObjectCalculate(Evas_Object* o)
{
  Ewk_View_Smart_Data* d = GetEwkViewSmartDataFromEvasObject(o);
  EWebView* wv = GetWebViewFromSmartData(d);
  if (!wv) {
    return;
  }
  Evas_Coord x, y, width, height;
  evas_object_geometry_get(o, &x, &y, &width, &height);
  d->view.x = x;
  d->view.y = y;
  d->view.w = width;
  d->view.h = height;
}

void handleEvasObjectColorSet(Evas_Object*, int red, int green, int blue, int alpha)
{
  // FIXME: implement.
}

Eina_Bool handleTextSelectionDown(Ewk_View_Smart_Data* d, int x, int y)
{
  EWebView* wv = GetWebViewFromSmartData(d);
  if (!wv)
    return false;
  return wv->HandleTextSelectionDown(x, y);
}

Eina_Bool handleTextSelectionUp(Ewk_View_Smart_Data* d, int x, int y)
{
  EWebView* wv = GetWebViewFromSmartData(d);
  if (!wv)
    return false;
  return wv->HandleTextSelectionUp(x, y);
}

unsigned long long handleExceededDatabaseQuota(Ewk_View_Smart_Data *sd, const char *databaseName, const char *displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage)
{
  // Chromium does not support quota per origin right now, this API can't be implemented
  NOTIMPLEMENTED();
  return EINA_FALSE;
}
/* LCOV_EXCL_STOP */

} // namespace


// --------- public API --------------

bool IsWebViewObject(const Evas_Object* evas_object)
{
  if (!evas_object) {
    return false;
  }

  const char* object_type = evas_object_type_get(evas_object);
  const Evas_Smart* evas_smart = evas_object_smart_smart_get(evas_object);
  if (!evas_smart) {
    EINA_LOG_ERR("%p (%s) is not a smart object!", evas_object,
                 object_type ? object_type : "(null)");  // LCOV_EXCL_LINE
    return false;                                        // LCOV_EXCL_LINE
  }

  const Evas_Smart_Class* smart_class = evas_smart_class_get(evas_smart);
  if (!smart_class) {
    EINA_LOG_ERR("%p (%s) is not a smart class object!", evas_object,
                 object_type ? object_type : "(null)");  // LCOV_EXCL_LINE
    return false;                                        // LCOV_EXCL_LINE
  }

  if (smart_class->data != EwkViewSmartClassName) {
    EINA_LOG_ERR("%p (%s) is not of an ewk_view (need %p, got %p)!",
                 evas_object, object_type ? object_type : "(null)",
                 EwkViewSmartClassName, smart_class->data);  // LCOV_EXCL_LINE
    return false;                                            // LCOV_EXCL_LINE
  }
  return true;
}

Ewk_View_Smart_Data* GetEwkViewSmartDataFromEvasObject(const Evas_Object* evas_object)
{
  assert(evas_object);
  assert(IsWebViewObject(evas_object));
  return static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(evas_object));
}

EWebView* GetWebViewFromSmartData(const Ewk_View_Smart_Data* smartData)
{
  if (smartData && smartData->priv) {
    return smartData->priv->GetTizenWebView();
  }
  return NULL;
}

EWebView* GetWebViewFromEvasObject(const Evas_Object* eo) {
  if (!IsWebViewObject(eo)) {
    return NULL;
  }
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(eo);
  return GetWebViewFromSmartData(sd);
}

Evas_Object* CreateWebViewAsEvasObject(Ewk_Context* context,
                                       Evas* canvas,
                                       Evas_Smart* smart /*= 0*/) {
  smart = smart ? smart : DefaultSmartClassInstance();
  Evas_Object* wv_evas_object = evas_object_smart_add(canvas, smart);
  EWebView* view = new EWebView(context, wv_evas_object);
  if (!view) {
    return NULL;
  }
  Ewk_View_Smart_Data* sd = GetEwkViewSmartDataFromEvasObject(wv_evas_object);
  if (!sd) {
    delete view;
    return NULL;
  }
  // attach webview as a member of smart data
  sd->priv = new EwkViewImpl(view);
  view->Initialize();
  return wv_evas_object;
}


bool InitSmartClassInterface(Ewk_View_Smart_Class& api)
{
  if (api.version != EWK_VIEW_SMART_CLASS_VERSION) {
    EINA_LOG_ERR("Ewk_View_Smart_Class %p is version %lu while %lu"
                 " was expected.",
                 &api, api.version, EWK_VIEW_SMART_CLASS_VERSION);
    return false;
  }

  if (!parent_smart_class_.add)
    evas_object_smart_clipped_smart_set(&parent_smart_class_);

  evas_object_smart_clipped_smart_set(&api.sc);

  // Set Evas_Smart_Class callbacks.
  api.sc.add = &handleEvasObjectAdd;
  api.sc.del = &handleEvasObjectDelete;
  api.sc.move = &handleEvasObjectMove;
  api.sc.resize = &handleEvasObjectResize;
  api.sc.show = &handleEvasObjectShow;
  api.sc.hide = &handleEvasObjectHide;
  api.sc.calculate = &handleEvasObjectCalculate;
  api.sc.color_set = &handleEvasObjectColorSet;

  // Set Ewk_View_Smart_Class callbacks.
  api.text_selection_down = &handleTextSelectionDown;
  api.text_selection_up = &handleTextSelectionUp;

  api.exceeded_database_quota = &handleExceededDatabaseQuota;

  // Type identifier.
  api.sc.data = EwkViewSmartClassName;

  return true;
}
