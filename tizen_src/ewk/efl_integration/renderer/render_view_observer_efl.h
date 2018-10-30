// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RENDER_VIEW_OBSERVER_EFL_H_
#define RENDER_VIEW_OBSERVER_EFL_H_

#include <string>

#include "base/timer/timer.h"
#include "common/web_preferences_efl.h"
#include "content/public/renderer/render_view_observer.h"
#include "private/ewk_hit_test_private.h"
#include "public/ewk_view_internal.h"
#include "renderer/content_renderer_client_efl.h"
#include "renderer/print_web_view_helper_efl.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
#include "third_party/WebKit/public/web/WebViewModeEnums.h"
namespace base {
class FilePath;
}

namespace content {
class ContentRendererClient;
class RenderView;
}

namespace WebKit {
class WebHitTestResult;
}

namespace blink {
class WebGestureEvent;
}

class EwkViewMsg_LoadData_Params;
class Hit_Test_Params;

class RenderViewObserverEfl: public content::RenderViewObserver {
 public:
  explicit RenderViewObserverEfl(
      content::RenderView* render_view,
      ContentRendererClientEfl* render_client);
  virtual ~RenderViewObserverEfl();

  bool OnMessageReceived(const IPC::Message& message) override;
  void OnDestruct() override;
  void DidCreateDocumentElement(blink::WebLocalFrame* frame) override;

  //Changes in PageScaleFactorLimits are applied when layoutUpdated is called
  //So using this notification to update minimum and maximum page scale factor values
  virtual void DidUpdateLayout() override;

  void DidHandleGestureEvent(const blink::WebGestureEvent& event) override;

 private:
  void OnSetContentSecurityPolicy(const std::string& policy, Ewk_CSP_Header_Type header_type);
  void OnSetScroll(int x, int y);
#if defined(OS_TIZEN_TV_PRODUCT)
  void OnSetTranslatedURL(const std::string url, bool* result);
  void OnSetPreferTextLang(const std::string& lang);
  void OnSetParentalRatingResult(const std::string& url, bool is_pass);
  void OnEdgeScrollBy(gfx::Point offset, gfx::Point mouse_position);
  void OnRequestHitTestForMouseLongPress(int render_id, int view_x, int view_y);
  void OnSetFloatVideoWindowState(bool enable);
#endif
#if defined(TIZEN_ATK_FEATURE_VD)
  void MoveFocusToBrowser(int direction) override;
#endif
  void OnUseSettingsFont();
  void OnPlainTextGet(int plain_text_get_callback_id);
  void OnDoHitTest(int x,
                   int y,
                   Ewk_Hit_Test_Mode mode);
  void OnDoHitTestAsync(int view_x, int view_y, Ewk_Hit_Test_Mode mode, int64_t request_id);
  bool DoHitTest(int view_x, int view_y, Ewk_Hit_Test_Mode mode, Hit_Test_Params* params);
  void OnPrintToPdf(int width, int height, const base::FilePath& filename);
  void OnGetMHTMLData(int callback_id);
  void OnGetBackgroundColor(int callback_id);
  void OnSetBackgroundColor(int red, int green, int blue, int alpha);
  void OnWebAppIconUrlGet(int callback_id);
  void OnWebAppIconUrlsGet(int callback_id);
  void OnWebAppCapableGet(int callback_id);
  void OnSetBrowserFont();
  void CheckContentsSize();
  void OnSuspendScheduledTasks();
  void OnResumeScheduledTasks();
#if defined(OS_TIZEN_TV_PRODUCT)
  void OnSuspendNetworkLoading();
  void OnResumeNetworkLoading();
  void OnAutoLogin(const std::string& user_name, const std::string& password);
#endif
  void OnIsVideoPlaying(int callback_id);
  void OnSetMainFrameScrollbarVisible(bool);
  void OnGetMainFrameScrollbarVisible(int);
  void OnUpdateWebKitPreferencesEfl(const WebPreferencesEfl&);
  void OnUpdateFocusedNodeBounds();
  void OnQueryInputType();

  void HandleTap(const blink::WebGestureEvent& event);

  // This function sets CSS "view-mode" media feature value.
  void OnSetViewMode(blink::WebViewMode view_mode);
#if defined(TIZEN_VIDEO_HOLE)
  void OnSetVideoHole(bool enable);
#endif
  blink::WebElement GetFocusedElement();
  void OnSetTextZoomFactor(float zoom_factor);
  void OnScrollFocusedNodeIntoView();
  void OnSetLongPollingGlobalTimeout(unsigned long timeout);

  void OnQueryNumberFieldAttributes();

  gfx::Size last_sent_contents_size_;
  base::OneShotTimer check_contents_size_timer_;

  content::ContentRendererClient* renderer_client_;
};

#endif /* RENDER_VIEW_OBSERVER_EFL_H_ */
