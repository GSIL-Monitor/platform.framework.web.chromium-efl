// Copyright 2014-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/web_view_browser_message_filter.h"

#include "browser/sound_effect.h"
#include "common/web_contents_utils.h"
#include "common/render_messages_ewk.h"
#include "common/hit_test_params.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#include "eweb_view.h"
#include "private/ewk_hit_test_private.h"
#include "ipc_message_start_ewk.h"

using content::BrowserThread;
using content::RenderViewHost;
using content::WebContents;
using namespace web_contents_utils;

namespace {
static const uint32_t kFilteredMessageClasses[] = { EwkMsgStart, ViewMsgStart };
}

class WebViewBrowserMessageFilterPrivate
    : public content::NotificationObserver {
 public:
  WebViewBrowserMessageFilterPrivate(WebContents* web_contents) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    registrar_.Add(this, content::NOTIFICATION_WEB_CONTENTS_DESTROYED,
        content::Source<content::WebContents>(web_contents));

    web_view_ = WebViewFromWebContents(web_contents);
    CHECK(web_view_);
  }

  /* LCOV_EXCL_START */
  void OnReceivedHitTestData(int render_view,
                             const Hit_Test_Params& params) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    if (!web_view_)
      return;

    RenderViewHost* render_view_host =
        web_view_->web_contents().GetRenderViewHost();
    DCHECK(render_view_host);

    if (render_view_host && render_view_host->GetRoutingID() == render_view)
      web_view_->UpdateHitTestData(params);
  }

  void OnReceivedHitTestAsyncData(int render_view,
                                  const Hit_Test_Params& params,
                                  int64_t request_id) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    // Only this method is called from renderer process. In particular case
    // WebContents may be destroyed here, so additional check is needed.
    if (!web_view_)
      return;

    RenderViewHost* render_view_host =
        web_view_->web_contents().GetRenderViewHost();
    DCHECK(render_view_host);

    if (render_view_host && render_view_host->GetRoutingID() == render_view) {
      web_view_->DispatchAsyncHitTestData(params, request_id);
    }
  }

  void OnPlainTextGetContents(const std::string& content_text, int plain_text_get_callback_id) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    if (web_view_)
      web_view_->InvokePlainTextGetCallback(content_text, plain_text_get_callback_id);
  }

  void OnWebAppIconUrlGet(const std::string &icon_url, int callback_id) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    if (web_view_)
      web_view_->InvokeWebAppIconUrlGetCallback(icon_url, callback_id);
  }

  void OnWebAppIconUrlsGet(const std::map<std::string, std::string> &icon_urls, int callback_id) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    if (web_view_)
      web_view_->InvokeWebAppIconUrlsGetCallback(icon_urls, callback_id);
  }

  void OnWebAppCapableGet(bool capable, int callback_id) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    if (web_view_)
      web_view_->InvokeWebAppCapableGetCallback(capable, callback_id);
  }
  /* LCOV_EXCL_STOP */

  void OnDidChangeContentsSize(int width, int height) {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
    if (web_view_)
      web_view_->DidChangeContentsSize(width, height);
  }

  /* LCOV_EXCL_START */
  void OnPageScaleFactorChanged(double scale_factor) {
    if (web_view_)
      web_view_->DidChangePageScaleFactor(scale_factor);
  }

  void OnMHTMLContentGet(const std::string& mhtml_content, int callback_id) {
    if (web_view_)
      web_view_->OnMHTMLContentGet(mhtml_content, callback_id);
  }

  void OnGetBackgroundColor(int r, int g, int b, int a, int callback_id) {
    if (web_view_)
      web_view_->OnGetBackgroundColor(r, g, b, a, callback_id);
  }

  void OnStartContentIntent(const GURL& content_url, bool is_main_frame) {
    // [M48_2564] Need to apply the new is_main_frame argument in this function
    //            FIXME: http://web.sec.samsung.net/bugzilla/show_bug.cgi?id=15388
    DCHECK(!content_url.is_empty());
    web_view_->ShowContentsDetectedPopup(content_url.spec().c_str());
  }

  void OnGetMainFrameScrollbarVisible(bool visible, int callback_id) {
    if (web_view_)
      web_view_->InvokeMainFrameScrollbarVisibleCallback(visible, callback_id);
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  void OnRequestHitTestForMouseLongPressACK(int render_view,
                                            const GURL& url,
                                            int render_id,
                                            int x,
                                            int y) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    auto web_contents = WebContentsFromViewID(render_id, render_view);
    if (web_contents) {
      WebViewFromWebContents(web_contents)
          ->ShowContextMenuWithLongPressed(url, x, y);
    }
  }
