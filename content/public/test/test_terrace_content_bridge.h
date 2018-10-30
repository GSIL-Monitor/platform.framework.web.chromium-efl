// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(S_TERRACE_SUPPORT)

#ifndef CONTENT_PUBLIC_TEST_TERRACE_CONTENT_BRIDGE_H_
#define CONTENT_PUBLIC_TEST_TERRACE_CONTENT_BRIDGE_H_

#include "content/browser/media/session/audio_focus_delegate.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/terrace_content_bridge.h"

namespace content {
class TestTerraceContentBridge : public TerraceContentBridge {
 public:
  TestTerraceContentBridge() = default;
  ~TestTerraceContentBridge() override;

 private:
  content::ContentViewCore* CreateContentViewCore(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& obj,
      content::WebContents* web_contents,
      float dpi_scale,
      const base::android::JavaRef<jobject>&
          java_bridge_retained_object_set) override;
  content::ContentViewRenderView* CreateContentViewRenderView(
      JNIEnv* env,
      jobject obj,
      gfx::NativeWindow root_window) override;
  content::MediaWebContentsObserver* CreateMediaWebContentsObserver(
        content::WebContentsImpl* web_contents) override;
  content::RenderWidgetHostViewAndroid* CreateRenderWidgetHostViewAndroid(
      content::RenderWidgetHostImpl* widget,
      content::ContentViewCoreImpl* content_view_core) override;
  blink::WebMediaPlayer* CreateWebMediaPlayer(
      blink::WebFrame* frame,
      blink::WebMediaPlayerClient* client,
      blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
      const base::WeakPtr<media::WebMediaPlayerDelegate>& delegate,
      content::RendererMediaPlayerManager* player_manager,
      const scoped_refptr<content::StreamTextureFactory>& factory,
      int frame_id,
      bool enable_texture_copy,
      const media::WebMediaPlayerParams& params) override;
  content::RendererMediaPlayerManager* CreateMediaPlayerManager(
      content::RenderFrame* render_frame) override;

  bool TrackerBlockResourceFilter(const GURL& request_url,
                                  const GURL& site_for_cookies,
                                  content::ResourceType resource_type,
                                  int child_id) override;

  bool ContentBlockerResourceFilter(
      const GURL& request_url,
      const GURL& site_for_cookies,
      content::ResourceType resource_type,
      int route_id,
      int child_id,
      ui::PageTransition transition_type,
      IPC::Sender* request_dispatch_message_filter) override;

  bool ContentBlockerPopupResourceFilter(const GURL& request_url,
                                         content::ResourceType resource_type,
                                         const int child_id,
                                         const int render_frame_id) override;

  void OnChildProcessDied(const int child_id) const override;

  bool IsActivatedAsEasySigninGetLogins() const override;

  bool IsCachedActivatedAsEasySignin() const override;

  void ShowMoveToEasySignin() const override;

  void OnAddLoginAsEasySignin(const autofill::PasswordForm& form) override;

  std::vector<std::unique_ptr<autofill::PasswordForm>> GetLoginsAsEasySignin(
      const password_manager::PasswordStore::FormDigest& form) override;

  void SendEventLog(const std::string& screen_id,
      const std::string& event_name) override;

  void ResetPageZoom(content::WebContents* web_contents) override;

  void InitCustomLogger() override;
  bool PreventSystemPopup() override;
  bool StartLogging(const std::string& type,
                    const std::string& source,
                    const std::string& crash_info) override;
  void SetCrashKeyValue(const base::StringPiece& key,
                        const base::StringPiece& value) override;

  std::unique_ptr<AudioFocusDelegate> CreateAudioFocusDelegate(
      MediaSessionImpl* media_session) override;

  DISALLOW_COPY_AND_ASSIGN(TestTerraceContentBridge);
};

} //  namespace content

#endif // CONTENT_PUBLIC_TEST_TERRACE_CONTENT_BRIDGE_H_

#endif // S_TERRACE_SUPPORT
