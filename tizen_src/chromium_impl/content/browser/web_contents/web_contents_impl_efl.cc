// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2014-2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_contents/web_contents_impl_efl.h"

#include "base/metrics/user_metrics.h"
#include "content/browser/browser_plugin/browser_plugin_guest.h"
#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "content/browser/dom_storage/session_storage_namespace_impl.h"
#include "content/browser/frame_host/navigation_entry_impl.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/web_contents/web_contents_view.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_plugin_guest_manager.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_efl_delegate.h"
#include "content/public/common/content_client.h"
#include "content/public/common/result_codes.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#endif

// Majority of the code in this file was taken directly from
// WebContentsImpl::CreateNewWindow. Compared to the original, the function
// was split into 3 major parts which can be executed independently. The
// parts are:
// * WebContentsImplEfl::CreateNewWindow - Handle parts until the actual
//   decision regarding if the window should be created or not.
// * WebContentsImplEfl::HandleNewWindowRequest - Handle positive/negative
//   decision regarding the window. Either continue processing the request or
//   cancel it.
// * WebContentsImplEfl::HandleNewWebContentsCreate - Create new WebContents
//
// The execution of parts 2 and 3 is controlled by the user of the API through
// WebContentsDelegate ShouldCreateWebContentsAsync and WebContentsCreatedAsync
// functions.
//
// In case WebContentsDelegate returns false from ShouldCreateWebContentsAsync
// and WebContentsCreateAsync functions the call sequence is:
//
// + WebContentsImplEfl::CreateMewWimdow
// |-+ WebContentsImplEfl::HandleNewWindowRequesst
//   |-+ WebContentsImplEfl::HandleNewWebContentsCreate
//
// All functions are called in a single sequence.
//
// In the oposite case, both ShouldCreateWebContentsAsync and
// WebContentsCreateAsyns return true:
//
// + WebContents::Impl::CreateNewWindow
// ...
// + WebContentsDelegate::WebContentsCreateCallback
// |-+ WebContentsImplEfl::HandleNewWindowRequest
// ...
// + WebContentsDelegate::WebContentsCreateAsync
// |-+ WebContentsImplEfl::HandleNewWebContentsCreate
//
// In this case the user of the API decides when to execute each stage of the
// sequence.

