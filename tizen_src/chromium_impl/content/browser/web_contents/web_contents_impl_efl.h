// Copyright (c) 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEB_CONTENTS_IMPL_EFL_H_
#define WEB_CONTENTS_IMPL_EFL_H_

#include "base/memory/ref_counted.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/content_export.h"

namespace content {

class SiteInstance;
class WebContentsEflDelegate;

class CONTENT_EXPORT WebContentsImplEfl : public WebContentsImpl {
 public:
  // See WebContents::Create for a description of these parameters.
  WebContentsImplEfl(BrowserContext* browser_context, void* platform_data);

  void CreateNewWindow(
      RenderFrameHost* opener,
      int32_t render_view_route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      const mojom::CreateNewWindowParams& params,
      SessionStorageNamespace* session_storage_namespace) override;

  void* GetPlatformData() const { return platform_data_; };

  void SetEflDelegate(WebContentsEflDelegate*);

  // Overrides for WebContents
  WebContents* Clone() override;

  void SetUserAgentOverride(const std::string& override) override;

#if defined(OS_TIZEN_TV_PRODUCT)
  void SetIMERecommendedWords(const std::string& recommended_words) override;
  void SetIMERecommendedWordsType(bool should_enable) override;
#endif

  // TODO: We pass ewk_view to WebContentsImplEfl since WebContentsDelegateEfl
  // cannot be included in render_widget_host_view_efl.cc due to dependencies
  // issue. This might be changed after resolving
  // http://suprem.sec.samsung.net/jira/browse/TWF-1453
  void set_ewk_view(void* ewk_view) { ewk_view_ = ewk_view; }
  void* ewk_view() const { return ewk_view_; }

 private:
  // Needed to access private WebContentsImplEfl constructor from
  // WebContents/WebContetnsImpl static Create* functions.
  friend class WebContentsImpl;
  friend class WebContents;

  void CancelWindowRequest(int32_t render_process_id,
                           int32_t main_frame_route_id);

  void HandleNewWindowRequest(
      int32_t render_process_id,
      int32_t render_view_route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      int32_t opener_render_frame_id,
      const mojom::CreateNewWindowParams& params,
      scoped_refptr<SessionStorageNamespace> session_storage_namespace,
      bool create);

  WebContents* HandleNewWebContentsCreate(
      int32_t render_process_id,
      int32_t render_view_route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      int32_t opener_render_frame_id,
      const mojom::CreateNewWindowParams& params,
      scoped_refptr<SessionStorageNamespace> session_storage_namespace,
      void* platform_data);

  void* platform_data_;
  void* ewk_view_;
  std::unique_ptr<WebContentsEflDelegate> efl_delegate_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsImplEfl);
};

} // namespace content

#endif
