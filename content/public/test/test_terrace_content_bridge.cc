// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(S_TERRACE_SUPPORT)

#include "content/public/test/test_terrace_content_bridge.h"

#include "content/browser/android/content_view_core_impl.h"
#include "content/browser/android/content_view_render_view.h"
#include "content/browser/media/android/media_web_contents_observer_android.h"
#include "content/browser/media/session/audio_focus_delegate_android.h"
#include "content/renderer/media/android/renderer_media_player_manager.h"

namespace content {
TestTerraceContentBridge::~TestTerraceContentBridge() = default;

ContentViewCore* TestTerraceContentBridge::CreateContentViewCore(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj,
    content::WebContents* web_contents,
    float dpi_scale,
    const base::android::JavaRef<jobject>& java_bridge_retained_object_set) {
  return new ContentViewCore(
      env, obj, web_contents, dpi_scale, java_bridge_retained_object_set);
}

ContentViewRenderView* TestTerraceContentBridge::CreateContentViewRenderView(
    JNIEnv* env,
    jobject obj,
    gfx::NativeWindow root_window) {
  return new ContentViewRenderView(env, obj, root_window);
}

MediaWebContentsObserver*
TestTerraceContentBridge::CreateMediaWebContentsObserver(
    WebContentsImpl* web_contents) {
  return new MediaWebContentsObserverAndroid(web_contents);
}

RenderWidgetHostViewAndroid*
TestTerraceContentBridge::CreateRenderWidgetHostViewAndroid(
    RenderWidgetHostImpl* widget,
    ContentViewCore* content_view_core) {
  return new RenderWidgetHostViewAndroid(widget, content_view_core);
}

blink::WebMediaPlayer* TestTerraceContentBridge::CreateWebMediaPlayer(
    blink::WebFrame* frame,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    const base::WeakPtr<media::WebMediaPlayerDelegate>& delegate,
    RendererMediaPlayerManager* player_manager,
    const scoped_refptr<StreamTextureFactory>& factory,
    int frame_id,
    bool enable_texture_copy,
    const media::WebMediaPlayerParams& params) {
  return NULL;
}

RendererMediaPlayerManager* TestTerraceContentBridge::CreateMediaPlayerManager(
    RenderFrame* render_frame) {
  return new RendererMediaPlayerManager(render_frame);
}

bool TestTerraceContentBridge::TrackerBlockResourceFilter(
    const GURL& request_url,
    const GURL& site_for_cookies,
    content::ResourceType resource_type,
    int child_id) {
  return false;
}

bool TestTerraceContentBridge::ContentBlockerResourceFilter(
    const GURL& request_url,
    const GURL& site_for_cookies,
    ResourceType resource_type,
    int route_id,
    int child_id,
    ui::PageTransition transition_type,
    IPC::Sender* request_dispatch_message_filter) {
  return false;
}

bool TestTerraceContentBridge::ContentBlockerPopupResourceFilter(
    const GURL& request_url,
    content::ResourceType resource_type,
    const int child_id,
    const int render_frame_id) {
  return false;
}

void TestTerraceContentBridge::OnChildProcessDied(const int child_id) const {
}

bool TestTerraceContentBridge::IsActivatedAsEasySigninGetLogins() const {
  return false;
}

bool TestTerraceContentBridge::IsCachedActivatedAsEasySignin() const {
  return false;
}

void TestTerraceContentBridge::ShowMoveToEasySignin() const {
}

void TestTerraceContentBridge::OnAddLoginAsEasySignin(
    const autofill::PasswordForm& form) {
}

std::vector<std::unique_ptr<autofill::PasswordForm>>
TestTerraceContentBridge::GetLoginsAsEasySignin(
    const password_manager::PasswordStore::FormDigest& form) {
  return std::vector<std::unique_ptr<autofill::PasswordForm>>();
}

void TestTerraceContentBridge::SendEventLog(const std::string& screen_id,
    const std::string& event_name) {
}

void TestTerraceContentBridge::ResetPageZoom(
    content::WebContents* web_contents) {}

void TestTerraceContentBridge::InitCustomLogger() {}

bool TestTerraceContentBridge::PreventSystemPopup() {
  return true;
}

bool TestTerraceContentBridge::StartLogging(const std::string& type,
                                            const std::string& source,
                                            const std::string& crash_info) {
  return true;
}

void TestTerraceContentBridge::SetCrashKeyValue(
    const base::StringPiece& key,
    const base::StringPiece& value) {}

std::unique_ptr<AudioFocusDelegate>
    TestTerraceContentBridge::CreateAudioFocusDelegate(
        MediaSessionImpl* media_session) {
  AudioFocusDelegateAndroid* delegate =
      new AudioFocusDelegateAndroid(media_session);
  delegate->Initialize();
  return std::unique_ptr<AudioFocusDelegate>(delegate);
}

} //  namespace content

#endif // S_TERRACE_SUPPORT
