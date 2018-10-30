// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/media_player_bridge_capi.h"

#include <player_internal.h>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/media/tizen/media_storage_helper.h"
#include "content/public/browser/browser_context.h"
#include "media/base/tizen/media_player_manager_efl.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "net/base/data_url.h"
#include "third_party/libyuv/include/libyuv/planar_functions.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/size.h"
#include "url/url_util.h"

#if defined(TIZEN_SOUND_FOCUS)
#include "media/base/tizen/sound_focus_manager.h"
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
#include <marlin_ms3_api.h>
#include <player_product.h>
#include <vconf/vconf.h>
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "ewk/efl_integration/common/content_switches_efl.h"
#include "tizen_src/ewk/efl_integration/common/application_type.h"
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
#include <Elementary.h>
#include <Evas.h>
#include "ui/gl/egl_util.h"
#endif

#if defined(TIZEN_VIDEO_HOLE)
#include "media/base/tizen/video_plane_manager_tizen.h"
#endif

namespace {

// Update duration every 100ms.
const int kDurationUpdateInterval = 100;
const int64_t kDrmTimeoutSeconds = 20;

enum PlayerState {
  PLAYER_STATE_DELAYED_NULL,
  PLAYER_STATE_DELAYED_PLAY,
  PLAYER_STATE_DELAYED_PAUSE,
  PLAYER_STATE_DELAYED_SEEK,
};

// TODO(max): Remove these proxy free functions.

// Called by player_prepare_async()
// It's guaranteed that no callback after player destructed.
static void PlayerPreparedCb(void* data) {
  DCHECK(data);
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(data);

  player->OnPlayerPrepared();
}

static void MediaPacketDecodedCb(media_packet_h packet, void* data) {
  DCHECK(data);
  if (!packet) {
    LOG(ERROR) << "media_packet handle is null";
    return;
  }
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(data);

  player->OnMediaPacketUpdated(packet);
}

// Called by player_set_completed_cb()
static void PlaybackCompleteCb(void* data) {
  DCHECK(data);
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(data);

  player->OnPlaybackCompleteUpdate();
}

// Called by player_set_play_position() / player_set_position()
static void SeekCompletedCb(void* data) {
  DCHECK(data);
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(data);

  player->OnSeekCompleteUpdate();
}

// Called by player_set_buffering_cb()
static void ChangedBufferingStatusCb(int percent, void* data) {
  DCHECK(data);
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(data);

  player->OnHandleBufferingStatus(percent);
}

// Called by player_set_error_cb()
static void ErrorCb(int error_code, void* data) {
  DCHECK(data);
  // when connection failed, mmplayer may retry some times
  if (error_code == PLAYER_ERROR_CONNECTION_FAILED)
    return;

  if (error_code == PLAYER_ERROR_NOT_SUPPORTED_SUBTITLE)
    return;

  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(data);

  player->OnHandlePlayerError(error_code, FROM_HERE);
}

// Called by player_set_interrupted_cb
static void InterruptedCb(player_interrupted_code_e code, void* data) {
  DCHECK(data);
  if (code != PLAYER_INTERRUPTED_BY_RESOURCE_CONFLICT) {
    return;
  }

  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(data);
#if defined(OS_TIZEN_TV_PRODUCT)
  player->OnResourceConflict();
#else
  player->OnInterrupted(code);
#endif
}

#if defined(OS_TIZEN_TV_PRODUCT)
// Called by player_preloading_async() for HBBTV media
static void PlayerPreLoadingCb(void* data) {
  DCHECK(data);
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(data);
  player->OnPlayerPreloading();
}

static void DrmErrorCb(int err_code, char* err_str, void* data) {
  DCHECK(data);
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(data);

  player->OnDrmError(err_code, err_str);
}

// called by player_set_others_event_cb
static void OtherEventCb(int event_type, void* event_data, void* data) {
  DCHECK(data);
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(data);
  player->OnOtherEvent(event_type, event_data);
}

static void RegisterTimelineCb(const char* timeline_selector,
                               guint32 units_per_tick,
                               guint32 units_per_second,
                               guint64 content_time,
                               ETimelineState timeline_state,
                               void* user_data) {
  DCHECK(user_data);
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(user_data);
  player->OnRegisterTimeline(timeline_selector, units_per_tick,
                             units_per_second, content_time, timeline_state);
}

static void SyncTimelineCb(const char* timeline_selector,
                           int sync,
                           void* user_data) {
  DCHECK(user_data);
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(user_data);
  player->OnSyncTimeline(timeline_selector, sync);
}

// call by player_open_subtitle_cb()
static void OnSubtitleDataCallback(unsigned long long time_stamp,
                                   unsigned index,
                                   void* buffer,
                                   void* user_data) {
  DCHECK(user_data);
  media::MediaPlayerBridgeCapi* player =
      static_cast<media::MediaPlayerBridgeCapi*>(user_data);
  if (buffer) {
    unsigned buffer_size = strlen(static_cast<char*>(buffer)) + 1;
    char* bufstr = static_cast<char*>(buffer);
    const std::string info(bufstr ? bufstr : "");

    LOG(INFO) << " onSubtitleDataCallback : index=" << index
              << ",time stamp=" << time_stamp << ",bufferSize=" << buffer_size;
    player->SubtitleDataCB(time_stamp, index, info, buffer_size);
  }
}
#endif

}  // namespace

namespace media {

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
player_h MediaPlayerBridgeCapi::mixer_player_group_[kMaxPlayerMixerNum] = {
    0,
};
player_h MediaPlayerBridgeCapi::mixer_player_active_audio_ = 0;
int MediaPlayerBridgeCapi::mixer_decoder_hw_state_[2] = {NOT_USED, NOT_USED};
#endif

// static
MediaPlayerEfl* MediaPlayerEfl::CreatePlayer(int player_id,
                                             const GURL& url,
                                             const std::string& mime_type,
                                             MediaPlayerManager* manager,
                                             const std::string& ua,
                                             int audio_latency_mode,
                                             bool video_hole) {
  LOG(INFO) << "MediaElement is using |CAPI| to play media";
  return new MediaPlayerBridgeCapi(player_id, url, mime_type, manager, ua,
                                   audio_latency_mode, video_hole);
}

MediaPlayerBridgeCapi::MediaPlayerBridgeCapi(int player_id,
                                             const GURL& url,
                                             const std::string& mime_type,
                                             MediaPlayerManager* manager,
                                             const std::string& user_agent,
                                             int audio_latency_mode,
                                             bool video_hole)
    : MediaPlayerEfl(player_id, manager, video_hole),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      url_(url),
      volume_(0),
      mime_type_(mime_type),
      is_end_reached_(false),
      is_file_url_(false),
      is_seeking_(false),
      is_resuming_(false),
      is_initialized_(false),
      is_media_related_action_(false),
      is_getting_cookies_(false),
      error_occured_(false),
      pending_prepare_(false),
      stored_seek_time_during_resume_(media::kNoTimestamp),
      audio_latency_mode_(
          static_cast<audio_latency_mode_e>(audio_latency_mode)),
      current_progress_(0),
      user_agent_(user_agent),
#if defined(OS_TIZEN_TV_PRODUCT)
      is_buffering_compeleted_(false),
      is_inband_text_track_(false),
      is_live_stream_(false),
      is_player_released_(false),
      is_player_seek_available_(true),
      need_report_buffering_state_(true),
      player_started_(false),
      active_audio_track_id_(-1),
      active_text_track_id_(-1),
      active_video_track_id_(-1),
      last_prefer_audio_adaptionset_idx_(-1),
      pending_active_audio_track_id_(-1),
      pending_active_text_track_id_(-1),
      pending_active_video_track_id_(-1),
      player_ready_state_(blink::WebMediaPlayer::kReadyStateHaveNothing),
      prefer_audio_adaptionset_idx_(-1),
      subtitle_state_(blink::WebMediaPlayer::kSubtitleStop),
      stream_type_(OTHER_STREAM),
      clean_url_(""),
      content_id_(""),
      data_url_(""),
      hbbtv_url_(""),
      mrs_url_(""),
      decryptor_{0},
      decryptor_condition_(&decryptor_lock_),
#endif
#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
      player_index_(0),
      mixer_decoder_hw_main_(false),
      mixer_decoder_hw_sub_(false),
      mixer_decoder_sw_(false),
#endif
      weak_factory_(this) {
#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser()) {
    LOG(INFO) << "Enter multi video mixer mode";
    // Player index selector
    for (int i = 0; i < kMaxPlayerMixerNum; i++) {
      if (!mixer_player_group_[i]) {
        player_index_ = i;
        LOG(INFO) << "select player_index_ = " << player_index_;
        break;
      }
    }
    // Video Mixer decoder type selector
    if (mixer_decoder_hw_state_[HW_MAIN] == NOT_USED) {
      mixer_decoder_hw_state_[HW_MAIN] = IN_USED;
      mixer_decoder_hw_main_ = true;
      LOG(INFO) << "Video mixer decoder type: HW-MAIN";
    } else if (mixer_decoder_hw_state_[HW_SUB] == NOT_USED) {
      mixer_decoder_hw_state_[HW_SUB] = IN_USED;
      mixer_decoder_hw_sub_ = true;
      LOG(INFO) << "Video mixer decoder type: HW-SUB";
    } else {
      mixer_decoder_sw_ = true;
      LOG(INFO) << "Video mixer decoder type: SW";
    }
  }
#endif

#if defined(TIZEN_SOUND_FOCUS)
  // According to the API guide, below calling sequence should be kept.
  // 1. |sound_manager_create_stream_information|
  // 2. |player_create|
  // 3. |player_set_sound_stream_info|
  // To follow this, initialize |SoundFocusManager| here.
  SoundFocusManager::GetInstance();
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  if (!CreatePlayerHandleIfNeeded())
    return;
#endif

  if (manager) {
    is_getting_cookies_ = true;
    if (!GetCookiesAsync(manager->GetWebContents(player_id), url_,
                         base::Bind(&MediaPlayerBridgeCapi::HandleCookies,
                                    weak_factory_.GetWeakPtr()))) {
      is_getting_cookies_ = false;
    }
  }

#if defined(TIZEN_VIDEO_HOLE)
  if (is_video_hole_) {
    if (is_video_window()) {
      video_plane_manager_.reset(VideoPlaneManagerTizen::Create(
          MediaPlayerEfl::main_window_handle(), this));
    } else  // video windowless mode(ROI)
    {
      video_plane_manager_.reset(VideoPlaneManagerTizen::Create(
          MediaPlayerEfl::main_window_handle(), this, WINDOWLESS_MODE_NORMAL));
    }
    int ret = video_plane_manager_->Initialize();
    if (ret != PLAYER_ERROR_NONE) {
      OnHandlePlayerError(ret, FROM_HERE);
      return;
    }
  }
#endif
}

void MediaPlayerBridgeCapi::ClearDelayedPlayerStateQueue() {
  delayed_player_state_queue_.clear();
}

void MediaPlayerBridgeCapi::HandleCookies(const std::string& cookies) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&MediaPlayerBridgeCapi::HandleCookies,
                                      weak_factory_.GetWeakPtr(), cookies));
    return;
  }

  is_getting_cookies_ = false;
  cookies_ = cookies;
  if (pending_prepare_) {
    pending_prepare_ = false;
    if (IsPlayerSuspended()) {
      LOG(INFO) << "suspend when getting cookies";
      return;
    }
    Prepare(base::Bind(&MediaPlayerBridgeCapi::OnInitComplete,
                       weak_factory_.GetWeakPtr()));
  }
}

MediaPlayerBridgeCapi::~MediaPlayerBridgeCapi() {
  LOG(INFO) << "~MediaPlayerBridgeCapi, this: " << (void*)this;
  weak_factory_.InvalidateWeakPtrs();
  Release();

#if defined(OS_TIZEN_TV_PRODUCT)
  decryptor_condition_.Signal();
  player_destroy(player_);
  player_ = nullptr;
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  if (!is_video_hole_)
    ui::SetSecureModeGPU(ui::TzGPUmode::NONSECURE_MODE);
#endif
}

void MediaPlayerBridgeCapi::Prepare(CompleteCB cb) {
  LOG(INFO) << "Prepare, this: " << (void*)this
            << ", url: " << url_.spec().c_str()
            << ", mime:" << mime_type_.c_str();

  error_occured_ = false;
  complete_callback_ = cb;

  if (url_.SchemeIsFileSystem()) {
    GetPlatformPathFromURL(manager()->GetWebContents(GetPlayerId()), url_,
                           base::Bind(&MediaPlayerBridgeCapi::DoPrepare,
                                      weak_factory_.GetWeakPtr()));
    return;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  if (url_.SchemeIs("data")) {
    is_file_url_ = true;
    std::string charset;
    std::string data;

    bool ret = net::DataURL::Parse(url_, &mime_type_, &charset, &data);
    LOG(INFO) << "mime:" << mime_type_ << ",charset:" << charset
              << ",decode_data:" << data.c_str();
    if (!ret || data.empty()) {
      LOG(ERROR) << "decode data failed,ret:" << ret;
      OnHandlePlayerError(PLAYER_ERROR_INVALID_URI, FROM_HERE);
      RunCompleteCB(false, FROM_HERE);
      return;
    }
    if (data.size() > url::GetMaxURLChars()) {
      LOG(ERROR) << "data url is longer than MaxUrlSize,size:" << data.size();
      OnHandlePlayerError(PLAYER_ERROR_INVALID_URI, FROM_HERE);
      RunCompleteCB(false, FROM_HERE);
      return;
    }
    SaveDataToFile(data, base::Bind(&MediaPlayerBridgeCapi::DoPrepare,
                                    weak_factory_.GetWeakPtr()));
    return;
  }
#endif

  DoPrepare(std::string());
}

void MediaPlayerBridgeCapi::DoPrepare(const std::string& fs_path) {
  LOG(INFO) << "DoPrepare,fs_path:" << fs_path;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (!fs_path.empty() && url_.SchemeIs("data"))
    data_url_ = fs_path;

  // if state not reset,there is an error in this case:
  // 1.when resume to play,state remains the state before suspend(EnoughData)
  // 2.player_start is called and buffering=1,not report  EnoughData
  // 3.when buffering=100%, it check that status is already  EnoughData,
  //   it also not report EnoughData
  // 4. then not report EnoghData both case when resume to play
  if (content::IsHbbTV())
    player_ready_state_ = blink::WebMediaPlayer::kReadyStateHaveNothing;

  parental_rating_pass_ = true;
#endif

  int ret = PLAYER_ERROR_NONE;

  if (IsPlayerSuspended()) {
    LOG(INFO) << "suspend before prepare...";
    return;
  }

  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&MediaPlayerBridgeCapi::DoPrepare,
                                      weak_factory_.GetWeakPtr(), fs_path));
    return;
  }

#if !defined(OS_TIZEN_TV_PRODUCT)
  if (GetPlayerState() == PLAYER_STATE_NONE) {
    if (!CreatePlayerHandleIfNeeded()) {
      RunCompleteCB(false, FROM_HERE);
      return;
    }
  }
#else
  notify_playback_state_ = blink::WebMediaPlayer::kPlaybackStop;
  is_player_released_ = false;
  bool media_resource_acquired = false;
  std::string translated_url;
  std::string drm_info;

  NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackLoad, url_.spec(),
                      mime_type_, &media_resource_acquired, &translated_url,
                      &drm_info);

  LOG(INFO) << "media_resource_acquired: " << media_resource_acquired
            << ",translated_url:" << translated_url
            << ",drm_info: " << drm_info;

  if (mime_type_.find("application/vnd.oipf.contentaccessstreaming+xml") !=
      std::string::npos) {
    LOG(INFO) << "CASD url,waiting for hbbtv parse the real url and set "
                 "translated url";
    return;
  }

  if (PlaybackNotificationEnabled() && content::IsHbbTV()) {
    if (!SetDrmInfo(drm_info)) {
      LOG(ERROR) << "SetDrmInfo failed";
      return;
    }

    if (!translated_url.empty())
      url_ = media::GetCleanURL(translated_url);
  }

  if (content::IsHbbTV()) {
    hbbtv_url_ = url_.spec();
    content_id_ = url_.spec();
    clean_url_ = url_.spec();
    RemoveUrlSuffixForCleanUrl();
    GetUserPreferAudioLanguage(prefer_audio_lang_);

    if (CheckMarlinEnable()) {
      if (!GetMarlinUrl())
        return;
    }
  }
