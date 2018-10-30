// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/scoped_java_ref.h"
#include "base/android/unguessable_token_android.h"
#include "content/browser/android/scoped_surface_request_manager.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/media/android/browser_media_player_manager.h"
#include "content/browser/media/android/media_web_contents_observer_android.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"
#include "gpu/ipc/common/gpu_surface_tracker.h"

#include "jni/GpuProcessCallback_jni.h"

namespace content {

namespace {

// Pass a java surface object to the MediaPlayerAndroid object
// identified by render process handle, render frame ID and player ID.
static void SetSurfacePeer(const base::android::JavaRef<jobject>& surface,
                           base::ProcessHandle render_process_handle,
                           int render_frame_id,
                           int player_id) {
  int render_process_id = 0;
  RenderProcessHost::iterator it = RenderProcessHost::AllHostsIterator();
  while (!it.IsAtEnd()) {
    if (it.GetCurrentValue()->GetHandle() == render_process_handle) {
      render_process_id = it.GetCurrentValue()->GetID();
      break;
    }
    it.Advance();
  }
  if (!render_process_id) {
    DVLOG(1) << "Cannot find render process for render_process_handle "
             << render_process_handle;
    return;
  }

  RenderFrameHostImpl* frame =
      RenderFrameHostImpl::FromID(render_process_id, render_frame_id);
  if (!frame) {
    DVLOG(1) << "Cannot find frame for render_frame_id " << render_frame_id;
    return;
  }

  BrowserMediaPlayerManager* player_manager =
      MediaWebContentsObserverAndroid::FromWebContents(
          WebContents::FromRenderFrameHost(frame))
          ->GetMediaPlayerManager(frame);
  if (!player_manager) {
    DVLOG(1) << "Cannot find the media player manager for frame " << frame;
    return;
  }

  media::MediaPlayerAndroid* player = player_manager->GetPlayer(player_id);
  if (!player) {
    DVLOG(1) << "Cannot find media player for player_id " << player_id;
    return;
  }

  if (player != player_manager->GetFullscreenPlayer()) {
    gl::ScopedJavaSurface scoped_surface(surface);
    player->SetVideoSurface(std::move(scoped_surface));
  }
}

}  // anonymous namespace

void EstablishSurfacePeer(JNIEnv* env,
                          const base::android::JavaParamRef<jclass>& clazz,
                          jint pid,
                          const base::android::JavaParamRef<jobject>& surface,
                          jint primary_id,
                          jint secondary_id) {
  base::android::ScopedJavaGlobalRef<jobject> jsurface;
  jsurface.Reset(env, surface);
  if (jsurface.is_null())
    return;

  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::UI));
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&SetSurfacePeer, jsurface, pid, primary_id, secondary_id));
}

void CompleteScopedSurfaceRequest(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz,
    const base::android::JavaParamRef<jobject>& token,
    const base::android::JavaParamRef<jobject>& surface) {
  base::UnguessableToken requestToken =
      base::android::UnguessableTokenAndroid::FromJavaUnguessableToken(env,
                                                                       token);
  if (!requestToken) {
    DLOG(ERROR) << "Received invalid surface request token.";
    return;
  }

  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::UI));

  base::android::ScopedJavaGlobalRef<jobject> jsurface;
  jsurface.Reset(env, surface);
  ScopedSurfaceRequestManager::GetInstance()->FulfillScopedSurfaceRequest(
      requestToken, gl::ScopedJavaSurface(jsurface));
}

base::android::ScopedJavaLocalRef<jobject> GetViewSurface(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& jcaller,
    jint surface_id) {
  gl::ScopedJavaSurface surface_view =
      gpu::GpuSurfaceTracker::GetInstance()->AcquireJavaSurface(surface_id);
  return base::android::ScopedJavaLocalRef<jobject>(surface_view.j_surface());
}

}  // namespace content