#endif

  /* LCOV_EXCL_STOP */

  void Observe(int type, const content::NotificationSource& source,
      const content::NotificationDetails& details) override {
    DCHECK_EQ(content::NOTIFICATION_WEB_CONTENTS_DESTROYED, type);
    web_view_ = NULL;
  }

 private:
  EWebView* web_view_;
  content::NotificationRegistrar registrar_;
};


WebViewBrowserMessageFilter::WebViewBrowserMessageFilter(
    content::WebContents* web_contents)
    : BrowserMessageFilter(kFilteredMessageClasses,
                           arraysize(kFilteredMessageClasses)),
      private_(new WebViewBrowserMessageFilterPrivate(web_contents)) {
}

WebViewBrowserMessageFilter::~WebViewBrowserMessageFilter() {
  // Due to NotificationRegistrar private_ shall be destroyed in UI thread
  BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, private_);
}

void WebViewBrowserMessageFilter::OverrideThreadForMessage(
    const IPC::Message& message, BrowserThread::ID* thread) {
  switch (message.type()) {
    case EwkViewHostMsg_HitTestAsyncReply::ID:
    case EwkHostMsg_PlainTextGetContents::ID:
    case EwkHostMsg_WebAppIconUrlGet::ID:
    case EwkHostMsg_WebAppIconUrlsGet::ID:
    case EwkHostMsg_WebAppCapableGet::ID:
    case EwkHostMsg_DidChangeContentsSize::ID:
    case ViewHostMsg_PageScaleFactorChanged::ID:
#if defined(OS_TIZEN_TV_PRODUCT)
    case EwkViewHostMsg_RequestHitTestForMouseLongPressACK::ID:
#endif
      *thread = BrowserThread::UI;
      break;
  }
}

bool WebViewBrowserMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebViewBrowserMessageFilterPrivate, message)
    /* LCOV_EXCL_START */
    IPC_MESSAGE_FORWARD(EwkViewHostMsg_HitTestReply, private_,
        WebViewBrowserMessageFilterPrivate::OnReceivedHitTestData)
    IPC_MESSAGE_FORWARD(EwkViewHostMsg_HitTestAsyncReply, private_,
        WebViewBrowserMessageFilterPrivate::OnReceivedHitTestAsyncData)
    IPC_MESSAGE_FORWARD(EwkHostMsg_PlainTextGetContents, private_,
        WebViewBrowserMessageFilterPrivate::OnPlainTextGetContents)
    IPC_MESSAGE_FORWARD(EwkHostMsg_WebAppIconUrlGet, private_,
        WebViewBrowserMessageFilterPrivate::OnWebAppIconUrlGet)
    IPC_MESSAGE_FORWARD(EwkHostMsg_WebAppIconUrlsGet, private_,
        WebViewBrowserMessageFilterPrivate::OnWebAppIconUrlsGet)
    IPC_MESSAGE_FORWARD(EwkHostMsg_WebAppCapableGet, private_,
        WebViewBrowserMessageFilterPrivate::OnWebAppCapableGet)
    /* LCOV_EXCL_STOP */
    IPC_MESSAGE_FORWARD(EwkHostMsg_DidChangeContentsSize, private_,
        WebViewBrowserMessageFilterPrivate::OnDidChangeContentsSize)
    /* LCOV_EXCL_START */
    IPC_MESSAGE_FORWARD(EwkHostMsg_ReadMHTMLData, private_,
        WebViewBrowserMessageFilterPrivate::OnMHTMLContentGet)
    IPC_MESSAGE_FORWARD(
        EwkHostMsg_GetBackgroundColor, private_,
        WebViewBrowserMessageFilterPrivate::OnGetBackgroundColor)
    IPC_MESSAGE_FORWARD(ViewHostMsg_PageScaleFactorChanged, private_,
        WebViewBrowserMessageFilterPrivate::OnPageScaleFactorChanged)
    IPC_MESSAGE_FORWARD(ViewHostMsg_StartContentIntent, private_,
        WebViewBrowserMessageFilterPrivate::OnStartContentIntent)
    IPC_MESSAGE_FORWARD(
        EwkHostMsg_GetMainFrameScrollbarVisible, private_,
        WebViewBrowserMessageFilterPrivate::OnGetMainFrameScrollbarVisible)
#if defined(OS_TIZEN_TV_PRODUCT)
    IPC_MESSAGE_FORWARD(EwkViewHostMsg_RequestHitTestForMouseLongPressACK,
                        private_,
                        WebViewBrowserMessageFilterPrivate::
                            OnRequestHitTestForMouseLongPressACK)
#endif
    /* LCOV_EXCL_STOP */
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}