#endif

  SetVolume(volume_);

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  if (!is_video_hole_) {
    ui::SetSecureModeGPU(ui::TzGPUmode::SECURE_MODE);

    // Enhance 360 performance by no window effect.
    if (!elm_win_aux_hint_val_set(
            static_cast<Evas_Object*>(MediaPlayerEfl::main_window_handle()),
            web360_status_hint_id_, "1"))
      LOG(WARNING) << "[elm hint] Set fail : " << kWeb360PlayStateElmHint;
  }
#endif

  if (url_.SchemeIsFile())
    is_file_url_ = true;
#if defined(OS_TIZEN_TV_PRODUCT)
  // Support html5 blob url.
  else if (url_.SchemeIsBlob()) {
    std::string blob_url =
        ResolveBlobUrl(manager()->GetWebContents(GetPlayerId()), url_);
    if (!blob_url.empty()) {
      url_ = GURL(blob_url);
    }
    is_file_url_ = true;
  }

  std::string tizen_app_id =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kTizenAppId);

  if (!tizen_app_id.empty()) {
    LOG(INFO) << "appid : " << tizen_app_id;

    ret = player_set_app_id(player_, tizen_app_id.c_str());
    if (ret != PLAYER_ERROR_NONE) {
      OnHandlePlayerError(ret, FROM_HERE);
      RunCompleteCB(false, FROM_HERE);
      return;
    }
  }

  if (url_.spec().find(".ts") != std::string::npos)
    stream_type_ = TS_STREAM;
  else if (url_.spec().find(".mpd") != std::string::npos)
    stream_type_ = DASH_STREAM;
  else if (url_.spec().find(".m3u") != std::string::npos)
    stream_type_ = HLS_STREAM;
  else if (mime_type_.find("application/dash+xml") != std::string::npos) {
    if (content::IsHbbTV())
      player_set_streaming_type(player_, static_cast<char*>("DASHEX"));
    else
      player_set_streaming_type(player_, static_cast<char*>("DASH"));
    stream_type_ = DASH_STREAM;
  }
  if (content::IsHbbTV() && CheckHighBitRate() &&
      (stream_type_ == DASH_STREAM || stream_type_ == HLS_STREAM))
    AppendUrlHighBitRate(url_.spec());
#endif

#if defined(TIZEN_SOUND_FOCUS)
  ret = player_set_sound_stream_info(
      player_, SoundFocusManager::GetInstance()->GetSoundStreamInfo());
  if (ret != PLAYER_ERROR_NONE)
    LOG(ERROR) << "player_set_sound_stream_info failed " << ret;
#endif

#if defined(TIZEN_VIDEO_HOLE)
  if (is_video_hole_) {
    ret = player_set_display(
        player_, PLAYER_DISPLAY_TYPE_OVERLAY,
        GET_DISPLAY(video_plane_manager_->GetVideoPlaneHandle()));
    if (ret != PLAYER_ERROR_NONE) {
      OnHandlePlayerError(ret, FROM_HERE);
      RunCompleteCB(false, FROM_HERE);
      return;
    }

#if !defined(OS_TIZEN_TV_PRODUCT)
    ret = player_enable_media_packet_video_frame_decoded_cb(player_,
                                                            !is_fullscreen_);
    if (ret != PLAYER_ERROR_NONE) {
      OnHandlePlayerError(ret, FROM_HERE);
      return;
    }
#endif
  }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  if (content::IsHbbTV()) {
    if (IsStandaloneEme()) {
      ret = PreparePlayerForDrmEme();
      if (PLAYER_ERROR_NONE != ret) {
        OnHandlePlayerError(ret, FROM_HERE);
        RunCompleteCB(false, FROM_HERE);
        return;
      }
      player_set_uri(player_, (hbbtv_url_ + "|DRM_TYPE=DRM_TYPE_CENC").c_str());
    } else
      player_set_uri(player_, hbbtv_url_.c_str());
  } else if (url_.SchemeIs("data"))
    player_set_uri(player_, data_url_.c_str());
  else
#endif
  {
    if (url_.SchemeIsFileSystem()) {
      LOG(INFO) << "Translated Path: " << fs_path;
      is_file_url_ = true;
      player_set_uri(player_, fs_path.c_str());
    } else
      player_set_uri(player_, url_.spec().c_str());
  }

  player_set_volume(player_, static_cast<float>(volume_),
                    static_cast<float>(volume_));
  if (player_set_streaming_user_agent(player_, user_agent_.c_str(),
                                      user_agent_.length()) !=
      PLAYER_ERROR_NONE)
    LOG(ERROR) << "Unable to set streaming user agent.";

#if defined(OS_TIZEN_TV_PRODUCT)
  if (!is_video_hole_)
#endif
  {
    ret = player_set_media_packet_video_frame_decoded_cb(
        player_, MediaPacketDecodedCb, this);
    if (ret != PLAYER_ERROR_NONE) {
      OnHandlePlayerError(ret, FROM_HERE);
      RunCompleteCB(false, FROM_HERE);
      return;
    }
  }

#if defined(TIZEN_VIDEO_HOLE) && defined(TIZEN_VD_LFD_ROTATE)
  if (is_video_hole_)
    video_plane_manager_->SetDisplayRotation();
#endif

  ret = player_set_audio_latency_mode(player_, audio_latency_mode_);
  if (ret != PLAYER_ERROR_NONE)
    LOG(ERROR) << "|player_set_audio_latency_mode| failed";

  player_set_completed_cb(player_, PlaybackCompleteCb, this);
  player_set_buffering_cb(player_, ChangedBufferingStatusCb, this);
  player_set_error_cb(player_, ErrorCb, this);
  player_set_interrupted_cb(player_, InterruptedCb, this);

#if defined(OS_TIZEN_TV_PRODUCT)
  player_set_drm_error_cb(player_, DrmErrorCb, this);
  ret = player_display_video_at_paused_state(player_, false);
  if (ret != PLAYER_ERROR_NONE)
    LOG(ERROR) << "player_display_video_at_paused_state() failed";

  if (content::IsHbbTV()) {
    ret = player_set_others_event_cb(player_, OtherEventCb, this);
    if (ret != PLAYER_ERROR_NONE)
      LOG(ERROR) << "player_set_others_event_cb() failed";
  }
  ret = player_open_subtitle_cb(player_, OnSubtitleDataCallback, this);
  if (ret != PLAYER_ERROR_NONE)
    LOG(ERROR) << "player_open_subtitle_cb:"
               << GetString(static_cast<player_error_e>(ret));
#endif

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser()) {
    LOG(INFO) << "mixer mode prepare";
    if (mixer_decoder_hw_main_) {
      ret = player_video_mixer_open(player_, PLAYER_MIXER_DECODER_HW_MAIN);
      if (ret != PLAYER_ERROR_NONE) {
        LOG(ERROR)
            << "player_video_mixer_open[PLAYER_MIXER_DECODER_HW_MAIN], ret = "
            << ret;
      }
    } else if (mixer_decoder_hw_sub_) {
      ret = player_video_mixer_open(player_, PLAYER_MIXER_DECODER_HW_SUB);
      if (ret != PLAYER_ERROR_NONE)
        LOG(ERROR)
            << "player_video_mixer_open[PLAYER_MIXER_DECODER_HW_SUB], ret = "
            << ret;
    } else {
      ret = player_video_mixer_open(player_, PLAYER_MIXER_DECODER_SW);
      if (ret != PLAYER_ERROR_NONE)
        LOG(ERROR) << "player_video_mixer_open[PLAYER_MIXER_DECODER_SW], ret = "
                   << ret;
    }

    player_deactivate_stream(player_, PLAYER_STREAM_TYPE_AUDIO);

    if (is_resuming_)
      SetGeometry(video_rect_);
  }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  // HBBTV media all run in preloading mode.
  if (content::IsHbbTV())
    ret = player_preloading_async(player_, -1, PlayerPreLoadingCb, this);
  else {
    NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackReady);
#endif
    ret = player_prepare_async(player_, PlayerPreparedCb, this);
#if defined(OS_TIZEN_TV_PRODUCT)
  }
#endif

  if (ret != PLAYER_ERROR_NONE) {
    OnHandlePlayerError(ret, FROM_HERE);
    RunCompleteCB(false, FROM_HERE);
  } else {
    if (!is_resuming_) {
      manager()->OnReadyStateChange(
          GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveNothing);
    }
    manager()->OnNetworkStateChange(
        GetPlayerId(), blink::WebMediaPlayer::kNetworkStateLoading);
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  if (url_.SchemeIs("data")) {
    manager()->OnReadyStateChange(
        GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveFutureData);
    manager()->OnNetworkStateChange(GetPlayerId(),
                                    blink::WebMediaPlayer::kNetworkStateLoaded);
  }
#endif
}

#if defined(TIZEN_VIDEO_HOLE)
void MediaPlayerBridgeCapi::SetGeometry(const gfx::RectF& rect) {
  if (!is_video_hole_)
    return;
  int ret = video_plane_manager_->SetGeometry(rect);
#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser())
    video_rect_ = rect;
#endif
  if (ret != PLAYER_ERROR_NONE) {
    OnHandlePlayerError(ret, FROM_HERE);
    RunCompleteCB(false, FROM_HERE);
    return;
  }
}

void MediaPlayerBridgeCapi::OnWebViewMoved() {
  if (!is_video_hole_)
    return;
  video_plane_manager_->ResetGeometry();
}
#endif  // defined(TIZEN_VIDEO_HOLE)

void MediaPlayerBridgeCapi::OnMediaError(MediaError error_type) {
  StopBufferingUpdateTimer();
  StopCurrentTimeUpdateTimer();
#if defined(OS_TIZEN_TV_PRODUCT)
  StopSeekableTimeUpdateTimer();
#endif
  MediaPlayerEfl::OnMediaError(error_type);
}

void MediaPlayerBridgeCapi::Initialize() {
  if (is_getting_cookies_) {
    pending_prepare_ = true;
  } else {
#if defined(TIZEN_VIDEO_HOLE)
    if (is_video_hole_ && !video_plane_manager_)
      return;
#endif
    Prepare(base::Bind(&MediaPlayerBridgeCapi::OnInitComplete,
                       weak_factory_.GetWeakPtr()));
  }
}

void MediaPlayerBridgeCapi::Resume() {
  LOG(INFO) << "Resume,this: " << this
            << ",time: " << GetCurrentTime().InSecondsF();
  MediaPlayerEfl::Resume();

  is_resuming_ = true;
  Prepare(base::Bind(&MediaPlayerBridgeCapi::OnResumeComplete,
                     weak_factory_.GetWeakPtr()));
  should_seek_after_resume_ = false;
}

void MediaPlayerBridgeCapi::Suspend() {
  LOG(INFO) << "Suspend, this: " << this;
  MediaPlayerEfl::Suspend();
  is_resuming_ = false;
  is_seeking_ = false;
  stored_seek_time_during_resume_ = media::kNoTimestamp;
  seek_duration_ = base::TimeDelta();
  playback_time_ = GetCurrentTime();
  pending_playback_rate_ = playback_rate_;
  playback_rate_ = 1.0;

  Release();

  // Rare scenario: Player's resource are released while prerolling.
  // In this case we need to inform HTMLMediaElement that we can play,
  // otherwise we will never be able to play such video.
  if (!is_initialized_ && !error_occured_) {
    manager()->OnReadyStateChange(
        GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveEnoughData);
    manager()->OnNetworkStateChange(GetPlayerId(),
                                    blink::WebMediaPlayer::kNetworkStateLoaded);
    is_initialized_ = true;
  }

  is_paused_ = true;
  manager()->OnPauseStateChange(GetPlayerId(), true);
}

void MediaPlayerBridgeCapi::Release() {
  LOG(INFO) << "Release, this: " << this;
#if defined(OS_TIZEN_TV_PRODUCT)
  if (is_player_released_) {
    LOG(INFO) << "player is already released";
    return;
  }
  is_player_released_ = true;
#endif
  StopCurrentTimeUpdateTimer();
  StopBufferingUpdateTimer();
#if defined(OS_TIZEN_TV_PRODUCT)
  StopSeekableTimeUpdateTimer();
  player_prepared_ = false;
  player_started_ = false;
  if (content::IsHbbTV())
    manager()->OnPlayerStarted(GetPlayerId(), false);
#endif
  ClearDelayedPlayerStateQueue();

  if (GetPlayerState() == PLAYER_STATE_NONE) {
    LOG(INFO) << "Player[" << GetPlayerId() << "] is already released";
    manager()->OnSuspendComplete(GetPlayerId());
    return;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  if (url_.SchemeIs("data")) {
    LOG(INFO) << "remove file:" << data_url_.c_str();
    errno = 0;
    int ret = remove(data_url_.c_str());
    if (ret != 0)
      LOG(WARNING) << "remove file fail,errno:" << errno;
  }
  decryptor_condition_.Signal();
  if (IsStandaloneEme()) {
    player_set_drm_handle(player_, PLAYER_DRM_TYPE_NONE, 0);
    player_set_drm_init_data_cb(
        player_, [](drm_init_data_type, void*, int, void*) { return 0; }, this);
    player_set_drm_init_complete_cb(
        player_,
        [](int*, unsigned int, unsigned char*, void*) { return false; }, this);
  }
#endif

  player_unset_completed_cb(player_);
#if !defined(OS_TIZEN_TV_PRODUCT)
  player_unset_interrupted_cb(player_);
#endif
  player_unset_error_cb(player_);
  player_unset_buffering_cb(player_);
  player_unset_interrupted_cb(player_);

#if defined(OS_TIZEN_TV_PRODUCT)
  if (!is_video_hole_)
#endif
    player_unset_media_packet_video_frame_decoded_cb(player_);

  if (GetPlayerState() > PLAYER_STATE_READY &&
      player_stop(player_) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "|player_stop| failed";
  }

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser())
    player_video_mixer_close(player_, DEPRECATED_PARAM);
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  if (GetPlayerState() >= PLAYER_STATE_IDLE &&
      player_unprepare(player_) != PLAYER_ERROR_NONE)
#else
  if (GetPlayerState() > PLAYER_STATE_IDLE &&
      player_unprepare(player_) != PLAYER_ERROR_NONE)
#endif
    LOG(ERROR) << "|player_unprepare| failed";

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser()) {
    LOG(INFO) << "mixer mode release";
    if (player_)
      mixer_player_group_[player_index_] = 0;
    if (mixer_decoder_hw_main_)
      mixer_decoder_hw_state_[HW_MAIN] = NOT_USED;
    else if (mixer_decoder_hw_sub_)
      mixer_decoder_hw_state_[HW_SUB] = NOT_USED;

    if (mixer_player_active_audio_ == player_)
      mixer_player_active_audio_ = 0;
  }
#endif

#if defined(TIZEN_TBM_SUPPORT) && defined(OS_TIZEN_TV_PRODUCT)
  if (!is_video_hole_ &&
      !elm_win_aux_hint_val_set(
          static_cast<Evas_Object*>(MediaPlayerEfl::main_window_handle()),
          web360_status_hint_id_, "0"))
    LOG(WARNING) << "[elm hint] Unset fail : " << kWeb360PlayStateElmHint;
#endif

#if !defined(OS_TIZEN_TV_PRODUCT)
  player_destroy(player_);
  player_ = NULL;
