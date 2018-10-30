// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_CLIENT_EFL_H
#define CONTENT_RENDERER_CLIENT_EFL_H

#include "content/public/common/url_loader_throttle.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/render_frame_observer.h"
#include "renderer/render_thread_observer_efl.h"
#include "v8/include/v8.h"
#include "components/nacl/common/features.h"

namespace blink {
class WebView;
}

namespace content {
class DocumentState;
class RenderFrame;
class RenderView;
class RenderThread;
}

namespace visitedlink {
class VisitedLinkSlave;
}

namespace network_hints {
class PrescientNetworkingDispatcher;
}

class Ewk_Wrt_Message_Data;

class V8Widget;
class WrtUrlParseImpl;

class ContentRendererClientEfl : public content::ContentRendererClient {
 public:
  ContentRendererClientEfl();
  ~ContentRendererClientEfl();

  void RenderThreadStarted() override;

  void RenderFrameCreated(content::RenderFrame* render_frame) override;

  void RenderViewCreated(content::RenderView* render_view) override;

  bool OverrideCreatePlugin(
      content::RenderFrame* render_frame,
      const blink::WebPluginParams& params,
      blink::WebPlugin** plugin) override;

  /* LCOV_EXCL_START */
  bool AllowPopup() override { return javascript_can_open_windows_; }

  // This setter is an EFL extension, where we cache the preference
  // that controls whether JS content is allowed to open new windows.
  // Note that strictly speaking this cached preference is incorrect:
  // Every 'view' holds a preference instance. So it is possible that
  // different views objects hold different preference set tha another
  // at a given time.
  void SetAllowPopup(bool value) { javascript_can_open_windows_ = value; }
  /* LCOV_EXCL_STOP */

  bool HandleNavigation(content::RenderFrame* render_frame,
                        content::DocumentState* document_state,
                        bool render_view_was_created_by_renderer,
                        blink::WebFrame* frame,
                        const blink::WebURLRequest& request,
                        blink::WebNavigationType type,
                        blink::WebNavigationPolicy default_policy,
                        bool is_redirect) override;

  bool WillSendRequest(
      blink::WebLocalFrame* frame,
      ui::PageTransition transition_type,
      const blink::WebURL& url,
      std::vector<std::unique_ptr<content::URLLoaderThrottle>>* throttles,
      GURL* new_url
#if defined(OS_TIZEN_TV_PRODUCT)
      ,
      bool& is_encrypted_file
#endif
      ) override;
#if defined(OS_TIZEN_TV_PRODUCT)
  bool GetFileDecryptedDataBuffer(const GURL& url,
                                  std::vector<char>* data) override;
#endif

  bool GetWrtParsedUrl(const GURL& url, GURL& parsed_url) override;

#if defined(OS_TIZEN_TV_PRODUCT)
  // Floating Video Window
  void SetFloatVideoWindowState(bool enable) {
    floating_video_window_on_ = enable;
  }
  bool FloatingVideoWindowOn() const override { return floating_video_window_on_; }

  void ScrollbarThumbPartFocusChanged(
      int view_id,
      blink::WebScrollbar::Orientation orientation,
      bool focused) override;
  void RunArrowScroll(int view_id) override;
  void AutoLogin(blink::WebFrame* frame,
                 const std::string& user_name,
                 const std::string& password);
#endif
  void DidCreateScriptContext(
      blink::WebFrame* frame,
      v8::Handle<v8::Context> context,
      int world_id);

  void WillReleaseScriptContext(
      blink::WebFrame* frame,
      v8::Handle<v8::Context>,
      int world_id);

  void GetNavigationErrorStrings(
      content::RenderFrame* render_frame,
      const blink::WebURLRequest& failed_request,
      const blink::WebURLError& error,
      std::string* error_html,
      base::string16* error_description) override;

  unsigned long long VisitedLinkHash(const char* canonical_url,
                                     size_t length) override;

  bool IsLinkVisited(unsigned long long link_hash) override;

  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems)
      override;

#if BUILDFLAG(ENABLE_NACL)
  bool IsExternalPepperPlugin(const std::string& module_name) override;
#endif

  blink::WebPrescientNetworking* GetPrescientNetworking() override;
  std::unique_ptr<blink::WebSpeechSynthesizer> OverrideSpeechSynthesizer(
      blink::WebSpeechSynthesizerClient* client) override;

  bool shutting_down() const { return shutting_down_; }
  /* LCOV_EXCL_START */
  void set_shutting_down(bool shutting_down) { shutting_down_ = shutting_down; }
/* LCOV_EXCL_STOP */

#if defined(TIZEN_PEPPER_EXTENSIONS)
  bool IsPluginAllowedToUseCompositorAPI(const GURL& url) override;
#endif

 private:
  static void ApplyCustomMobileSettings(blink::WebView*);

  std::unique_ptr<V8Widget> widget_;
  std::unique_ptr<RenderThreadObserverEfl> render_thread_observer_;
  std::unique_ptr<visitedlink::VisitedLinkSlave> visited_link_slave_;
  std::unique_ptr<network_hints::PrescientNetworkingDispatcher>
      prescient_networking_dispatcher_;
  bool javascript_can_open_windows_;
  bool shutting_down_;
#if defined(OS_TIZEN_TV_PRODUCT)
  bool floating_video_window_on_;
#endif
};

#endif
