// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_TERRACE_CONTENT_BRIDGE_H_
#define CONTENT_PUBLIC_BROWSER_TERRACE_CONTENT_BRIDGE_H_

#include "base/android/jni_android.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/password_manager/core/browser/password_store_default.h"
#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
#include "ui/base/page_transition_types.h"
#include "ui/gfx/native_widget_types.h"

namespace IPC {
class Sender;
}

namespace autofill {
struct PasswordForm;
}

namespace blink {
class WebFrame;
class WebMediaPlayer;
class WebMediaPlayerClient;
class WebMediaPlayerEncryptedMediaClient;
}

namespace media {
class WebMediaPlayerDelegate;
class WebMediaPlayerParams;
}

class GURL;

namespace content {

class AudioFocusDelegate;
class ContentViewCore;
class ContentViewRenderView;
class RenderFrame;
class RendererMediaPlayerManager;
class RenderWidgetHostImpl;
class RenderWidgetHostViewAndroid;
class MediaSessionImpl;
class MediaWebContentsObserver;
class StreamTextureFactory;
class WebContents;
class WebContentsImpl;

class CONTENT_EXPORT TerraceContentBridge {
 public:
  static TerraceContentBridge& Get();

  static void SetInstance(TerraceContentBridge* instance);

  virtual ContentViewCore* CreateContentViewCore(
      JNIEnv* env,
      const base::android::JavaRef<jobject>& obj,
      content::WebContents* web_contents,
      float dpi_scale) = 0;
  virtual ContentViewRenderView* CreateContentViewRenderView(
      JNIEnv* env,
      jobject obj,
      gfx::NativeWindow root_window) = 0;
  virtual MediaWebContentsObserver* CreateMediaWebContentsObserver(
      WebContentsImpl* web_contents) = 0;
  virtual RenderWidgetHostViewAndroid* CreateRenderWidgetHostViewAndroid(
      RenderWidgetHostImpl* widget,
      ContentViewCore* content_view_core) = 0;
  virtual blink::WebMediaPlayer* CreateWebMediaPlayer(
      blink::WebFrame* frame,
      blink::WebMediaPlayerClient* client,
      blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
      const base::WeakPtr<media::WebMediaPlayerDelegate>& delegate,
      content::RendererMediaPlayerManager* player_manager,
      const scoped_refptr<content::StreamTextureFactory>& factory,
      int frame_id,
      bool enable_texture_copy,
      std::unique_ptr<media::WebMediaPlayerParams> params) = 0;
  virtual RendererMediaPlayerManager* CreateMediaPlayerManager(
      RenderFrame* render_frame) = 0;

  virtual bool TrackerBlockResourceFilter(const GURL& request_url,
                                          const GURL& site_for_cookies,
                                          content::ResourceType resource_type,
                                          int child_id) = 0;

  virtual bool ContentBlockerResourceFilter(
      const GURL& request_url,
      const GURL& site_for_cookies,
      content::ResourceType resource_type,
      int route_id,
      int child_id,
      ui::PageTransition transition_type,
      IPC::Sender* request_dispatch_message_filter) = 0;

  virtual bool ContentBlockerPopupResourceFilter(
      const GURL& request_url,
      content::ResourceType resource_type,
      const int child_id,
      const int render_frame_id) = 0;

  virtual void OnChildProcessDied(const int child_id) const = 0;

  virtual bool IsActivatedAsEasySigninGetLogins() const = 0;

  virtual bool IsCachedActivatedAsEasySignin() const = 0;

  virtual void ShowMoveToEasySignin() const = 0;

  virtual void OnAddLoginAsEasySignin(const autofill::PasswordForm& form) = 0;

  virtual std::vector<std::unique_ptr<autofill::PasswordForm>>
  GetLoginsAsEasySignin(
      const password_manager::PasswordStore::FormDigest& form) = 0;

  virtual std::unique_ptr<content::AudioFocusDelegate>
      CreateAudioFocusDelegate(content::MediaSessionImpl* media_session) = 0;

  virtual void SendEventLog(const std::string& screen_id,
      const std::string& event_name) = 0;

  virtual void ResetPageZoom(content::WebContents* web_contents) = 0;

  virtual void InitCustomLogger() = 0;
  virtual bool PreventSystemPopup() = 0;
  virtual bool StartLogging(const std::string& type,
                            const std::string& source,
                            const std::string& crash_info) = 0;
  virtual void SetCrashKeyValue(const base::StringPiece& key,
                                const base::StringPiece& value) = 0;

 protected:
  TerraceContentBridge() = default;
  virtual ~TerraceContentBridge() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(TerraceContentBridge);
};

} // namespace content

#endif // CONTENT_PUBLIC_BROWSER_TERRACE_CONTENT_BRIDGE_H_