#else
  pending_active_text_track_id_ = active_text_track_id_;
  pending_active_audio_track_id_ = active_audio_track_id_;
  pending_active_video_track_id_ = active_video_track_id_;
  NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackStop);
  NotifySubtitleState(blink::WebMediaPlayer::kSubtitleStop);
  active_audio_track_id_ = -1;
  active_video_track_id_ = -1;
#endif

  manager()->OnSuspendComplete(GetPlayerId());
}

void MediaPlayerBridgeCapi::Play() {
  LOG(INFO) << "Play,this:" << this
            << ",current_time:" << GetCurrentTime().InSecondsF();
#if defined(OS_TIZEN_TV_PRODUCT)
  // HBBTV run in preloading and preplay mode. If no prepread do prepare
  // in PlayerPrePlay(), otherwise do normal Play.
  if (content::IsHbbTV() && !player_prepared_) {
    if (!PlayerPrePlay())
      LOG(ERROR) << "HBBTV prePlay fail.";
    return;
  }
#endif

  if (is_resuming_ || is_seeking_) {
    PushDelayedPlayerState(PLAYER_STATE_DELAYED_PLAY);
    return;
  }

  if (DOUBLE_EQUAL(playback_rate_, 0.0)) {
    is_paused_ = false;
    return;
  }

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser()) {
    LOG(INFO) << "play mixer mode";
    if (mixer_player_active_audio_ != player_) {
      if (!IsMuted()) {
        if (mixer_player_active_audio_)
          player_deactivate_stream(mixer_player_active_audio_,
                                   PLAYER_STREAM_TYPE_AUDIO);
        player_activate_stream(player_, PLAYER_STREAM_TYPE_AUDIO);
        mixer_player_active_audio_ = player_;
      }
    }
  }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  if (url_.SchemeIs("data") && !player_prepared_) {
    LOG(INFO) << "player prepare not finish yet for data url,pending play";
    PushDelayedPlayerState(PLAYER_STATE_DELAYED_PLAY);
    return;
  }

  if (content::IsHbbTV() && !parental_rating_pass_) {
    LOG(INFO) << "parental rating authenticatoin is not pass yet,waiting...";
    PushDelayedPlayerState(PLAYER_STATE_DELAYED_PLAY);
    return;
  }

  // some TC check currentTime after receive "playing" event
  // but OnCurrentTimeUpdateTimerFired is slower than "playing"
  // so,notify HTMLMediaElement currentTime earlier for the first time
  if (content::IsHbbTV())
    manager()->OnTimeUpdate(GetPlayerId(), GetCurrentTime());

  if (GetPlayerState() != PLAYER_STATE_PLAYING) {
    decryptor_condition_.Signal();
    int ret = player_start(player_);
    if (ret != PLAYER_ERROR_NONE) {
      LOG(ERROR) << "|player_start| failed";
      OnHandlePlayerError(ret, FROM_HERE);
      return;
    }
  } else {
    LOG(INFO) << "player already in playing state. Ignore |player_start|";
  }
#else
  if (player_start(player_) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "|player_start| failed";
    return;
  }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  if (!DOUBLE_EQUAL(pending_playback_rate_, kInvalidPlayRate)) {
    SetRate(pending_playback_rate_);
    pending_playback_rate_ = kInvalidPlayRate;
  }

  if (pending_active_text_track_id_ != -1 && active_text_track_id_ == -1) {
    active_text_track_id_ = pending_active_text_track_id_;
    pending_active_text_track_id_ = -1;
  }

  if (pending_active_audio_track_id_ != -1 && active_audio_track_id_ == -1) {
    SetActiveAudioTrack(pending_active_audio_track_id_);
    pending_active_audio_track_id_ = -1;
  }

  if (pending_active_video_track_id_ != -1 && active_video_track_id_ == -1) {
    SetActiveVideoTrack(pending_active_video_track_id_);
    pending_active_video_track_id_ = -1;
  }

  NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackStart);
  NotifySubtitleState(blink::WebMediaPlayer::kSubtitlePlay);

  player_started_ = true;
  if (content::IsHbbTV()) {
    int is_buffering = 0;
    int ret = player_get_adaptive_streaming_info(
        player_, &is_buffering, PLAYER_ADAPTIVE_INFO_STREAMING_IS_BUFFERING);

    if (ret != PLAYER_ERROR_NONE)
      LOG(WARNING) << "player_get_adaptive_streaming_info failed";

    if (is_buffering && !is_file_url_)
      manager()->OnNetworkStateChange(
          GetPlayerId(), blink::WebMediaPlayer::kNetworkStateLoading);
    else if (player_ready_state_ <
             blink::WebMediaPlayer::kReadyStateHaveEnoughData) {
      manager()->OnReadyStateChange(
          GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveEnoughData);
      manager()->OnNetworkStateChange(
          GetPlayerId(), blink::WebMediaPlayer::kNetworkStateLoaded);
      player_ready_state_ = blink::WebMediaPlayer::kReadyStateHaveEnoughData;
      LOG(INFO) << "report EnoughData when player_started";
      manager()->OnPlayerStarted(GetPlayerId(), true);
    }
  }
#endif

  if (GetMediaType() & MediaType::Video)
    WakeUpDisplayAndAcquireDisplayLock();

  StartCurrentTimeUpdateTimer();
  if (!is_file_url_)
    StartBufferingUpdateTimer();

  is_paused_ = false;
  is_end_reached_ = false;
  is_media_related_action_ = false;
  should_seek_after_resume_ = true;
}