namespace content {

bool FindMatchingProcess(int render_process_id,
                         bool* did_match_process,
                         FrameTreeNode* node) {
  if (node->current_frame_host()->GetProcess()->GetID() == render_process_id) {
    *did_match_process = true;
    return false;
  }
  return true;
}

WebContentsImplEfl::WebContentsImplEfl(BrowserContext* browser_context,
                                       void* platform_data)
    : WebContentsImpl(browser_context),
      platform_data_(platform_data),
      ewk_view_(nullptr) {}

void WebContentsImplEfl::SetEflDelegate(WebContentsEflDelegate* delegate) {
  efl_delegate_.reset(delegate);
}

WebContents* WebContentsImplEfl::Clone() {
  NOTREACHED() << "Cloning WebContents is not supported in EFL port";
  return NULL;
}

void WebContentsImplEfl::SetUserAgentOverride(const std::string& override) {
  if (GetUserAgentOverride() == override)
    return;

  renderer_preferences_.user_agent_override = override;

  // Send the new override string to the renderer.
  RenderViewHost* host = GetRenderViewHost();
  if (host)
    host->SyncRendererPrefs();

  // In chromium upstream, page is reloaded if a load is currently in progress.
  // In chromium-efl port, the behaviour is different.

  for (auto& observer : observers_)
    observer.UserAgentOverrideSet(override);
}

void WebContentsImplEfl::CreateNewWindow(
    RenderFrameHost* opener,
    int32_t render_view_route_id,
    int32_t main_frame_route_id,
    int32_t main_frame_widget_route_id,
    const mojom::CreateNewWindowParams& params,
    SessionStorageNamespace* session_storage_namespace) {
  // We should have zero valid routing ids, or three valid routing IDs.
  DCHECK_EQ((render_view_route_id == MSG_ROUTING_NONE),
            (main_frame_route_id == MSG_ROUTING_NONE));
  DCHECK_EQ((render_view_route_id == MSG_ROUTING_NONE),
            (main_frame_widget_route_id == MSG_ROUTING_NONE));
  DCHECK(opener);

  int render_process_id = opener->GetProcess()->GetID();
  SiteInstance* source_site_instance = opener->GetSiteInstance();

  // The route IDs passed into this function can be trusted not to already
  // be in use; they were allocated by the RenderWidgetHelper by the caller.
  DCHECK(!RenderFrameHostImpl::FromID(render_process_id, main_frame_route_id));

  // We usually create the new window in the same BrowsingInstance (group of
  // script-related windows), by passing in the current SiteInstance.  However,
  // if the opener is being suppressed (in a non-guest), we create a new
  // SiteInstance in its own BrowsingInstance.
  bool is_guest = BrowserPluginGuest::IsGuest(this);

  // If the opener is to be suppressed, the new window can be in any process.
  // Since routing ids are process specific, we must not have one passed in
  // as argument here.
  DCHECK(!params.opener_suppressed || render_view_route_id == MSG_ROUTING_NONE);

  scoped_refptr<SiteInstance> site_instance =
      params.opener_suppressed && !is_guest
          ? SiteInstance::CreateForURL(GetBrowserContext(), params.target_url)
          : source_site_instance;

  // We must assign the SessionStorageNamespace before calling Init().
  //
  // http://crbug.com/142685
  const std::string& partition_id =
      GetContentClient()->browser()->GetStoragePartitionIdForSite(
          GetBrowserContext(), site_instance->GetSiteURL());
  StoragePartition* partition = BrowserContext::GetStoragePartition(
      GetBrowserContext(), site_instance.get());
  DOMStorageContextWrapper* dom_storage_context =
      static_cast<DOMStorageContextWrapper*>(partition->GetDOMStorageContext());
  SessionStorageNamespaceImpl* session_storage_namespace_impl =
      static_cast<SessionStorageNamespaceImpl*>(session_storage_namespace);
  CHECK(session_storage_namespace_impl->IsFromContext(dom_storage_context));

  // Added for EFL implementation
  scoped_refptr<SessionStorageNamespace> ssn = session_storage_namespace;
  base::Callback<void(bool)> callback = base::Bind(
      &WebContentsImplEfl::HandleNewWindowRequest, base::Unretained(this),
      render_process_id, render_view_route_id, main_frame_route_id,
      main_frame_widget_route_id, opener->GetRoutingID(),
      base::ConstRef(params), ssn);
  if (efl_delegate_ &&
      efl_delegate_->ShouldCreateWebContentsAsync(callback, params.target_url))
    return;
  // End of EFL port specific code.

  if (delegate_ &&
      !delegate_->ShouldCreateWebContents(
          this, opener, source_site_instance, render_view_route_id,
          main_frame_route_id, main_frame_widget_route_id,
          params.window_container_type, opener->GetLastCommittedURL(),
          params.frame_name, params.target_url, partition_id,
          session_storage_namespace)) {
    CancelWindowRequest(render_process_id, main_frame_route_id);
    return;
  }

  // Added for EFL implementation of WebContentsImp. In non EFL version
  // contents of HandleNewWebContentsCreate would come here.
  callback.Run(true);
}

void WebContentsImplEfl::CancelWindowRequest(int32_t render_process_id,
                                             int32_t main_frame_route_id) {
  // Note: even though we're not creating a WebContents here, it could have
  // been created by the embedder so ensure that the RenderFrameHost is
  // properly initialized.
  // It's safe to only target the frame because the render process will not
  // have a chance to create more frames at this point.
  RenderFrameHostImpl* rfh =
      RenderFrameHostImpl::FromID(render_process_id, main_frame_route_id);
  if (rfh) {
    DCHECK(rfh->IsRenderFrameLive());
    rfh->Init();
  }
}

// This function does not exist in WebContentsImpl. The decision regarding
// new WebContents request is always made synchronously.
void WebContentsImplEfl::HandleNewWindowRequest(
    int32_t render_process_id,
    int32_t render_view_route_id,
    int32_t main_frame_route_id,
    int32_t main_frame_widget_route_id,
    int32_t opener_render_frame_id,
    const mojom::CreateNewWindowParams& params,
    scoped_refptr<SessionStorageNamespace> session_storage_namespace,
    bool create) {
  if (create) {
    scoped_refptr<SessionStorageNamespace> ssn = session_storage_namespace;
    WebContentsEflDelegate::WebContentsCreateCallback callback = base::Bind(
        &WebContentsImplEfl::HandleNewWebContentsCreate, base::Unretained(this),
        render_process_id, render_view_route_id, main_frame_route_id,
        main_frame_widget_route_id, opener_render_frame_id,
        base::ConstRef(params), ssn);
    if (efl_delegate_ && efl_delegate_->WebContentsCreateAsync(callback))
      return;

    callback.Run(NULL);
  } else {
    CancelWindowRequest(render_process_id, main_frame_route_id);
  }
}

WebContents* WebContentsImplEfl::HandleNewWebContentsCreate(
    int32_t render_process_id,
    int32_t render_view_route_id,
    int32_t main_frame_route_id,
    int32_t main_frame_widget_route_id,
    int32_t opener_render_frame_id,
    const mojom::CreateNewWindowParams& params,
    scoped_refptr<SessionStorageNamespace> session_storage_namespace,
    void* platform_data) {
  bool is_guest = BrowserPluginGuest::IsGuest(this);
  scoped_refptr<SiteInstance> site_instance =
      params.opener_suppressed && !is_guest
          ? SiteInstance::CreateForURL(GetBrowserContext(), params.target_url)
          : GetSiteInstance();

  const std::string& partition_id =
      GetContentClient()->browser()->GetStoragePartitionIdForSite(
          GetBrowserContext(), site_instance->GetSiteURL());

  // Create the new web contents. This will automatically create the new
  // WebContentsView. In the future, we may want to create the view separately.
  CreateParams create_params(GetBrowserContext(), site_instance.get());
  create_params.routing_id = render_view_route_id;
  create_params.main_frame_routing_id = main_frame_route_id;
  create_params.main_frame_widget_routing_id = main_frame_widget_route_id;
  create_params.main_frame_name = params.frame_name;
  create_params.opener_render_process_id = render_process_id;
  create_params.opener_render_frame_id = opener_render_frame_id;
  create_params.opener_suppressed = params.opener_suppressed;
  if (params.disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB)
    create_params.initially_hidden = true;
  create_params.renderer_initiated_creation =
      main_frame_route_id != MSG_ROUTING_NONE;

  WebContentsImplEfl* new_contents = nullptr;
  if (!is_guest) {
    create_params.context = view_->GetNativeView();
    create_params.initial_size = GetContainerBounds().size();
    // Added for EFL port
    new_contents =
        new WebContentsImplEfl(create_params.browser_context, platform_data);

    if (!new_contents)
      return nullptr;

    FrameTreeNode* opener = nullptr;
    if (create_params.opener_render_frame_id != MSG_ROUTING_NONE) {
      RenderFrameHostImpl* opener_rfh =
          RenderFrameHostImpl::FromID(create_params.opener_render_process_id,
                                      create_params.opener_render_frame_id);
      if (opener_rfh)
        opener = opener_rfh->frame_tree_node();
    }

    if (!create_params.opener_suppressed && opener) {
      new_contents->GetFrameTree()->root()->SetOpener(opener);
      new_contents->created_with_opener_ = true;
    }
    new_contents->Init(create_params);
    // End of EFL port specific code.
  } else {
    // TODO(t.czekala): WebContents and BrowserPluginGuest are integrated and
    // it should be checked if we can create WebContentsImplEfl in this scenario
    NOTREACHED();
    return nullptr;
  }

  new_contents->GetController().SetSessionStorageNamespace(
      partition_id, session_storage_namespace.get());

  // Added for EFL port
  new_contents->platform_data_ = platform_data;
  if (efl_delegate_)
    efl_delegate_->SetUpSmartObject(platform_data,
                                    new_contents->GetNativeView());
  // End of EFL port specific code.

  // If the new frame has a name, make sure any SiteInstances that can find
  // this named frame have proxies for it.  Must be called after
  // SetSessionStorageNamespace, since this calls CreateRenderView, which uses
  // GetSessionStorageNamespace.
  if (!params.frame_name.empty())
    new_contents->GetRenderManager()->CreateProxiesForNewNamedFrame();

  // Save the window for later if we're not suppressing the opener (since it
  // will be shown immediately).
  if (!params.opener_suppressed) {
    if (!is_guest) {
      WebContentsView* new_view = new_contents->view_.get();

      // TODO(brettw): It seems bogus that we have to call this function on the
      // newly created object and give it one of its own member variables.
      new_view->CreateViewForWidget(
          new_contents->GetRenderViewHost()->GetWidget(), false);
    }
    // Save the created window associated with the route so we can show it
    // later.
    DCHECK_NE(MSG_ROUTING_NONE, main_frame_widget_route_id);
    pending_contents_[std::make_pair(
        render_process_id, main_frame_widget_route_id)] = new_contents;
    AddDestructionObserver(new_contents);
  }

  if (delegate_) {
    delegate_->WebContentsCreated(this, render_process_id,
                                  opener_render_frame_id, params.frame_name,
                                  params.target_url, new_contents);
  }

  // Any new WebContents opened while this WebContents is in fullscreen can be
  // used to confuse the user, so drop fullscreen.
  if (IsFullscreenForCurrentTab())
    ExitFullscreen(true);

  if (params.opener_suppressed) {
    // When the opener is suppressed, the original renderer cannot access the
    // new window.  As a result, we need to show and navigate the window here.
    bool was_blocked = false;
    if (delegate_) {
      gfx::Rect initial_rect;
      base::WeakPtr<WebContentsImpl> weak_new_contents =
          new_contents->weak_factory_.GetWeakPtr();

      delegate_->AddNewContents(this, new_contents, params.disposition,
                                initial_rect, params.user_gesture,
                                &was_blocked);

      if (!weak_new_contents) {
        // The delegate deleted |new_contents| during AddNewContents().
        return nullptr;
      }
    }
    if (!was_blocked) {
      OpenURLParams open_params(params.target_url, params.referrer,
                                WindowOpenDisposition::CURRENT_TAB,
                                ui::PAGE_TRANSITION_LINK,
                                true /* is_renderer_initiated */);
      open_params.user_gesture = params.user_gesture;

      if (delegate_ && !is_guest &&
          !delegate_->ShouldResumeRequestsForCreatedWindow()) {
        // We are in asynchronous add new contents path, delay opening url
        new_contents->delayed_open_url_params_.reset(
            new OpenURLParams(open_params));
      } else {
        new_contents->OpenURL(open_params);
      }
    }
  }

  return new_contents;
}

#if defined(OS_TIZEN_TV_PRODUCT)
void WebContentsImplEfl::SetIMERecommendedWords(
    const std::string& recommended_words) {
  RenderWidgetHostViewEfl* rwhv =
      static_cast<RenderWidgetHostViewEfl*>(GetRenderWidgetHostView());
  if (!rwhv)
    return;
  rwhv->SetIMERecommendedWords(recommended_words);
}

void WebContentsImplEfl::SetIMERecommendedWordsType(bool should_enable) {
  RenderWidgetHostViewEfl* rwhv =
      static_cast<RenderWidgetHostViewEfl*>(GetRenderWidgetHostView());
  if (!rwhv)
    return;
  rwhv->SetIMERecommendedWordsType(should_enable);
}
#endif

}  // namespace content