void MediaPlayerBridgeCapi::Pause(bool is_media_related_action) {
  LOG(INFO) << "is_media_related_action : " << is_media_related_action
            << ",this:" << this
            << ",current_time:" << GetCurrentTime().InSecondsF();
  if (is_resuming_ || is_seeking_) {
    PushDelayedPlayerState(PLAYER_STATE_DELAYED_PAUSE);
    return;
  }

  if (GetPlayerState() != PLAYER_STATE_PLAYING) {
    is_paused_ = true;
    return;
  }

  if (player_pause(player_) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "|player_pause| failed";
    return;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  NotifySubtitleState(blink::WebMediaPlayer::kSubtitlePause);
#endif

  if (!is_file_url_)
    StartBufferingUpdateTimer();

  if (GetMediaType() & MediaType::Video)
    ReleaseDisplayLock();
  StopCurrentTimeUpdateTimer();
  is_paused_ = true;
}

void MediaPlayerBridgeCapi::SetRate(double rate) {
  if (DOUBLE_EQUAL(playback_rate_, rate))
    return;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (GetPlayerState() != PLAYER_STATE_PLAYING &&
      !DOUBLE_EQUAL(playback_rate_, 0.0)) {
    pending_playback_rate_ = rate;
    return;
  }
#else
  if (GetPlayerState() < PLAYER_STATE_READY) {
    pending_playback_rate_ = rate;
    return;
  }
#endif

  bool need_play = DOUBLE_EQUAL(playback_rate_, 0);
  playback_rate_ = rate;

  // No operation is performed, if rate is 0
  if (DOUBLE_EQUAL(rate, 0.0)) {
    Pause(true);
    return;
  }

  if (is_file_url_) {
    int err = player_set_playback_rate(player_, static_cast<float>(rate));
    if (err != PLAYER_ERROR_NONE) {
      LOG(ERROR) << "|player_set_playback_rate| failed";
      return;
    }
  } else {
    int err =
        player_set_streaming_playback_rate(player_, static_cast<float>(rate));
    if (err != PLAYER_ERROR_NONE) {
      LOG(ERROR) << "|player_set_streaming_playback_rate| failed";
      return;
    }
  }

  if (need_play)
    Play();
}

void MediaPlayerBridgeCapi::Seek(base::TimeDelta time) {
  LOG(INFO) << " time : " << time;
  if (GetPlayerState() < PLAYER_STATE_READY) {
    if (time == playback_time_) {
      stored_seek_time_during_resume_ = time;
      manager()->OnSeekComplete(GetPlayerId());
    } else {
      stored_seek_time_during_resume_ = media::kNoTimestamp;
      playback_time_ = time;
      PushDelayedPlayerState(PLAYER_STATE_DELAYED_SEEK);
      pending_seek_duration_ = playback_time_;
      manager()->OnTimeUpdate(GetPlayerId(), playback_time_);
#if defined(OS_TIZEN_TV_PRODUCT)
      UpdateCurrentTime(playback_time_);
#endif
      manager()->OnTimeChanged(GetPlayerId());
    }

    return;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  if (!is_player_seek_available_) {
    SeekCompleteUpdate();
    LOG(ERROR) << "|player seek is not available";
    return;
  }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  NotifySubtitleState(blink::WebMediaPlayer::kSubtitleSeekStart,
                      time.InSecondsF());
#endif

  base::TimeDelta player_seek_time = time;
  const auto err = PlayerSetPlayPosition(player_, player_seek_time, true,
                                         SeekCompletedCb, this);

  if (err != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "|player_set_play_position| failed";
    OnHandlePlayerError(err, FROM_HERE);
    const auto current_time = GetCurrentTime();
    manager()->OnTimeUpdate(GetPlayerId(), current_time);
#if defined(OS_TIZEN_TV_PRODUCT)
    UpdateCurrentTime(current_time);
#endif
    manager()->OnSeekComplete(GetPlayerId());
    return;
  }

  if (!is_paused_)
    StopCurrentTimeUpdateTimer();

  StopBufferingUpdateTimer();
  UpdateSeekState(true);
  seek_duration_ = time;
  is_end_reached_ = time != duration_ ? false : true;
  manager()->OnTimeUpdate(GetPlayerId(), time);
#if defined(OS_TIZEN_TV_PRODUCT)
  UpdateCurrentTime(time);
#endif

  if (!is_paused_)
    StartCurrentTimeUpdateTimer();
}

void MediaPlayerBridgeCapi::SetVolume(double volume) {
  volume_ = volume;
  if (GetPlayerState() == PLAYER_STATE_NONE)
    return;

  if (player_set_volume(player_, volume, volume) != PLAYER_ERROR_NONE)
    LOG(ERROR) << "|player_set_volume| failed";
}

void MediaPlayerBridgeCapi::UpdateMediaType() {
  int sample_rate = 0;
  int channel = 0;
  int audio_bit_rate = 0;

  int err = player_get_audio_stream_info(player_, &sample_rate, &channel,
                                         &audio_bit_rate);
  if (err != PLAYER_ERROR_NONE) {
    OnHandlePlayerError(err, FROM_HERE);
    RunCompleteCB(false, FROM_HERE);
    return;
  }

#if defined(TIZEN_VIDEO_HOLE)
  if (is_video_hole_) {
    int width = 0;
    int height = 0;
    err = player_get_video_size(player_, &width, &height);

    if (err != PLAYER_ERROR_NONE) {
      OnHandlePlayerError(err, FROM_HERE);
      RunCompleteCB(false, FROM_HERE);
      return;
    }

    width_ = width;
    height_ = height;

    // without Media Packet.
    if (width_ > 0 && height_ > 0)
      SetMediaType(MediaType::Video);
  }
#endif
  // Audio stream is present if sample rate is valid.
  if (sample_rate > 0)
    SetMediaType(MediaType::Audio);

#if defined(OS_TIZEN_TV_PRODUCT)
  int textCnt = 0;
  err = player_get_adaptationset_count(player_, PLAYER_STREAM_TYPE_TEXT,
                                       &textCnt);
  if (err == PLAYER_ERROR_NONE && textCnt > 0)
    SetMediaType(MediaType::Text);
#endif

  manager()->OnMediaDataChange(GetPlayerId(), width_, height_, GetMediaType());

#if defined(OS_TIZEN_TV_PRODUCT)
  if (!is_buffering_compeleted_) {
#endif
    manager()->OnReadyStateChange(
        GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveMetadata);
#if defined(OS_TIZEN_TV_PRODUCT)
    player_ready_state_ = blink::WebMediaPlayer::kReadyStateHaveMetadata;
  }
#endif
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool MediaPlayerBridgeCapi::CheckLiveStreaming() const {
  int isLive = 0;
  int retVal = player_get_adaptive_streaming_info(player_, &isLive,
                                                  PLAYER_ADAPTIVE_INFO_IS_LIVE);
  if (retVal != PLAYER_ERROR_NONE || !isLive)
    return false;
  LOG(INFO) << "This is a live streaming.";
  return true;
}

std::string MediaPlayerBridgeCapi::ParseDashKeyword(
    const std::string& keyword) {
  char* dash_info = nullptr;
  // dash_info format:
  // {"type":0,"minBufferTime":4000,"mrsUrl":"","periodId":"first"}
  const int ret = player_get_dash_info(player_, &dash_info);
  if (ret != PLAYER_ERROR_NONE) {
    if (dash_info)
      free(dash_info);
    LOG(ERROR) << "Fail to call player_get_dash_info,ret:" << ret;
    return "";
  }

  if (!dash_info) {
    LOG(ERROR) << "empty dash_info.";
    return "";
  }

  const std::string info(dash_info);
  free(dash_info);
  LOG(INFO) << "dash info str: " << info;

  const std::string::size_type delimiter = info.find(keyword);
  if (delimiter == std::string::npos) {
    LOG(ERROR) << "Fail to find keyword: " << keyword;
    return "";
  }

  std::string::size_type begin = info.find(':', delimiter);
  std::string::size_type end = info.find_first_of(",}", begin);

  if (begin != std::string::npos && end != std::string::npos) {
    std::string value = info.substr(begin + 1, end - begin - 1);
    // num-value has no "",but string-value has "",remove "" for string
    begin = 0;
    end = value.length() - 1;
    if (value[0] == '"') {
      begin = begin + 1;
      if (value[end] == '"')
        end = end - 1;
      value = value.substr(begin, end - begin + 1);
    }

    LOG(INFO) << "keyword: " << keyword << ",value:" << value;
    return value;
  }
  return "";
}

void MediaPlayerBridgeCapi::ParseDashInfo() {
  mrs_url_ = ParseDashKeyword("mrsUrl");
  std::string periodId_ = ParseDashKeyword("periodId");
  if (periodId_ != "")
    content_id_ = clean_url_ + "#period=" + periodId_;
  LOG(INFO) << "mrsUrl: " << mrs_url_ << ",contentId: " << content_id_;
}

bool MediaPlayerBridgeCapi::GetDashLiveDuration(int64_t* duration) {
  DCHECK(duration);
  std::string dur = ParseDashKeyword("mediaPresentationDuration");
  if (dur.empty()) {
    LOG(ERROR) << "Fail to get duration.";
    return false;
  }

  // According to dash spec, default_value: -1.
  int64_t out_duration = -1;
  if (!base::StringToInt64(dur, &out_duration)) {
    LOG(ERROR) << "Fail to get duration: out_duration get failed";
    return false;
  }
  // According to dash spec, if duration == -1, set max time as duration.
  if (out_duration == -1)
    out_duration = base::TimeDelta::Max().InMilliseconds();
  *duration = out_duration;

  return true;
}

bool MediaPlayerBridgeCapi::GetLiveStreamingDuration(int64_t* min,
                                                     int64_t* max) const {
  DCHECK(min);
  DCHECK(max);
  constexpr const size_t kDurationBufferSize = 64;
  char liveDuration[kDurationBufferSize] = {
      0,
  };
  char* livePtr = liveDuration;
  const auto retVal = player_get_adaptive_streaming_info(
      player_, &livePtr, PLAYER_ADAPTIVE_INFO_LIVE_DURATION);
  if (retVal != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "Fail to get duration!!,retVal:" << retVal;
    return false;
  }

  const std::string duration(liveDuration);
  const std::string::size_type delimiter = duration.find('|');

  if (delimiter == std::string::npos) {
    LOG(ERROR) << "Fail to find delimiter | in duration: " << duration;
    return false;
  }

  const std::string duration_min = duration.substr(0, delimiter);
  if (duration_min.empty()) {
    LOG(ERROR) << "Fail empty min sub str: " << duration << ", " << delimiter;
    return false;
  }

  int64 out_min = 0;
  if (!base::StringToInt64(duration_min, &out_min)) {
    LOG(ERROR) << "Fail to get min from duration: " << duration;
    return false;
  }

  const std::string duration_max = duration.substr(delimiter + 1);
  if (duration_max.empty()) {
    LOG(ERROR) << "Fail empty max sub str: " << duration << ", " << delimiter;
    return false;
  }

  int64 out_max = 0;
  if (!base::StringToInt64(duration_max, &out_max)) {
    LOG(ERROR) << "Fail to get max from duration: " << duration;
    return false;
  }

  *min = out_min;
  *max = out_max;
  return true;
}

void MediaPlayerBridgeCapi::UpdateSeekableTime() {
  if (GetPlayerState() == PLAYER_STATE_NONE)
    return;

  int64 min = 0;
  int64 max = 0;

  if (GetLiveStreamingDuration(&min, &max)) {
    if (min_seekable_time_.InMilliseconds() != min ||
        max_seekable_time_.InMilliseconds() != max) {
      min_seekable_time_ = base::TimeDelta::FromMilliseconds(min);
      max_seekable_time_ = base::TimeDelta::FromMilliseconds(max);
      manager()->OnSeekableTimeChange(GetPlayerId(), min_seekable_time_,
                                      max_seekable_time_);
    }
  }
}

void MediaPlayerBridgeCapi::StartSeekableTimeUpdateTimer() {
  const int kSeekableTimeUpdateInterval = 500;
  if (!seekable_time_update_timer_.IsRunning()) {
    seekable_time_update_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(kSeekableTimeUpdateInterval), this,
        &MediaPlayerBridgeCapi::OnSeekableTimeUpdateTimerFired);
  }
}

void MediaPlayerBridgeCapi::StopSeekableTimeUpdateTimer() {
  if (seekable_time_update_timer_.IsRunning())
    seekable_time_update_timer_.Stop();
}

void MediaPlayerBridgeCapi::OnSeekableTimeUpdateTimerFired() {
  UpdateSeekableTime();
}

bool MediaPlayerBridgeCapi::SetDrmInfo(std::string& drm_info) {
  // The DRM info from HbbTV comes as a JSON-like string in the following
  // format:
  // pair = Property ':' Value
  // string = pair {',' pair}
  std::vector<std::string> drm_info_pairs = base::SplitStringUsingSubstr(
      drm_info, ", ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (auto it = drm_info_pairs.begin(); it != drm_info_pairs.end(); ++it) {
    std::vector<std::string> drm_info_pair = base::SplitStringUsingSubstr(
        *it, ": ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    if (drm_info_pair.size() != 2)
      continue;

    std::string trim_key = drm_info_pair.at(0);
    std::string trim_value = drm_info_pair.at(1);

    LOG(INFO) << "trim_key: " << trim_key.c_str()
              << ",trim_value: " << trim_value.c_str();

    if (!SetMediaDRMInfo(trim_key, trim_value))
      return false;
  }

  return true;
}

bool MediaPlayerBridgeCapi::SetMediaDRMInfo(const std::string& type_data,
                                            const std::string& value_data) {
  LOG(INFO) << "player_set_drm_info(type_length(" << type_data.length() << ") "
            << " value_length(" << value_data.length() << ")) ";
  uses_hbbtv_drm_ = true;
  int ret = PLAYER_ERROR_INVALID_OPERATION;
  const void* type_data_ptr = type_data.c_str();
  const void* value_data_ptr = value_data.c_str();
  player_drm_type_e drm_type = PLAYER_DRM_TYPE_PLAYREADY;
  if (CheckMarlinEnable()) {
    drm_type = PLAYER_DRM_TYPE_MARLIN;
    std::string property_type = "PROPERTY_MARLIN_LICENSE_URL";
    ret = player_set_drm_info(player_, property_type.c_str(),
                              property_type.length(), url_.spec().c_str(),
                              url_.spec().length(), drm_type);
  } else {
    ret = player_set_drm_info(player_, type_data_ptr, type_data.length(),
                              value_data_ptr, value_data.length(), drm_type);
  }
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "SetMediaDRMInfo failed";
    OnHandlePlayerError(ret, FROM_HERE);
    RunCompleteCB(false, FROM_HERE);
    return false;
  }

  return true;
}

void MediaPlayerBridgeCapi::HandleMrsUrlChange(const std::string& mrsUrl) {
  mrs_url_ = mrsUrl;
  manager()->OnMrsUrlChange(GetPlayerId(), mrs_url_);
}

void MediaPlayerBridgeCapi::HandlePeriodIdChange(const std::string& periodId) {
  if (!periodId.empty()) {
    content_id_ = clean_url_ + "#period=" + periodId;
    manager()->OnContentIdChange(GetPlayerId(), content_id_);
  }
}

void MediaPlayerBridgeCapi::HandleParentalRatingInfo(const std::string& info,
                                                     const std::string& url) {
  content::WebContentsDelegate* web_contents_delegate =
      GetWebContentsDelegate();
  if (!web_contents_delegate)
    return;
  web_contents_delegate->NotifyParentalRatingInfo(info, url);
}

void MediaPlayerBridgeCapi::OnOtherEvent(int event_type, void* event_data) {
  if (!event_data) {
    LOG(ERROR) << "event_data is null.";
    return;
  }
  switch (event_type) {
    case PLAYER_MSG_STREAM_EVENT_TYPE: {
      LOG(INFO) << "PLAYER_MSG_STREAM_EVENT_TYPE";
      EventStream* eventstream = static_cast<EventStream*>(event_data);
      if (!eventstream) {
        LOG(ERROR) << "eventstream is null.";
        return;
      }
      std::string uri(eventstream->schemeIdUri ? eventstream->schemeIdUri : "");
      std::string value(eventstream->value ? eventstream->value : "");
      player_event_mode type = eventstream->type;
      LOG(INFO) << "uri:" << uri << ",value:" << value << ",type:" << type;

      // parental rating event only need to handle data info
      if (type == PLAYER_DVB_EVENT_MODE_PARENTAL_GUIDANCE) {
        LOG(INFO) << "parental rating event, skip type";
        return;
      }

      if (!uri.compare("urn:mpeg:dash:event:2012") ||
          !uri.compare("urn:dvb:iptv:cpm:2014")) {
        LOG(INFO) << "not supported scheme id uri,uri:" << uri;
        return;
      }

      const std::string info = uri + " " + value;
      task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&MediaPlayerBridgeCapi::HandleInbandTextTrack,
                     weak_factory_.GetWeakPtr(), info, ADD_INBAND_TEXT_TRACK));
      break;
    }
    case PLAYER_MSG_STREAM_EVENT_DATA: {
      LOG(INFO) << "PLAYER_MSG_STREAM_EVENT_DATA";
      Event* event = static_cast<Event*>(event_data);
      if (!event) {
        LOG(ERROR) << "event is null.";
        return;
      }
      std::string uri(event->schemeIdUri ? event->schemeIdUri : "");
      std::string value(event->value ? event->value : "");
      unsigned int id = event->id;
      long long int start_time = event->startTimeMs;
      long long int end_time = event->endTimeMs;
      std::string data(event->data ? event->data : "");
      player_event_mode type = event->type;
      LOG(INFO) << "uri:" << uri << ",value:" << value << ",data:" << data
                << ",id:" << id << ",start_time:" << start_time
                << ",end_time:" << end_time << ",type:" << type;

      if (type == PLAYER_DVB_EVENT_MODE_PARENTAL_GUIDANCE) {
        //<mpeg7:ParentalRating href="urn:dvb:iptv:rating:2014:15"/>
        std::size_t pos = data.find("ParentalRating href=");
        if (pos != std::string::npos) {
          parental_rating_pass_ = false;
          std::size_t quotation1 = data.find('"', pos);
          std::size_t quotation2 = data.find('"', quotation1 + 1);
          std::string target =
              data.substr(quotation1 + 1, quotation2 - quotation1 - 1);
          LOG(INFO) << "parentl rating info string is:" << target;
          task_runner_->PostTask(
              FROM_HERE,
              base::Bind(&MediaPlayerBridgeCapi::HandleParentalRatingInfo,
                         weak_factory_.GetWeakPtr(), target, url_.spec()));
        }
      } else {
        if (0 == uri.compare("urn:mpeg:dash:event:2012") ||
            0 == uri.compare("urn:dvb:iptv:cpm:2014")) {
          LOG(INFO) << "not supported scheme id uri,uri:" << uri;
          return;
        }
        const std::string info = uri + " " + value + " " + data;
        task_runner_->PostTask(
            FROM_HERE, base::Bind(&MediaPlayerBridgeCapi::HandleInbandTextCue,
                                  weak_factory_.GetWeakPtr(), info, id,
                                  start_time, end_time));
      }
      break;
    }
    case PLAYER_MSG_STREAM_EVENT_MRS_URL_CHANGED: {
      LOG(INFO) << "PLAYER_MSG_STREAM_EVENT_MRS_URL_CHANGED";
      char* url = static_cast<char*>(event_data);
      std::string mrsUrl(url ? url : "");
      LOG(INFO) << "mrsUrl:" << mrsUrl;
      task_runner_->PostTask(
          FROM_HERE, base::Bind(&MediaPlayerBridgeCapi::HandleMrsUrlChange,
                                weak_factory_.GetWeakPtr(), mrsUrl));
      break;
    }
    case PLAYER_MSG_STREAM_EVENT_PERIOAD_ID_CHANGED: {
      LOG(INFO) << "PLAYER_MSG_STREAM_EVENT_PERIOAD_ID_CHANGED";
      char* id = static_cast<char*>(event_data);
      std::string periodId(id ? id : "");
      LOG(INFO) << "periodId:" << periodId;
      task_runner_->PostTask(
          FROM_HERE, base::Bind(&MediaPlayerBridgeCapi::HandlePeriodIdChange,
                                weak_factory_.GetWeakPtr(), periodId));
      break;
    }
    case PLAYER_MSG_STREAM_EVENT_REMOVE_TRACK: {
      LOG(INFO) << "PLAYER_MSG_STREAM_EVENT_REMOVE_TRACK";
      EventStream* eventstream = static_cast<EventStream*>(event_data);
      if (!eventstream) {
        LOG(ERROR) << "eventstream is null.";
        return;
      }
      std::string uri(eventstream->schemeIdUri ? eventstream->schemeIdUri : "");
      std::string value(eventstream->value ? eventstream->value : "");
      LOG(INFO) << "uri:" << uri << ",value:" << value;

      const std::string info = uri + " " + value;
      task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&MediaPlayerBridgeCapi::HandleInbandTextTrack,
                     weak_factory_.GetWeakPtr(), info, DEL_INBAND_TEXT_TRACK));
      break;
    }
    default:
      LOG(INFO) << "unknow event type:" << event_type;
      break;
  }
}

bool MediaPlayerBridgeCapi::RegisterTimeline(
    const std::string& timeline_selector) {
  LOG(INFO) << "RegisterTimeline:" << timeline_selector.c_str();
  int ret = player_subscribe_timeline(player_, timeline_selector.c_str(),
                                      RegisterTimelineCb, this);
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_subscribe_timeline fail,ret:" << ret;
    return false;
  }
  return true;
}

bool MediaPlayerBridgeCapi::UnRegisterTimeline(
    const std::string& timeline_selector) {
  LOG(INFO) << "UnRegisterTimeline:" << timeline_selector.c_str();
  int ret = player_unsubscribe_timeline(player_, timeline_selector.c_str());
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_unsubscribe_timeline fail,ret:" << ret;
    return false;
  }
  return true;
}

void MediaPlayerBridgeCapi::GetTimelinePositions(
    const std::string& timeline_selector,
    uint32_t* units_per_tick,
    uint32_t* units_per_second,
    int64_t* content_time,
    bool* paused) {
  LOG(INFO) << "GetTimelinePositions,timeline: " << timeline_selector.c_str();
  DCHECK(units_per_tick);
  DCHECK(units_per_second);
  DCHECK(content_time);
  DCHECK(paused);
  int ret = player_get_timeline_play_position(
      player_, timeline_selector.c_str(), units_per_tick, units_per_second,
      content_time, paused);
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_get_timeline_play_position fail,ret:" << ret;
    return;
  }

  LOG(INFO) << "units_per_tick: " << *units_per_tick
            << ",units_per_second: " << *units_per_second
            << ",content_time: " << *content_time << ",paused: " << *paused;
}

double MediaPlayerBridgeCapi::GetSpeed() {
  LOG(INFO) << "MediaSync GetSpeed";
  if (GetPlayerState() < PLAYER_STATE_READY) {
    LOG(ERROR) << "state < PLAYER_STATE_READY";
    return 0.0;
  }
  double speed = 0.0;
  if (player_get_playback_rate(player_, &speed) != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_get_playback_rate fail:";
    return 0.0;
  }

  LOG(INFO) << "get speed: " << speed;
  return speed;
}

std::string MediaPlayerBridgeCapi::GetMrsUrl() {
  return mrs_url_;
}

std::string MediaPlayerBridgeCapi::GetContentId() {
  return content_id_;
}

bool MediaPlayerBridgeCapi::SyncTimeline(const std::string& timeline_selector,
                                         int64_t timeline_pos,
                                         int64_t wallclock_pos,
                                         int tolerance) {
  LOG(INFO) << "SyncTimeline,timeline:" << timeline_selector.c_str()
            << ",timeline_pos:" << timeline_pos
            << ",wallclock_pos:" << wallclock_pos << ",tolerance:" << tolerance;
  int ret =
      player_sync_timeline(player_, timeline_selector.c_str(), timeline_pos,
                           wallclock_pos, tolerance, SyncTimelineCb, this);
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_sync_timeline fail,ret:" << ret;
    return false;
  }
  return true;
}

bool MediaPlayerBridgeCapi::SetWallClock(const std::string& wallclock_url) {
  LOG(INFO) << "SetWallClock,url:" << wallclock_url.c_str();
  int ret = player_sync_set_wallclock(player_, wallclock_url.c_str());
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_sync_set_wallclock fail,ret:" << ret;
    return false;
  }
  return true;
}

void MediaPlayerBridgeCapi::HandleRegisterTimeline(
    const char* timeline_selector,
    uint32_t units_per_tick,
    uint32_t units_per_second,
    int64_t content_time,
    ETimelineState timeline_state) {
  LOG(INFO) << "HandleRegisterTimeline timeline_selector:" << timeline_selector
            << ",units_per_tick:" << units_per_tick
            << ",units_per_second:" << units_per_second
            << ",content_time:" << content_time
            << ",timeline_state:" << timeline_state;
  std::string timeline(timeline_selector ? timeline_selector : "");

  blink::WebMediaPlayer::register_timeline_cb_info_s info;
  info.timeline_selector = timeline;
  info.units_per_tick = units_per_tick;
  info.units_per_second = units_per_second;
  info.content_time = content_time;
  info.timeline_state = timeline_state;
  manager()->OnRegisterTimelineCbInfo(GetPlayerId(), info);
}

void MediaPlayerBridgeCapi::HandleSyncTimeline(const char* timeline_selector,
                                               int sync) {
  LOG(INFO) << "HandleSyncTimeline timeline_selector:" << timeline_selector
            << ",sync:" << sync;
  std::string timeline(timeline_selector ? timeline_selector : "");
  manager()->OnSyncTimelineCbInfo(GetPlayerId(), timeline, sync);
}

void MediaPlayerBridgeCapi::OnRegisterTimeline(const char* timeline_selector,
                                               uint32_t units_per_tick,
                                               uint32_t units_per_second,
                                               int64_t content_time,
                                               ETimelineState timeline_state) {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlayerBridgeCapi::HandleRegisterTimeline,
                 weak_factory_.GetWeakPtr(), timeline_selector, units_per_tick,
                 units_per_second, content_time, timeline_state));
}

void MediaPlayerBridgeCapi::OnSyncTimeline(const char* timeline_selector,
                                           int sync) {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlayerBridgeCapi::HandleSyncTimeline,
                 weak_factory_.GetWeakPtr(), timeline_selector, sync));
}

/*
 * http://xx.mpd#period=bar&t=10
 * http://xx.mpd#period=bar
 * http://xx.mpd#t=10
 */
void MediaPlayerBridgeCapi::RemoveUrlSuffixForCleanUrl() {
  std::size_t pos = clean_url_.find("&t=");
  if (pos != std::string::npos)
    clean_url_ = clean_url_.substr(0, pos);
  else if ((pos = clean_url_.find("#t=")) != std::string::npos)
    clean_url_ = clean_url_.substr(0, pos);

  pos = clean_url_.find("#period=");
  if (pos != std::string::npos)
    clean_url_ = clean_url_.substr(0, pos);
}

void MediaPlayerBridgeCapi::AppendUrlHighBitRate(const std::string& url) {
  // don't use url.append("|STARTBITRATE=HIGHEST") to get hbbtv url
  // "|" will be replaced with "%7C"
  int len = url.length() + strlen("|STARTBITRATE=HIGHEST") + 1;
  char* str_url_ = (char*)malloc(len);
  if (!str_url_) {
    LOG(ERROR) << "malloc fail";
    RunCompleteCB(false, FROM_HERE);
    return;
  }

  memset(str_url_, 0, len);
  strncat(str_url_, url.c_str(), url.length());
  strncat(str_url_, "|STARTBITRATE=HIGHEST", strlen("|STARTBITRATE=HIGHEST"));
  hbbtv_url_ = str_url_;
  BLINKFREE(str_url_);
  LOG(INFO) << "hbbtv url:" << hbbtv_url_.c_str();
}

bool MediaPlayerBridgeCapi::GetMarlinUrl() {
  std::string compound_url = url_.spec();
  int marlin_handle = -1;
  LOG(INFO) << "compound url:" << compound_url.c_str();
  if (!(compound_url.find("ms3://") != std::string::npos &&
        compound_url.find("#") != std::string::npos)) {
    LOG(ERROR) << "invalid compound url";
    OnHandlePlayerError(PLAYER_ERROR_DRM_INFO, FROM_HERE);
    RunCompleteCB(false, FROM_HERE);
    return false;
  }
  marlin_handle =
      marlin_ms3_open_ex(compound_url.c_str(), MSD_OPEN_NONE, MSD_NO_DLA, NULL);
  if (marlin_handle == MSD_COULD_NOT_GET_HANDLE || marlin_handle == -1) {
    LOG(ERROR) << "|marlin_ms3_open_ex| failed,handle:" << marlin_handle;
    OnHandlePlayerError(PLAYER_ERROR_DRM_INFO, FROM_HERE);
    RunCompleteCB(false, FROM_HERE);
    return false;
  }

  char* content_uri = NULL;
  unsigned int content_uri_len = 0;
  MSD_RET ret = marlin_ms3_getright(marlin_handle, compound_url.c_str(),
                                    compound_url.size(), MS3_COMPOUNDURL,
                                    &content_uri, &content_uri_len);
  if (ret != MSD_SUCCESS) {
    LOG(ERROR) << "|marlin_ms3_getright| failed,ret:" << ret;
    if (ret == MSD_ERROR_CONNECTIONFAIL)
      OnHandlePlayerError(PLAYER_ERROR_CONNECTION_FAILED, FROM_HERE);
    else if (ret == MSD_ERROR_NO_RIGHTS)
      OnHandlePlayerError(PLAYER_ERROR_DRM_INFO, FROM_HERE);
    if (marlin_handle != MSD_COULD_NOT_GET_HANDLE && marlin_handle != -1)
      marlin_ms3_close(marlin_handle);
    RunCompleteCB(false, FROM_HERE);
    return false;
  }
  LOG(INFO) << "marlin content uri:" << content_uri;

  std::string marlin_url(content_uri ? content_uri : "");
  BLINKFREE(content_uri);
  url_ = media::GetCleanURL(marlin_url);

  if (marlin_handle != MSD_COULD_NOT_GET_HANDLE && marlin_handle != -1)
    marlin_ms3_close(marlin_handle);
  return true;
}

void MediaPlayerBridgeCapi::ElementRemove() {
  LOG(INFO) << "ElementRemove";
  NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackStop);
}

void MediaPlayerBridgeCapi::SetParentalRatingResult(bool is_pass) {
  LOG(INFO) << "ParentalRatingResult:" << is_pass;
  parental_rating_pass_ = is_pass;

  // if authentication fail, raise MEDIA_ERROR_DECODE
  if (!parental_rating_pass_) {
    OnHandlePlayerError(PLAYER_ERROR_INVALID_OPERATION, FROM_HERE);
    return;
  }

  if (player_prepared_) {
    LOG(INFO) << "player already prepared,execute play";
    ExecuteDelayedPlayerState();
  }
}
#endif

void MediaPlayerBridgeCapi::UpdateDuration() {
  player_state_e compare_state = PLAYER_STATE_IDLE;
#if defined(OS_TIZEN_TV_PRODUCT)
  if (content::IsHbbTV())
    compare_state = PLAYER_STATE_NONE;
#endif

  if (GetPlayerState() <= compare_state)
    return;

  // Change to use 64 bit, for live stream duration may exceed 32 bit.
  int64 duration = 0;

#if defined(OS_TIZEN_TV_PRODUCT)
  if (is_live_stream_) {
    if (stream_type_ == HLS_STREAM) {
      int64 min = 0;
      if (!GetLiveStreamingDuration(&min, &duration)) {
        LOG(ERROR) << "Fail to get duration for hls live stream.";
        return;
      }
    } else if (stream_type_ == DASH_STREAM) {
      if (!GetDashLiveDuration(&duration)) {
        if (duration_ != base::TimeDelta::Max()) {
          duration_ = base::TimeDelta::Max();
          manager()->OnDurationChange(GetPlayerId(), duration_);
        }
        return;
      }
    } else {
      LOG(ERROR) << "Unknow live stream type : " << stream_type_;
    }
  } else
#endif
  {
    // For player_get_duration() api defined 32 bit common_duration for not live
    // stream video.
    int common_duration = 0;
    int ret = player_get_duration(player_, &common_duration);
    if (ret != PLAYER_ERROR_NONE) {
      OnHandlePlayerError(ret, FROM_HERE);
      RunCompleteCB(false, FROM_HERE);
      return;
    }
    duration = common_duration;
  }

  if (duration_.InMilliseconds() != duration) {
    duration_ = base::TimeDelta::FromMilliseconds(duration);
    manager()->OnDurationChange(GetPlayerId(), duration_);
  }
}

base::TimeDelta MediaPlayerBridgeCapi::GetCurrentTime() {
  if (is_seeking_) {
    LOG(INFO) << "seeking, current_time:" << seek_duration_.InSecondsF();
    return seek_duration_;
  }

  if (is_resuming_ &&
      (stored_seek_time_during_resume_ != media::kNoTimestamp)) {
    LOG(INFO) << "resuming,current_time:"
              << stored_seek_time_during_resume_.InSecondsF();
    return stored_seek_time_during_resume_;
  }
  // For http://instagram.com/p/tMQOo0lWqm/
  // After playback completed current-time and duration are not equal.
  if (is_end_reached_) {
    if (playback_rate_ < 0.0)
      return base::TimeDelta();
    if (!duration_.is_zero())
      return duration_;
  }

  if (!pending_seek_duration_.is_zero())
    return pending_seek_duration_;

  base::TimeDelta postion;
  PlayerGetPlayPosition(player_, &postion);

  return postion;
}

double MediaPlayerBridgeCapi::GetStartDate() {
#if defined(OS_TIZEN_TV_PRODUCT)
  int64_t millisecond = 0;
  int ret = player_get_start_date(player_, &millisecond);
  if (ret != PLAYER_ERROR_NONE) {
    OnHandlePlayerError(ret, FROM_HERE);
    LOG(ERROR) << "Failed to get start date from player!";
    return std::numeric_limits<double>::quiet_NaN();
  }

  return static_cast<double>(millisecond) / 1000.0;
#else
  return std::numeric_limits<double>::quiet_NaN();
#endif
}

bool MediaPlayerBridgeCapi::CreatePlayerHandleIfNeeded() {
  if (player_)
    return true;

  int ret = player_create(&player_);
#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser())
    mixer_player_group_[player_index_] = player_;
#endif

  if (ret != PLAYER_ERROR_NONE) {
    OnHandlePlayerError(ret, FROM_HERE);
    return false;
  }

  return true;
}

void MediaPlayerBridgeCapi::OnCurrentTimeUpdateTimerFired() {
  const auto current_time = GetCurrentTime();
  manager()->OnTimeUpdate(GetPlayerId(), current_time);
#if defined(OS_TIZEN_TV_PRODUCT)
  UpdateCurrentTime(current_time);
#endif
  UpdateDuration();
}

void MediaPlayerBridgeCapi::StartCurrentTimeUpdateTimer() {
  if (!current_time_update_timer_.IsRunning()) {
    current_time_update_timer_.Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(kDurationUpdateInterval),
        this, &MediaPlayerBridgeCapi::OnCurrentTimeUpdateTimerFired);
  }
}

void MediaPlayerBridgeCapi::StopCurrentTimeUpdateTimer() {
  if (current_time_update_timer_.IsRunning())
    current_time_update_timer_.Stop();
}

void MediaPlayerBridgeCapi::OnBufferingUpdateTimerFired() {
  if (GetPlayerState() <= PLAYER_STATE_IDLE)
    return;

  int start = 0, current = 0;
  if (player_get_streaming_download_progress(player_, &start, &current) !=
      PLAYER_ERROR_NONE) {
    LOG(INFO) << "|player_get_streaming_download_progress| failed";

    // TODO b.chechlinsk: FIXME Calling GetCurrentTime() when player is resuming
    // blocks muse-server receiving thread and decyrptor cannot be passed to
    // drm_eme.
    if (GetPlayerState() <= PLAYER_STATE_IDLE) {
      current = current_progress_;
    } else if (!duration_.is_zero()) {
      current =
          (GetCurrentTime().InMillisecondsF() / duration_.InMillisecondsF()) *
          100;
      current = std::max(current + 1, current_progress_);
    }
  }

  if (current >= 100) {
    current = 100;
    StopBufferingUpdateTimer();
    manager()->OnNetworkStateChange(GetPlayerId(),
                                    blink::WebMediaPlayer::kNetworkStateLoaded);
  }
  manager()->OnBufferUpdate(GetPlayerId(), current);
  current_progress_ = current;
}

void MediaPlayerBridgeCapi::StartBufferingUpdateTimer() {
  if (!buffering_update_timer_.IsRunning()) {
    buffering_update_timer_.Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(kDurationUpdateInterval),
        this, &MediaPlayerBridgeCapi::OnBufferingUpdateTimerFired);
  }
}

void MediaPlayerBridgeCapi::StopBufferingUpdateTimer() {
  if (buffering_update_timer_.IsRunning())
    buffering_update_timer_.Stop();
}

void MediaPlayerBridgeCapi::PlaybackCompleteUpdate() {
  LOG(INFO) << "PlaybackCompleteUpdate, this:" << this;
  is_end_reached_ = true;

  StopCurrentTimeUpdateTimer();
  const auto current_time = GetCurrentTime();
  manager()->OnTimeUpdate(GetPlayerId(), current_time);
#if defined(OS_TIZEN_TV_PRODUCT)
  UpdateCurrentTime(current_time);
#endif
  manager()->OnTimeChanged(GetPlayerId());
#if defined(OS_TIZEN_TV_PRODUCT)
  NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackFinish);
  NotifySubtitleState(blink::WebMediaPlayer::kSubtitleStop);
#endif
}

void MediaPlayerBridgeCapi::SeekCompleteUpdate() {
  LOG(INFO) << "SeekCompleteUpdate, this:" << this;
  UpdateSeekState(false);
  manager()->OnSeekComplete(GetPlayerId());

  if (!is_file_url_)
    StartBufferingUpdateTimer();

#if defined(OS_TIZEN_TV_PRODUCT)
  NotifySubtitleState(blink::WebMediaPlayer::kSubtitleSeekComplete);

  if (is_media_related_action_)
    Play();
  // BBC app is checking for stream end by comparing current play time with
  // stream duration, if end of stream callback is received first, playback time
  // will not be updated because of logic in
  // MediaPlayerBridgeCapi::GetCurrentTime().
  if (is_end_reached_) {
    const auto current_time = GetCurrentTime();
    manager()->OnTimeUpdate(GetPlayerId(), current_time);
    UpdateCurrentTime(current_time);
    manager()->OnTimeChanged(GetPlayerId());
  }
  if (is_media_related_action_)
    Play();
#endif
  ExecuteDelayedPlayerState();
}

void MediaPlayerBridgeCapi::UpdateSeekState(bool state) {
  is_seeking_ = state;
}

void MediaPlayerBridgeCapi::PlayerPrepared() {
  LOG(INFO) << "PlayerPrepared,this: " << this;

  if (GetPlayerState() < PLAYER_STATE_READY) {
    return;
  }
#if defined(TIZEN_VIDEO_HOLE)
  if (is_video_hole_)
    video_plane_manager_->ApplyDeferredVideoRectIfNeeded();
#endif

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser()) {
    LOG(INFO) << "mixer mode set position";
    player_video_mixer_set_position_of_mixedframe(player_, DEPRECATED_PARAM, 0,
                                                  0, MIXER_SCREEN_WIDTH,
                                                  MIXER_SCREEN_HEIGHT);
    player_display_rotation_e screen_degree = ConvertToPlayerDisplayRotation(
        display::Screen::GetScreen()->GetPrimaryDisplay().RotationAsDegree());
    LOG(INFO) << "video rotate angle : " << screen_degree;
    int ret = PLAYER_ERROR_NONE;
    if (screen_degree == PLAYER_DISPLAY_ROTATION_90 ||
        screen_degree == PLAYER_DISPLAY_ROTATION_270) {
      ret = player_video_mixer_set_resolution_of_mixedframe(
          player_, DEPRECATED_PARAM, MIXER_SCREEN_HEIGHT, MIXER_SCREEN_WIDTH);
    } else {
      ret = player_video_mixer_set_resolution_of_mixedframe(
          player_, DEPRECATED_PARAM, MIXER_SCREEN_WIDTH, MIXER_SCREEN_HEIGHT);
    }
    if (ret != PLAYER_ERROR_NONE) {
      LOG(INFO) << "player_video_mixer_set_resolution_of_mixedframe, ret = "
                << ret;
    }
  }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  player_prepared_ = true;
  is_live_stream_ = CheckLiveStreaming();

  if (!content::IsHbbTV()) {
    if (is_live_stream_ && stream_type_ != OTHER_STREAM) {
      UpdateSeekableTime();
      StartSeekableTimeUpdateTimer();
    }
    UpdateAudioTrackInfo();
    UpdateVideoTrackInfo();
    UpdateTextTrackInfo();
    UpdateDuration();
    UpdateMediaType();
  }

  int canSeek = 0;
  int retVal = player_seek_available(player_, &canSeek);
  if (retVal == PLAYER_ERROR_NONE && !canSeek)
    is_player_seek_available_ = false;
#else

  UpdateDuration();
  UpdateMediaType();
#endif

  if (GetMediaType() == 0) {
    RunCompleteCB(true, FROM_HERE);
    return;
  }
  // player_get_streaming_download_progress can be called only if player
  // state in PLAYING or PAUSED. So buffer update/loading in progress is not
  // updated for intital bufering stage. Call OnBufferUpdate explicity with
  // (0,0) to update loading in progess for progessive streaming clips.
  // In case of local file no need to buffer. So, update with (0, 100).
  if (!is_file_url_)
    manager()->OnBufferUpdate(GetPlayerId(), 0);
  else
    manager()->OnBufferUpdate(GetPlayerId(), 100);

#if !defined(OS_TIZEN_TV_PRODUCT)
  if (pending_playback_rate_ != kInvalidPlayRate) {
    SetRate(pending_playback_rate_);
    pending_playback_rate_ = kInvalidPlayRate;
  }
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
  // buffering status perent=100% maybe before PlayerPrepared
  if (is_buffering_compeleted_ && content::IsHbbTV())
    UpdatePreferAudio();

  if (!content::IsHbbTV() || GetMediaType() == MediaType::Text) {
    if (!is_buffering_compeleted_ && !is_file_url_)
      manager()->OnNetworkStateChange(
          GetPlayerId(), blink::WebMediaPlayer::kNetworkStateLoading);
    else if (player_ready_state_ <
             blink::WebMediaPlayer::kReadyStateHaveEnoughData) {
      manager()->OnReadyStateChange(
          GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveEnoughData);
      manager()->OnNetworkStateChange(
          GetPlayerId(), blink::WebMediaPlayer::kNetworkStateLoaded);
      player_ready_state_ = blink::WebMediaPlayer::kReadyStateHaveEnoughData;
      LOG(INFO) << "report EnoughDate when playerPrepared";
    }
  }
#else
  manager()->OnReadyStateChange(
      GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveEnoughData);
  manager()->OnNetworkStateChange(GetPlayerId(),
                                  blink::WebMediaPlayer::kNetworkStateLoaded);
#endif

  ExecuteDelayedPlayerState();
  RunCompleteCB(true, FROM_HERE);
}

#if defined(OS_TIZEN_TV_PRODUCT)
void MediaPlayerBridgeCapi::HandleBufferingStatusOnTV(int percent) {
  if (percent == 100) {
    is_buffering_compeleted_ = true;
    need_report_buffering_state_ = true;
  } else if (is_buffering_compeleted_)
    is_buffering_compeleted_ = false;

  if (!player_prepared_)
    return;

  if (percent == 100) {
    if (content::IsHbbTV()) {
      UpdatePreferAudio();
      if (!player_started_)
        return;
    }

    if (player_ready_state_ == blink::WebMediaPlayer::kReadyStateHaveEnoughData)
      return;
    manager()->OnNetworkStateChange(GetPlayerId(),
                                    blink::WebMediaPlayer::kNetworkStateLoaded);
    player_ready_state_ = blink::WebMediaPlayer::kReadyStateHaveEnoughData;
    if (!is_file_url_)
      StartBufferingUpdateTimer();

    manager()->OnReadyStateChange(
        GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveEnoughData);
    LOG(INFO) << "report EnoughData when buffering=100";
    if (content::IsHbbTV())
      manager()->OnPlayerStarted(GetPlayerId(), true);
  } else {
    if (!need_report_buffering_state_)
      return;
    need_report_buffering_state_ = false;
    LOG(INFO) << "percent = " << percent
              << " %, player_ready_state_ = " << player_ready_state_;
    if (player_ready_state_ >=
        blink::WebMediaPlayer::kReadyStateHaveCurrentData) {
      LOG(INFO) << "No need change ready state";
      return;
    }
    player_ready_state_ = blink::WebMediaPlayer::kReadyStateHaveCurrentData;

    manager()->OnReadyStateChange(
        GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveCurrentData);
    manager()->OnNetworkStateChange(
        GetPlayerId(), blink::WebMediaPlayer::kNetworkStateLoading);
  }
}
#endif

void MediaPlayerBridgeCapi::HandleBufferingStatus(int percent) {
  if (is_paused_ || is_seeking_)
    return;

  if (percent == 100) {
    if (GetPlayerState() != PLAYER_STATE_PAUSED)
      return;
    if (player_start(player_) != PLAYER_ERROR_NONE) {
      LOG(ERROR) << "|player_start| failed";
      return;
    }
    StartCurrentTimeUpdateTimer();

    if (!is_file_url_)
      StartBufferingUpdateTimer();

    manager()->OnReadyStateChange(
        GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveEnoughData);
  } else {
    if (GetPlayerState() != PLAYER_STATE_PLAYING)
      return;
    if (player_pause(player_) != PLAYER_ERROR_NONE) {
      LOG(ERROR) << "|player_pause| failed";
      return;
    }

    StopCurrentTimeUpdateTimer();
    manager()->OnReadyStateChange(
        GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveCurrentData);
  }
  manager()->OnNetworkStateChange(GetPlayerId(),
                                  blink::WebMediaPlayer::kNetworkStateLoading);
}

void MediaPlayerBridgeCapi::RunCompleteCB(bool success,
                                          const base::Location& from) {
  if (!success)
    LOG(ERROR) << __FUNCTION__ << " is called from : " << from.ToString();

  if (!complete_callback_.is_null()) {
    complete_callback_.Run(success);
    complete_callback_.Reset();
  }
}

player_state_e MediaPlayerBridgeCapi::GetPlayerState() {
  player_state_e state = PLAYER_STATE_NONE;
  player_get_state(player_, &state);
  return state;
}

void MediaPlayerBridgeCapi::PushDelayedPlayerState(int new_state) {
  // Calling deque front() for empty container causes undefined behaviour.
  // So, check for size() before calling the front().
  if (delayed_player_state_queue_.size() == 0) {
    delayed_player_state_queue_.push_front(new_state);
    return;
  }

  switch (new_state) {
    case PLAYER_STATE_DELAYED_SEEK:
      // Do not queue another seek else push seek to front
      if (delayed_player_state_queue_.front() != new_state)
        delayed_player_state_queue_.push_front(new_state);
      break;
    case PLAYER_STATE_DELAYED_PAUSE:
    case PLAYER_STATE_DELAYED_PLAY:
      // remove all pause/play commands
      delayed_player_state_queue_.erase(
          std::remove_if(delayed_player_state_queue_.begin(),
                         delayed_player_state_queue_.end(),
                         [](int state) {
                           return (state == PLAYER_STATE_DELAYED_PLAY) ||
                                  (state == PLAYER_STATE_DELAYED_PAUSE);
                         }),
          delayed_player_state_queue_.end());

      delayed_player_state_queue_.push_back(new_state);
      break;
    default:
      NOTREACHED();
      break;
  }
}

void MediaPlayerBridgeCapi::ExecuteDelayedPlayerState() {
  if (delayed_player_state_queue_.empty()) {
    LOG(INFO) << "delayed_player_state_queue_ is empty";
    return;
  }
  const auto delay_state = delayed_player_state_queue_.front();
  delayed_player_state_queue_.pop_front();
  LOG(INFO) << "delay_state: " << delay_state << ",(1:play 2:pause 3:seek)";
  switch (delay_state) {
    case PLAYER_STATE_DELAYED_PLAY:
      Play();
      break;
    case PLAYER_STATE_DELAYED_PAUSE:
      Pause(false);
      break;
    case PLAYER_STATE_DELAYED_SEEK:
      Seek(pending_seek_duration_);
      pending_seek_duration_ = base::TimeDelta();
      break;
    default:
      break;
  }
}

void MediaPlayerBridgeCapi::OnMediaPacketUpdated(media_packet_h packet) {
  // Discard frames during resume sequence
  if (is_resuming_ && (is_fullscreen_ && is_video_hole_)) {
    media_packet_destroy(packet);
  } else {
#if defined(USE_DIRECT_CALL_MEDIA_PACKET_THREAD)
    DeliverMediaPacket(ScopedMediaPacket(packet));
#else
    ScopedMediaPacket packet_proxy(packet);
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&MediaPlayerBridgeCapi::DeliverMediaPacket,
                   weak_factory_.GetWeakPtr(), base::Passed(&packet_proxy)));
#endif
  }
}

#if !defined(OS_TIZEN_TV_PRODUCT)
void MediaPlayerBridgeCapi::OnInterrupted(player_interrupted_code_e code) {
  // If invoked from other threads, relay to current task_runner.
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&MediaPlayerBridgeCapi::OnInterrupted,
                                      weak_factory_.GetWeakPtr(), code));
    return;
  }

  LOG(INFO) << "Resource Conflict : player[" << GetPlayerId() << "]";
  Suspend();
}

#else

void MediaPlayerBridgeCapi::CheckHLSSupport(const std::string& url,
                                            bool* support) {
  *support = player_is_supportable_hlsurl(url.c_str());
}

void MediaPlayerBridgeCapi::SetHasEncryptedListenerOrCdm(bool value) {
  LOG(INFO) << "Has encrypted listener or CDM: " << value;
  has_encrypted_listener_or_cdm_ = value;
}

void MediaPlayerBridgeCapi::OnPlayerPreloading() {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&MediaPlayerBridgeCapi::PlayerPreloaded,
                                    weak_factory_.GetWeakPtr()));
}

bool MediaPlayerBridgeCapi::HBBTVResourceAcquired() {
  bool mediaResourceAcquired = false;
  NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackReady, url_.spec(),
                      mime_type_, &mediaResourceAcquired, NULL, NULL);
  if (!mediaResourceAcquired) {
    NotifyPlaybackState(blink::WebMediaPlayer::kPlaybackStop);
    LOG(ERROR) << "HBBTV media resource acquired failed";
  }

  return mediaResourceAcquired;
}

void MediaPlayerBridgeCapi::PlayerPreloaded() {
  LOG(INFO) << "PlayerPreloaded,this: " << this
            << ",is_resuming: " << is_resuming_
            << ",player_prepared_:" << player_prepared_;

  is_live_stream_ = CheckLiveStreaming();
  if (stream_type_ == DASH_STREAM)
    ParseDashInfo();
  if (is_live_stream_ && stream_type_ != OTHER_STREAM) {
    UpdateSeekableTime();
    StartSeekableTimeUpdateTimer();
  }
  UpdateAudioTrackInfo();
  UpdateVideoTrackInfo();
  UpdateTextTrackInfo();
  UpdateDuration();
  UpdateMediaType();
  if (GetMediaType() == 0) {
    RunCompleteCB(true, FROM_HERE);
    return;
  }

  manager()->OnReadyStateChange(
      GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveFutureData);
  manager()->OnNetworkStateChange(GetPlayerId(),
                                  blink::WebMediaPlayer::kNetworkStateLoading);

  if (is_resuming_ && !player_prepared_ && HBBTVResourceAcquired()) {
    int ret = player_prepare_async(player_, PlayerPreparedCb, this);
    if (ret != PLAYER_ERROR_NONE) {
      OnHandlePlayerError(ret, FROM_HERE);
      RunCompleteCB(false, FROM_HERE);
    }
  }
}

bool MediaPlayerBridgeCapi::PlayerPrePlay() {
  LOG(INFO) << "PlayerPrePlay, this: " << this
            << ",is_player_prepared : " << player_prepared_;
  if (IsPlayerSuspended())
    return false;

  if (!HBBTVResourceAcquired())
    return false;

  PushDelayedPlayerState(PLAYER_STATE_DELAYED_PLAY);
  LOG(INFO) << "begin to |player_prepare_async|";
  int ret = player_prepare_async(player_, PlayerPreparedCb, this);
  if (ret != PLAYER_ERROR_NONE) {
    OnHandlePlayerError(ret, FROM_HERE);
    RunCompleteCB(false, FROM_HERE);
    return false;
  }

  return true;
}

void MediaPlayerBridgeCapi::OnResourceConflict() {
  LOG(INFO) << "Resource Conflict occurred";
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&MediaPlayerEfl::OnPlayerResourceConflict,
                                    weak_factory_.GetWeakPtr()));
}

void MediaPlayerBridgeCapi::HandleDrmError(int err_code) {
  LOG(ERROR) << "DrmError: "
             << GetString(static_cast<player_error_e>(err_code));
  manager()->OnReadyStateChange(
      GetPlayerId(), blink::WebMediaPlayer::kReadyStateHaveEnoughData);
  manager()->OnNetworkStateChange(GetPlayerId(),
                                  blink::WebMediaPlayer::kNetworkStateLoaded);
  manager()->OnDrmError(GetPlayerId());
  OnMediaError(GetMediaError(err_code));
}

void MediaPlayerBridgeCapi::OnDrmError(int err_code, char* err_str) {
  LOG(ERROR) << "OnDrmError err_str[" << err_str << "]";
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&MediaPlayerBridgeCapi::HandleDrmError,
                                    weak_factory_.GetWeakPtr(), err_code));
}

void MediaPlayerBridgeCapi::HandleInbandTextTrack(const std::string& info,
                                                  int action) {
  manager()->OnInbandEventTextTrack(GetPlayerId(), info, action);
}

void MediaPlayerBridgeCapi::HandleInbandTextCue(const std::string& info,
                                                unsigned int id,
                                                long long int start_time,
                                                long long int end_time) {
  manager()->OnInbandEventTextCue(GetPlayerId(), info, id, start_time,
                                  end_time);
}

std::string MediaPlayerBridgeCapi::MapDashMediaTrackKind(
    const std::string& role,
    const int size,
    bool isAudio) {
  // HBBTV Spec : A.2.12.3 Modifications to clause 8.4.6
  // HBBTV spec v2.0.2 seperate audioTrack and videoTrack kind definition
  // and changed the "main" definiton of audioTrack(page 258)
  if (role.find("alternate") != std::string::npos &&
      role.find("main") == std::string::npos &&
      role.find("commentary") == std::string::npos &&
      role.find("dub") == std::string::npos)
    return "alternative";

  if (role.find("caption") != std::string::npos &&
      role.find("main") != std::string::npos)
    return "captions";

  if (role.find("description") != std::string::npos &&
      role.find("supplementary") != std::string::npos)
    return "descriptions";

  if (role.find("main") != std::string::npos &&
      role.find("caption") == std::string::npos &&
      role.find("subtitle") == std::string::npos &&
      role.find("dub") == std::string::npos) {
    if (!isAudio)
      return "main";
    else if (role.find("description") == std::string::npos)
      return "main";
  }

  if (role.find("main") != std::string::npos &&
      role.find("description") != std::string::npos)
    return "main-desc";

  if (role.find("subtitle") != std::string::npos &&
      role.find("main") != std::string::npos)
    return "subtitles";

  if (role.find("dub") != std::string::npos &&
      role.find("main") != std::string::npos)
    return "translation";

  if (role.find("commentary") != std::string::npos &&
      role.find("main") == std::string::npos)
    return "commentary";

  return "";
}

void MediaPlayerBridgeCapi::UpdateAudioTrackInfo() {
  int cntTracks = 0;
  int err = player_get_adaptationset_count(player_, PLAYER_STREAM_TYPE_AUDIO,
                                           &cntTracks);
  if (err != PLAYER_ERROR_NONE || cntTracks == 0) {
    LOG(ERROR) << "get audio track fail,err:" << err << ",count:" << cntTracks;
    return;
  }

  LOG(INFO) << "audio track count: " << cntTracks;

  player_audio_adaptationset_info* audio_track_info =
      static_cast<player_audio_adaptationset_info*>(
          malloc(cntTracks * sizeof(player_audio_adaptationset_info)));
  if (!audio_track_info) {
    LOG(ERROR) << "malloc fail";
    return;
  }
  memset(audio_track_info, 0,
         cntTracks * sizeof(player_audio_adaptationset_info));

  for (int i = 0; i < cntTracks; i++) {
    int audio_alter_count = 0;
    err = player_get_alternative_count(player_, PLAYER_STREAM_TYPE_AUDIO, i,
                                       &audio_alter_count);
    if (err != PLAYER_ERROR_NONE || audio_alter_count == 0) {
      LOG(WARNING) << "player_get_alternative_count error,idx: " << i
                   << ",audio_alter_count: " << audio_alter_count;
      continue;
    }

    LOG(INFO) << "audio track idx:" << i
              << ",alter_count:" << audio_alter_count;
    audio_track_info[i].alternatives =
        static_cast<Alternative_audioStreamInfo*>(
            malloc(audio_alter_count * sizeof(Alternative_audioStreamInfo)));
    if (!audio_track_info[i].alternatives) {
      LOG(ERROR) << "malloc fail,free memory which is already malloced";
      if (i >= 1) {
        for (int j = 0; j <= i - 1; j++)
          BLINKFREE(audio_track_info[j].alternatives);
      }
      BLINKFREE(audio_track_info);
      return;
    }
    memset(audio_track_info[i].alternatives, 0,
           audio_alter_count * sizeof(Alternative_audioStreamInfo));
  }

  err = player_get_audio_adaptationset_info(player_, audio_track_info);
  if (err != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "|player_get_audio_adaptationset_info| failed";
    for (int i = 0; i < cntTracks; i++)
      BLINKFREE(audio_track_info[i].alternatives);
    BLINKFREE(audio_track_info);
    return;
  }

  int selected_idx = 0;
  for (int i = 0; i < cntTracks; i++) {
    if (!audio_track_info[i].alternatives) {
      LOG(WARNING) << "track[" << i << "] fail,ignore it.";
      continue;
    }

    const char* language = audio_track_info[i].language;
    if (prefer_audio_lang_.find(language) != std::string::npos) {
      if (stream_type_ == DASH_STREAM)
        prefer_audio_adaptionset_idx_ = audio_track_info[i].index;
      else
        prefer_audio_adaptionset_idx_ = i;

      selected_idx = i;
    }
  }
  LOG(INFO) << "prefer_audio_adaptationset_idx_: "
            << prefer_audio_adaptionset_idx_
            << ",selected_idx:" << selected_idx;

  for (int i = 0; i < cntTracks; i++) {
    if (!audio_track_info[i].alternatives) {
      LOG(WARNING) << "track[" << i << "] fail,ignore it.";
      continue;
    }
    const char* lang = audio_track_info[i].language;
    std::string langStr(lang ? lang : "");

    std::stringstream roleInfoBuilder;
    const int role_count = audio_track_info[i].role_count;

    for (int k = 0; k < role_count; k++) {
      const char* role_member = audio_track_info[i].role_value[k];
      const std::string role_str(role_member ? role_member : "");
      roleInfoBuilder << role_str << ",";
    }

    LOG(INFO) << "role str:" << roleInfoBuilder.str()
              << ",role count:" << role_count;
    std::string kindStr = "";
    if (stream_type_ == DASH_STREAM)
      kindStr = MapDashMediaTrackKind(roleInfoBuilder.str(), role_count, true);
    if (stream_type_ == TS_STREAM)  // role_count = 1
      kindStr = audio_track_info[i].role_value[0];

    char* label = nullptr;
    err = player_get_content_info(player_, PLAYER_CONTENT_INFO_TITLE, &label);
    if (err != PLAYER_ERROR_NONE)
      LOG(WARNING) << "|player_get_content_info| failed";
    std::string labelStr(label ? label : "");
    BLINKFREE(label);

    int id = 0;
    if (stream_type_ == DASH_STREAM)
      id = audio_track_info[i].adaptationset_id;
    else
      id = audio_track_info[i].alternatives[0].track_id;

    BLINKFREE(audio_track_info[i].alternatives);

    LOG(INFO) << "AudioTrack Info - id: " << id << ", kind: " << kindStr.c_str()
              << ", label: " << labelStr.c_str()
              << ", lang: " << langStr.c_str()
              << ", index: " << audio_track_info[i].index;

    blink::WebMediaPlayer::audio_video_track_info_s info;
    info.id = std::to_string(id);
    info.kind = kindStr;
    info.label = labelStr;
    info.language = langStr;
    info.enabled = (selected_idx == i);
    manager()->OnAddAudioTrack(GetPlayerId(), info);
  }

  BLINKFREE(audio_track_info);
}

void MediaPlayerBridgeCapi::UpdateVideoTrackInfo() {
  int cntTracks = 0;
  int err = player_get_adaptationset_count(player_, PLAYER_STREAM_TYPE_VIDEO,
                                           &cntTracks);
  if (err != PLAYER_ERROR_NONE || cntTracks == 0) {
    LOG(ERROR) << "get video track fail,err:" << err << ",count" << cntTracks;
    return;
  }

  LOG(INFO) << "video track count: " << cntTracks;

  player_video_adaptationset_info* video_track_info =
      static_cast<player_video_adaptationset_info*>(
          malloc(cntTracks * sizeof(player_video_adaptationset_info)));
  if (!video_track_info) {
    LOG(ERROR) << "malloc fail";
    return;
  }
  memset(video_track_info, 0,
         cntTracks * sizeof(player_video_adaptationset_info));

  int adaptionSetIdx = -1;
  int alternativeIdx = -1;
  err = player_get_current_track_ex(player_, PLAYER_STREAM_TYPE_VIDEO,
                                    &adaptionSetIdx, &alternativeIdx);
  if (err != PLAYER_ERROR_NONE)
    LOG(WARNING) << "|player_get_current_track| failed";
  LOG(INFO) << "video adaptionSetIdx:" << adaptionSetIdx
            << ",alternativeIdx:" << alternativeIdx;

  for (int i = 0; i < cntTracks; i++) {
    int video_alter_count = 0;
    err = player_get_alternative_count(player_, PLAYER_STREAM_TYPE_VIDEO, i,
                                       &video_alter_count);
    if (err != PLAYER_ERROR_NONE || video_alter_count == 0) {
      LOG(WARNING) << "player_get_alternative_count error,idx: " << i
                   << ",video_alter_count: " << video_alter_count;
      continue;
    }

    LOG(INFO) << "video track idx:" << i
              << ",alter_count:" << video_alter_count;
    video_track_info[i].alternatives =
        static_cast<Alternative_videoStreamInfo*>(
            malloc(video_alter_count * sizeof(Alternative_videoStreamInfo)));
    if (!video_track_info[i].alternatives) {
      LOG(ERROR) << "malloc fail,free memory which is already malloced";
      if (i >= 1) {
        for (int j = 0; j <= i - 1; j++)
          BLINKFREE(video_track_info[j].alternatives);
      }
      BLINKFREE(video_track_info);
      return;
    }
    memset(video_track_info[i].alternatives, 0,
           video_alter_count * sizeof(Alternative_videoStreamInfo));
  }

  err = player_get_video_adaptationset_info(player_, video_track_info);
  if (err != PLAYER_ERROR_NONE) {
    LOG(WARNING) << "|player_get_video_adaptationset_info| failed";
    for (int i = 0; i < cntTracks; i++)
      BLINKFREE(video_track_info[i].alternatives);
    BLINKFREE(video_track_info);
    return;
  }

  for (int i = 0; i < cntTracks; i++) {
    if (!video_track_info[i].alternatives) {
      LOG(WARNING) << "track[" << i << "] fail,ignore it.";
      continue;
    }

    char* lang = nullptr;
    err = player_get_track_language_code_ex(player_, PLAYER_STREAM_TYPE_VIDEO,
                                            i, &lang);
    if (err != PLAYER_ERROR_NONE)
      LOG(WARNING) << "|player_get_track_language_code| failed";
    std::string langStr(lang ? lang : "");
    BLINKFREE(lang);

    std::stringstream roleInfoBuilder;
    const int role_count = video_track_info[i].role_count;

    for (int k = 0; k < role_count; k++) {
      const char* role_member = video_track_info[i].role_value[k];
      const std::string role_str(role_member ? role_member : "");
      roleInfoBuilder << role_str << ",";
    }

    LOG(INFO) << "role str:" << roleInfoBuilder.str()
              << ",role count:" << role_count;

    std::string kindStr = "";
    if (stream_type_ == DASH_STREAM)
      kindStr = MapDashMediaTrackKind(roleInfoBuilder.str(), role_count, false);

    char* label = nullptr;
    err = player_get_content_info(player_, PLAYER_CONTENT_INFO_TITLE, &label);
    if (err != PLAYER_ERROR_NONE)
      LOG(WARNING) << "|player_get_content_info| failed";
    std::string labelStr(label ? label : "");
    BLINKFREE(label);

    int id = 0;
    if (stream_type_ == DASH_STREAM)
      id = video_track_info[i].adaptationset_id;
    else
      id = video_track_info[i].alternatives[0].track_id;

    if (adaptionSetIdx == i)
      LOG(INFO) << "VideoTrack index: [" << i << "] is selected";

    BLINKFREE(video_track_info[i].alternatives);

    LOG(INFO) << "VideoTrack Info - id: " << id << ", kind: " << kindStr.c_str()
              << ", label: " << labelStr.c_str()
              << ", lang: " << langStr.c_str();

    blink::WebMediaPlayer::audio_video_track_info_s info;
    info.id = std::to_string(id);
    info.kind = kindStr;
    info.label = labelStr;
    info.language = langStr;
    info.enabled = (adaptionSetIdx == i);
    manager()->OnAddVideoTrack(GetPlayerId(), info);
  }
  BLINKFREE(video_track_info);
}

void MediaPlayerBridgeCapi::UpdateTextTrackInfo() {
  int cntTracks = 0;
  int err = player_get_adaptationset_count(player_, PLAYER_STREAM_TYPE_TEXT,
                                           &cntTracks);
  if (err != PLAYER_ERROR_NONE || cntTracks == 0) {
    LOG(ERROR) << "get text track fail,err:" << err << ",count:" << cntTracks;
    return;
  }

  LOG(INFO) << "text track count: " << cntTracks;

  player_subtitle_adaptationset_info* text_track_info =
      static_cast<player_subtitle_adaptationset_info*>(
          malloc(cntTracks * sizeof(player_subtitle_adaptationset_info)));
  if (!text_track_info) {
    LOG(ERROR) << "malloc fail";
    return;
  }
  memset(text_track_info, 0,
         cntTracks * sizeof(player_subtitle_adaptationset_info));

  int adaptionSetIdx = -1;
  int alternativeIdx = -1;
  err = player_get_current_track_ex(player_, PLAYER_STREAM_TYPE_TEXT,
                                    &adaptionSetIdx, &alternativeIdx);
  if (err != PLAYER_ERROR_NONE)
    LOG(WARNING) << "|player_get_current_track| failed";
  LOG(INFO) << "text adaptionSetIdx:" << adaptionSetIdx
            << ",alternativeIdx:" << alternativeIdx;

  for (int i = 0; i < cntTracks; i++) {
    int text_alter_count = 0;
    err = player_get_alternative_count(player_, PLAYER_STREAM_TYPE_TEXT, i,
                                       &text_alter_count);
    if (err != PLAYER_ERROR_NONE || text_alter_count == 0) {
      LOG(WARNING) << "player_get_alternative_count error,idx: " << i
                   << ",text_alter_count: " << text_alter_count;
      continue;
    }

    LOG(INFO) << "text track idx:" << i << ",alter_count:" << text_alter_count;
    text_track_info[i].alternatives =
        static_cast<Alternative_subtitleStreamInfo*>(
            malloc(text_alter_count * sizeof(Alternative_subtitleStreamInfo)));
    if (!text_track_info[i].alternatives) {
      LOG(ERROR) << "malloc fail,free memory which is already malloced";
      if (i >= 1) {
        for (int j = 0; j <= i - 1; j++)
          BLINKFREE(text_track_info[j].alternatives);
      }
      BLINKFREE(text_track_info);
      return;
    }
    memset(text_track_info[i].alternatives, 0,
           text_alter_count * sizeof(Alternative_subtitleStreamInfo));
  }

  err = player_get_subtitle_adaptationset_info(player_, text_track_info);
  if (err != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "|player_get_subtitle_adaptationset_info| failed";
    for (int i = 0; i < cntTracks; i++)
      BLINKFREE(text_track_info[i].alternatives);
    BLINKFREE(text_track_info);
    return;
  }

  for (int i = 0; i < cntTracks; i++) {
    if (!text_track_info[i].alternatives) {
      LOG(WARNING) << "track[" << i << "] fail,ignore it.";
      continue;
    }

    char* lang = nullptr;
    err = player_get_track_language_code_ex(player_, PLAYER_STREAM_TYPE_TEXT, i,
                                            &lang);
    if (err != PLAYER_ERROR_NONE)
      LOG(WARNING) << "|player_get_track_language_code| failed";
    std::string langStr(lang ? lang : "");
    BLINKFREE(lang);

    char* label = nullptr;
    err = player_get_content_info(player_, PLAYER_CONTENT_INFO_TITLE, &label);
    if (err != PLAYER_ERROR_NONE)
      LOG(WARNING) << "|player_get_content_info| failed";
    std::string labelStr(label ? label : "");
    BLINKFREE(label);

    int id = text_track_info[i].adaptationset_id;

    if (adaptionSetIdx == i)
      LOG(INFO) << "TextTrack index: [" << i << "] is selected";

    BLINKFREE(text_track_info[i].alternatives);

    LOG(INFO) << "TextTrack Info - id: " << id
              << ", label: " << labelStr.c_str()
              << ", lang: " << langStr.c_str();
    manager()->OnAddTextTrack(GetPlayerId(), labelStr, langStr,
                              std::to_string(id));
  }

  BLINKFREE(text_track_info);
}

void MediaPlayerBridgeCapi::UpdatePreferAudio() {
  LOG(INFO) << "UpdatePreferAudio,idx: " << prefer_audio_adaptionset_idx_
            << ",last idx:" << last_prefer_audio_adaptionset_idx_;
  if (prefer_audio_adaptionset_idx_ == last_prefer_audio_adaptionset_idx_) {
    LOG(WARNING) << "same track,no need to select";
    return;
  }
  int ret = player_select_track_ex(player_, PLAYER_STREAM_TYPE_AUDIO,
                                   prefer_audio_adaptionset_idx_, -1);
  if (ret != PLAYER_ERROR_NONE) {
    LOG(ERROR) << "player_select_track_ex fail";
    return;
  }
  last_prefer_audio_adaptionset_idx_ = prefer_audio_adaptionset_idx_;
}

bool MediaPlayerBridgeCapi::GetUserPreferAudioLanguage(
    std::string& audio_prefer_lang) {
  audio_prefer_lang.clear();
  int retval;
  const char* primary_audio_lang =
      "db/menu/broadcasting/audio_options/primary_audio_language";
  int ret = vconf_get_int(primary_audio_lang, &retval);
  if (ret != 0) {
    LOG(ERROR) << "vconf_get_int failed";
    return false;
  }
  audio_prefer_lang += (char)(retval >> 16 & 0xFF);
  audio_prefer_lang += (char)(retval >> 8 & 0xFF);
  LOG(INFO) << "primary audio lang is: " << audio_prefer_lang.c_str();
  return true;
}

void MediaPlayerBridgeCapi::SetActiveAudioTrack(int index) {
  LOG(INFO) << "active audio track index=" << index;
  if (index == -1) {
    player_set_mute(player_, true);
    active_audio_track_id_ = -1;
    return;
  }

  if (active_audio_track_id_ == index)
    return;
  active_audio_track_id_ = index;
  int ret = player_select_track_ex(player_, PLAYER_STREAM_TYPE_AUDIO,
                                   active_audio_track_id_, -1);
  if (ret != PLAYER_ERROR_NONE)
    LOG(WARNING) << "player_select_track_ex fail,ret:" << ret;
}

void MediaPlayerBridgeCapi::SetActiveVideoTrack(int index) {
  LOG(INFO) << "active video track index=" << index;
  if (index == -1) {
    player_set_display_visible(player_, false);
    active_video_track_id_ = -1;
    return;
  }

  if (active_video_track_id_ == index)
    return;
  active_video_track_id_ = index;
  int ret = player_select_track_ex(player_, PLAYER_STREAM_TYPE_VIDEO,
                                   active_video_track_id_, -1);
  if (ret != PLAYER_ERROR_NONE)
    LOG(WARNING) << "player_select_track_ex fail,ret:" << ret;
}

void MediaPlayerBridgeCapi::SubtitleDataCB(unsigned long long time_stamp,
                                           unsigned index,
                                           const std::string& buffer,
                                           unsigned buffer_size) {
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerBridgeCapi::NotifySubtitleData,
                            weak_factory_.GetWeakPtr(), index, time_stamp,
                            buffer, buffer_size));
}

void MediaPlayerBridgeCapi::NotifySubtitleData(int track_id,
                                               double time_stamp,
                                               const std::string& data,
                                               unsigned int size) {
  if (!player_)
    return;

  if (!content::IsHbbTV())
    return;

  content::WebContentsDelegate* web_contents_delegate =
      GetWebContentsDelegate();
  if (!web_contents_delegate)
    return;

  web_contents_delegate->NotifySubtitleData(track_id, time_stamp, data, size);
}

void MediaPlayerBridgeCapi::NotifySubtitleState(int state, double time_stamp) {
  if (!content::IsHbbTV())
    return;
  content::WebContentsDelegate* web_contents_delegate =
      GetWebContentsDelegate();
  if (!web_contents_delegate)
    return;
  if (SubtitleNotificationEnabled()) {
    if (subtitle_state_ == state)
      return;
    LOG(INFO) << "state:" << state << ", time_stamp:" << time_stamp;
    int old_state = subtitle_state_;
    if (state != blink::WebMediaPlayer::kSubtitleSeekStart &&
        state != blink::WebMediaPlayer::kSubtitleSeekComplete)
      subtitle_state_ = state;

    switch (state) {
      case blink::WebMediaPlayer::kSubtitlePlay:
        if (old_state == blink::WebMediaPlayer::kSubtitlePause)
          web_contents_delegate->NotifySubtitleState(
              blink::WebMediaPlayer::kSubtitleResume);
        else
          NotifyPlayTrack();
        break;
      case blink::WebMediaPlayer::kSubtitlePause:
        if (old_state != blink::WebMediaPlayer::kSubtitleStop)
          web_contents_delegate->NotifySubtitleState(state);
        break;
      case blink::WebMediaPlayer::kSubtitleStop:
        NotifyStopTrack();
        break;
      case blink::WebMediaPlayer::kSubtitleSeekStart:
        web_contents_delegate->NotifySubtitleState(state, time_stamp);
        break;
      case blink::WebMediaPlayer::kSubtitleSeekComplete:
        web_contents_delegate->NotifySubtitleState(state);
        break;
      default:
        NOTREACHED();
        return;
    }
  }
}

void MediaPlayerBridgeCapi::SetActiveTextTrack(int id, bool is_in_band) {
  LOG(INFO) << "id=" << id << ",is_in_band=" << is_in_band;
  if (!SubtitleNotificationEnabled())
    return;

  if (id == -1) {
    NotifyStopTrack();
    return;
  }

  if (active_text_track_id_ == id)
    return;

  active_text_track_id_ = id;
  is_inband_text_track_ = is_in_band;
  NotifyPlayTrack();
}

void MediaPlayerBridgeCapi::NotifyPlayTrack() {
  if (active_text_track_id_ == -1 ||
      subtitle_state_ == blink::WebMediaPlayer::kSubtitleStop)
    return;
  if (is_inband_text_track_) {
    // process select player's track ...
    UpdateActiveTextTrack(active_text_track_id_);
    NotifySubtitlePlay(active_text_track_id_, blink::WebString().Utf8(),
                       blink::WebString().Utf8());
  } else {
    // get track info
    manager()->GetPlayTrackInfo(GetPlayerId(), active_text_track_id_);
  }
}

void MediaPlayerBridgeCapi::NotifyStopTrack() {
  active_text_track_id_ = -1;  // set default : -1
  if (is_inband_text_track_)
    UpdateActiveTextTrack(active_text_track_id_);

  is_inband_text_track_ = false;  // set default : false
  content::WebContentsDelegate* web_contents_delegate =
      GetWebContentsDelegate();
  if (!web_contents_delegate)
    return;
  web_contents_delegate->NotifySubtitleState(
      blink::WebMediaPlayer::kSubtitleStop);
}

void MediaPlayerBridgeCapi::NotifySubtitlePlay(int id,
                                               const std::string& url,
                                               const std::string& lang) {
  content::WebContentsDelegate* web_contents_delegate =
      GetWebContentsDelegate();
  if (!web_contents_delegate)
    return;
  web_contents_delegate->NotifySubtitlePlay(id, url, lang);
}

void MediaPlayerBridgeCapi::UpdateActiveTextTrack(int id) {
  int ret = PLAYER_ERROR_NONE;

  // unselected...
  if (id == -1) {
    ret = player_close_subtitle_cb(player_);
    if (ret != PLAYER_ERROR_NONE)
      LOG(ERROR) << "player_close_subtitle_cb"
                 << GetString(static_cast<player_error_e>(ret));

    return;
  }

  int count = 0;
  int err = player_get_track_count(player_, PLAYER_STREAM_TYPE_TEXT, &count);
  if (err != PLAYER_ERROR_NONE || count <= 0 || id >= count) {
    LOG(ERROR) << "player_get_track_count err,ret=" << ret
               << ",count=" << count;
    return;
  }

  // or player_set_active_subtitle_track(player, track_num)
  ret = player_select_track_ex(player_, PLAYER_STREAM_TYPE_TEXT, id, -1);
  if (ret != PLAYER_ERROR_NONE)
    LOG(ERROR) << "player_select_track"
               << GetString(static_cast<player_error_e>(ret));
}
#endif

void MediaPlayerBridgeCapi::OnPlaybackCompleteUpdate() {
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerBridgeCapi::PlaybackCompleteUpdate,
                            weak_factory_.GetWeakPtr()));
}

void MediaPlayerBridgeCapi::OnSeekCompleteUpdate() {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&MediaPlayerBridgeCapi::SeekCompleteUpdate,
                                    weak_factory_.GetWeakPtr()));
}

void MediaPlayerBridgeCapi::OnPlayerPrepared() {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&MediaPlayerBridgeCapi::PlayerPrepared,
                                    weak_factory_.GetWeakPtr()));
}

void MediaPlayerBridgeCapi::OnHandleBufferingStatus(int percent) {
#if defined(OS_TIZEN_TV_PRODUCT)
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerBridgeCapi::HandleBufferingStatusOnTV,
                            weak_factory_.GetWeakPtr(), percent));
#else
  task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerBridgeCapi::HandleBufferingStatus,
                            weak_factory_.GetWeakPtr(), percent));
#endif
}

void MediaPlayerBridgeCapi::OnHandlePlayerError(
    int player_error_code,
    const base::Location& location) {
  // If invoked from other threads, relay to current task_runner.
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&MediaPlayerBridgeCapi::OnHandlePlayerError,
                   weak_factory_.GetWeakPtr(), player_error_code, location));
    return;
  }

  LOG(ERROR) << GetString(static_cast<player_error_e>(player_error_code))
             << " from " << location.function_name() << "("
             << location.line_number() << ")";
  error_occured_ = true;
#if defined(OS_TIZEN_TV_PRODUCT)
  decryptor_condition_.Signal();
  NotifySubtitleState(blink::WebMediaPlayer::kSubtitleStop);
#endif
  OnMediaError(GetMediaError(player_error_code));
  OnPlayerSuspendRequest();
}

void MediaPlayerBridgeCapi::OnResumeComplete(bool success) {
  is_resuming_ = false;
  auto current_time = GetCurrentTime();
  if (stored_seek_time_during_resume_ != media::kNoTimestamp &&
      stored_seek_time_during_resume_ != current_time) {
    Seek(stored_seek_time_during_resume_);
  }
  stored_seek_time_during_resume_ = media::kNoTimestamp;
  if (success) {
    current_time = GetCurrentTime();
    manager()->OnResumeComplete(GetPlayerId());
    manager()->OnTimeUpdate(GetPlayerId(), current_time);
#if defined(OS_TIZEN_TV_PRODUCT)
    UpdateCurrentTime(current_time);
#endif
  }
}

void MediaPlayerBridgeCapi::OnInitComplete(bool success) {
  is_initialized_ = true;
  manager()->OnInitComplete(GetPlayerId(), success);
}

void MediaPlayerBridgeCapi::EnteredFullscreen() {
  LOG(INFO) << "MPBC->EnteredFullscreen.";

  MediaPlayerEfl::EnteredFullscreen();
#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
  if (!content::IsWebBrowser()) {
    LOG(INFO) << "mixer mode enter fullscreen";

    if (!IsMuted() && mixer_player_active_audio_ != player_) {
      if (mixer_player_active_audio_) {
        player_deactivate_stream(mixer_player_active_audio_,
                                 PLAYER_STREAM_TYPE_AUDIO);
      }
      player_activate_stream(player_, PLAYER_STREAM_TYPE_AUDIO);
      mixer_player_active_audio_ = player_;
    }
  }
#endif

#if defined(TIZEN_VIDEO_HOLE) && !defined(OS_TIZEN_TV_PRODUCT)
  if (GetPlayerState() == PLAYER_STATE_NONE || !is_video_hole_)
    return;

  int ret = player_enable_media_packet_video_frame_decoded_cb(player_, false);
  if (ret != PLAYER_ERROR_NONE) {
    OnHandlePlayerError(ret, FROM_HERE);
    return;
  }
#endif
}

void MediaPlayerBridgeCapi::ExitedFullscreen() {
  MediaPlayerEfl::ExitedFullscreen();

#if defined(TIZEN_VIDEO_HOLE) && !defined(OS_TIZEN_TV_PRODUCT)
  if (GetPlayerState() == PLAYER_STATE_NONE || !is_video_hole_)
    return;

  int ret = player_enable_media_packet_video_frame_decoded_cb(player_, true);
  if (ret != PLAYER_ERROR_NONE) {
    OnHandlePlayerError(ret, FROM_HERE);
    return;
  }
#endif
}

#if defined(TIZEN_VD_MULTIPLE_MIXERDECODER)
bool MediaPlayerBridgeCapi::IsMuted(void) {
  LOG(INFO) << "MediaPlayerBridgeCapi::IsMuted-------volume_ = " << volume_;
  if (volume_ <= 0.00000001 && volume_ >= -0.00000001) {
    return true;
  }

  return false;
}
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
void MediaPlayerBridgeCapi::SetDecryptor(eme::eme_decryptor_t decryptor) {
  LOG(INFO) << "Received decryptor " << reinterpret_cast<void*>(decryptor);

  base::AutoLock auto_lock(decryptor_lock_);
  decryptor_ = decryptor;

  if (decryptor == 0) {
    OnHandlePlayerError(PLAYER_ERROR_DRM_EXPIRED, FROM_HERE);
  }

  decryptor_condition_.Signal();
}

bool MediaPlayerBridgeCapi::IsStandaloneEme() const {
  // assumes HbbTV DRM is inactive
  bool is_hbbtv_drm_inactive = true;

  content::WebContentsDelegate* contents_delegate = GetWebContentsDelegate();
  if (contents_delegate != nullptr) {
    auto& active_drm = contents_delegate->GetActiveDRM();

    LOG(INFO) << "active DRM: '" << active_drm << "'";

    // for HbbTV 2.0.2 assumes inactive DRM unless drmAgent.setActiveDRM() was
    // called with something else than "urn:hbbtv:oipfdrm:inactive"

    is_hbbtv_drm_inactive =
        active_drm.empty() || (active_drm == "urn:hbbtv:oipfdrm:inactive");
  }

  bool ret = content::IsHbbTV() && (stream_type_ == DASH_STREAM) &&
             is_hbbtv_drm_inactive && !uses_hbbtv_drm_ &&
             has_encrypted_listener_or_cdm_;

  LOG(INFO) << "is HbbTV: " << content::IsHbbTV()
            << ", HbbTV DRM inactive: " << is_hbbtv_drm_inactive
            << ", uses HbbTV DRM: " << uses_hbbtv_drm_
            << ", stream type: " << stream_type_
            << ", has encrypted listener or CDM: "
            << has_encrypted_listener_or_cdm_ << ", standalone EME: " << ret;

  return ret;
}

int MediaPlayerBridgeCapi::PreparePlayerForDrmEme() {
  LOG(INFO) << "Preparing player for stand-alone EME";

  auto player_res = player_set_drm_handle(player_, PLAYER_DRM_TYPE_EME, 0);
  if (PLAYER_ERROR_NONE != player_res) {
    LOG(ERROR) << "Failed to set drm handle with error [" << player_res << "]";
    return player_res;
  }

  player_res = player_set_drm_init_data_cb(player_, OnPlayerDrmInitData, this);
  if (PLAYER_ERROR_NONE != player_res) {
    LOG(ERROR) << "Failed to set drm init data cb with error [" << player_res
               << "]";
    return player_res;
  }

  player_res =
      player_set_drm_init_complete_cb(player_, OnPlayerDrmInitComplete, this);
  if (PLAYER_ERROR_NONE != player_res) {
    LOG(ERROR) << "Failed to set drm init complete cb with error ["
               << player_res << "]";
    return player_res;
  }

  return player_res;
}

int MediaPlayerBridgeCapi::OnPlayerDrmInitData(drm_init_data_type init_type,
                                               void* data,
                                               int data_length,
                                               void* user_data) {
  LOG(INFO) << "Received init data, size: " << data_length
            << ", type: " << init_type;
  if (init_type != CENC) {
    LOG(WARNING) << "Not CENC init data type";
    return NOT_CENC_DATA;
  }
  if (!user_data) {
    LOG(ERROR) << "No user data";
    return NO_USER_DATA;
  }

  auto thiz = static_cast<MediaPlayerBridgeCapi*>(user_data);

  auto data_bytes = static_cast<unsigned char*>(data);
  std::vector<unsigned char> init_data(data_bytes, data_bytes + data_length);
  thiz->manager()->OnInitData(thiz->GetPlayerId(), init_data);

  return ERROR_NONE;
}

bool MediaPlayerBridgeCapi::OnPlayerDrmInitComplete(int* drmhandle,
                                                    unsigned int length,
                                                    unsigned char* psshdata,
                                                    void* user_data) {
  if (!user_data) {
    LOG(ERROR) << "No user data";
    return false;
  }

  LOG(INFO) << "DRM init completion requested";

  auto thiz = static_cast<MediaPlayerBridgeCapi*>(user_data);
  auto result = thiz->CompleteDrmInit(*drmhandle);

  LOG(INFO) << "Returning decryptor " << reinterpret_cast<void*>(*drmhandle);

  return result;
}

bool MediaPlayerBridgeCapi::CompleteDrmInit(int& drmhandle) {
  base::AutoLock auto_lock(decryptor_lock_);

  if (decryptor_ == 0) {
    // OnPlayerDrmInitComplete callback will be invoked by CAPI Player when
    // drmeme GStreamer element will attempt decrypting first frame.
    // drmeme then waits 20 seconds during which it expects to receive
    // decryptor handle. If this time-outs player error is reported.
    // If the decryptor_ is not yet known below we need to wait for it.
    // It will arrive when Media Keys Session is updated with license data
    // for the first time.
    // If the application misbehaves the wait here would be indefinite blocking
    // other callbacks registered in CAPI Player.
    // The timed wait here is a safety net for cases when the
    // OnPlayerDrmInitComplete callback was invoked but we the decryptor handle
    // doesn’t arrive on time.
    // 20 second was picked since it makes no sense to wait longer because
    // drmeme will time-out anyway just few moments earlier.
    LOG(INFO) << "Waiting for decryptor...";
    manager()->OnWaitingForKey(GetPlayerId());
    decryptor_condition_.TimedWait(
        base::TimeDelta::FromSeconds(kDrmTimeoutSeconds));
    if (decryptor_ == 0) {
      LOG(ERROR) << "No decryptor received before drmeme timeout!";
      OnMediaError(MEDIA_ERROR_DECODE);
    }
  } else {
    LOG(INFO) << "Decryptor already set";
  }

  drmhandle = decryptor_;
  return (decryptor_ != 0);
}
#endif

PlayerRoleFlags MediaPlayerBridgeCapi::GetRoles() const noexcept {
  return PlayerRole::UrlBased;
}
}  // namespace media
