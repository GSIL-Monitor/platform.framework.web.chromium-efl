// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_smplayer_adapter.h"

#include <ISmPlayer.h>
#include <capi-system-info/system_info.h>

#include <algorithm>
#include <cstring>

#include "ewk/efl_integration/ewk_extension_system_delegate.h"

#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_player_utils_tizen.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "ppapi/c/pp_errors.h"

namespace content {

namespace {
constexpr uint32_t kBitratesRequestCount = 64;
constexpr bool kAccurateSeek = true;
constexpr uint32_t kUltraHDWidth = 3840;
constexpr uint32_t kUltraHDHeight = 2160;
constexpr uint32_t kFullHDWidth = 1920;
constexpr uint32_t kFullHDHeight = 1080;

static const char kFilePrefix[] = "file://";

inline SmpTrackType ToSMPlayerTrackType(
    PP_ElementaryStream_Type_Samsung pp_type) {
  switch (pp_type) {
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
      return SMP_TRACK_TYPE_VIDEO;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
      return SMP_TRACK_TYPE_AUDIO;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_TEXT:
      return SMP_TRACK_TYPE_SUBTITLE;
    default:
      NOTREACHED();
      return SMP_TRACK_TYPE_MAX;
  }
}

// @return seconds converted to SMPlayer PTS format [ns]
inline uint64_t ToSMPlayerPts(PP_TimeTicks seconds) {
  return static_cast<uint64_t>(seconds * 1e9);
}

// @return seconds converted to [ms]
inline int32_t ToSMPlayerTimeDeltaMs(PP_TimeDelta seconds) {
  return static_cast<int32_t>(seconds * 1e3);
}

// NOLINTNEXTLINE(runtime/int)
inline unsigned long ToSMPlayerTimeTicksUnsignedMs(PP_TimeDelta seconds) {
  // NOLINTNEXTLINE(runtime/int)
  return static_cast<unsigned long>(seconds * 1e3);
}

inline void UpdateMaxResolution(const ppapi::VideoCodecConfig& pp_config,
                                SmpVideoStreamInfo* smp_info) {
  if (pp_config.frame_size.width > 0 && pp_config.frame_size.height > 0) {
    smp_info->max_width = pp_config.frame_size.width;
    smp_info->max_height = pp_config.frame_size.height;
    return;
  }

  bool uhd_panel_supported;
  int ret = system_info_get_value_bool(SYSTEM_INFO_KEY_UD_PANEL_SUPPORTED,
                                       &uhd_panel_supported);
  bool can_use_uhd = (ret == SYSTEM_INFO_ERROR_NONE && uhd_panel_supported);
  // Due to H/W decoder limiations we can set UHD resolution
  // only for HEVC (H265) codec, for other codecs we must use FHD resolution.
  can_use_uhd =
      can_use_uhd && pp_config.codec == PP_VIDEOCODEC_TYPE_SAMSUNG_H265;

  if (can_use_uhd) {
    smp_info->max_width = kUltraHDWidth;
    smp_info->max_height = kUltraHDHeight;
  } else {
    smp_info->max_width = kFullHDWidth;
    smp_info->max_height = kFullHDHeight;
  }
}

inline SmpVideoStreamInfo ToSMPlayerStreamInfo(
    const PepperVideoStreamInfo& pp_info) {
  SmpVideoStreamInfo smp_info;
  memset(&smp_info, 0x00, sizeof(SmpVideoStreamInfo));
  const auto& pp_config = pp_info.config;
  smp_info.mime = pepper_player_utils::GetCodecMimeType(pp_config.codec);
  smp_info.framerate_num = pp_config.frame_rate.first;
  smp_info.framerate_den = pp_config.frame_rate.second;
  smp_info.width = pp_config.frame_size.width;
  smp_info.height = pp_config.frame_size.height;
  // TODO(p.balut): fix line below
  smp_info.codec_extradata =
      const_cast<unsigned char*>(pp_config.extra_data.data());
  smp_info.extradata_size = pp_config.extra_data.size();
  smp_info.version = pepper_player_utils::GetCodecVersion(pp_config.codec);
  // We can always set drm type as EME as this properly supports pushing both
  // encrypted and clean packets to player
  smp_info.drm_type = SMP_DRMTYPE_EME;
  smp_info.is_preset =
      (pp_info.initialization_mode == PP_STREAMINITIALIZATIONMODE_MINIMAL);
  if (smp_info.is_preset)
    UpdateMaxResolution(pp_config, &smp_info);
  return smp_info;
}

inline SmpAudioStreamInfo ToSMPlayerStreamInfo(
    const PepperAudioStreamInfo& pp_info) {
  SmpAudioStreamInfo smp_info;
  memset(&smp_info, 0x00, sizeof(SmpAudioStreamInfo));
  const auto& pp_config = pp_info.config;
  smp_info.mime = pepper_player_utils::GetCodecMimeType(pp_config.codec);
  smp_info.channels =
      pepper_player_utils::GetChannelsCount(pp_config.channel_layout);
  smp_info.sample_rate = pp_config.samples_per_second;
  smp_info.bit_rate = pp_config.bits_per_channel;  // ??
  smp_info.block_align = 0;
  // TODO(p.balut): fix line below
  smp_info.codec_extradata =
      const_cast<unsigned char*>(pp_config.extra_data.data());
  smp_info.extradata_size = pp_config.extra_data.size();
  smp_info.version = pepper_player_utils::GetCodecVersion(pp_config.codec);
  smp_info.user_info = 0;
  // We can always set drm type as EME as this properly supports pushing both
  // encrypted and clean packets to player
  smp_info.drm_type = SMP_DRMTYPE_EME;
  smp_info.is_preset =
      (pp_info.initialization_mode == PP_STREAMINITIALIZATIONMODE_MINIMAL);
  return smp_info;
}

inline PP_MediaPlayerState ToPPState(SmPlayerState smp_state) {
  switch (smp_state) {
    case SM_PLAYER_STATE_NONE:
      return PP_MEDIAPLAYERSTATE_NONE;
    case SM_PLAYER_STATE_NULL:
    case SM_PLAYER_STATE_IDLE:
      return PP_MEDIAPLAYERSTATE_UNINITIALIZED;
    case SM_PLAYER_STATE_READY:
      return PP_MEDIAPLAYERSTATE_READY;
    case SM_PLAYER_STATE_PLAYING:
      return PP_MEDIAPLAYERSTATE_PLAYING;
    case SM_PLAYER_STATE_PAUSED:
      return PP_MEDIAPLAYERSTATE_PAUSED;
    default:
      NOTREACHED();
      return PP_MEDIAPLAYERSTATE_NONE;
  }
}

}  // namespace

void PepperSMPlayerAdapter::SmPlayerDeleter::operator()(ISmPlayer* player) {
  if (player)
    ISmPlayer::Destroy(player);
}

PepperSMPlayerAdapter::PepperSMPlayerAdapter()
    : player_(nullptr),
      initialization_condition_(&initialization_lock_),
      initialization_state_(InitializationState::kNotInitialized),
      seek_in_progres_(false),
      seek_time_(0.),
      is_playing_(false),
      external_subs_attached_(false),
      state_(PP_MEDIAPLAYERSTATE_UNINITIALIZED),
      weak_ptr_factory_(this) {}

PepperSMPlayerAdapter::~PepperSMPlayerAdapter() {
  Reset();
}

int32_t PepperSMPlayerAdapter::Initialize() {
  assert(player_ == nullptr);

  ISmPlayerPtr player{ISmPlayer::Create()};

  if (player) {
    // TODO(p.balut): I gather this will prevent us from playing audio-only.
    if (!player->SetMediaType(SMP_MEDIA_TYPE_MOVIE)) {
      LOG(ERROR) << "Failed setting media type.";
      return PP_ERROR_FAILED;
    }

    if (!player->SetDecodeType(DECODER_TYPE_HW)) {
      LOG(ERROR) << "Failed setting HW decoder.";
      return PP_ERROR_FAILED;
    }

    if (!player->SetMessageCallback(CallOnMessage, this)) {
      LOG(ERROR) << "Failed registering message callback.";
      return PP_ERROR_FAILED;
    }

    player_ = std::move(player);
    return PP_OK;
  }
  return PP_ERROR_FAILED;
}

int32_t PepperSMPlayerAdapter::Reset() {
  Unprepare();

  PepperPlayerAdapterBase::Reset();
  player_.reset();

  return PP_OK;
}

int32_t PepperSMPlayerAdapter::PrepareES() {
  if (!player_->SetDemuxType(DEMUXER_TYPE_EXTERNAL)) {
    LOG(ERROR) << "Failed setting external demuxer mode.";
    return PP_ERROR_FAILED;
  }

  initialization_cb_runner_ = base::MessageLoop::current()->task_runner();

  {
    base::AutoLock auto_lock{initialization_lock_};
    initialization_state_ = InitializationState::kNotInitialized;

    if (!player_->InitPlayer("es://")) {
      LOG(ERROR) << "Failed initializing player in ES mode.";
      return PP_ERROR_FAILED;
    }

    while (initialization_state_ == InitializationState::kNotInitialized)
      initialization_condition_.Wait();

    if (initialization_state_ == InitializationState::kFailure)
      return PP_ERROR_FAILED;
  }  // auto_lock
  state_ = PP_MEDIAPLAYERSTATE_READY;
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::PrepareURL(const std::string& url) {
  if (!player_->SetDemuxType(DEMUXER_TYPE_FFMPEG)) {
    LOG(ERROR) << "Failed setting demuxer.";
    return PP_ERROR_FAILED;
  }

  if (!player_->StartUsingBuffer(false)) {
    LOG(ERROR) << "Failed setting buffering mode.";
    return PP_ERROR_FAILED;
  }

  initialization_cb_runner_ = base::MessageLoop::current()->task_runner();

  {
    base::AutoLock auto_lock{initialization_lock_};
    initialization_state_ = InitializationState::kNotInitialized;

    // When accessing file from file system
    // SM Player needs path without "file://" prefix
    std::string proper_url = url;
    if (!proper_url.compare(0, sizeof(kFilePrefix) - 1, kFilePrefix))
      proper_url = proper_url.substr(sizeof(kFilePrefix) - 1);

    if (!player_->InitPlayer(proper_url.c_str())) {
      LOG(ERROR) << "Failed initializing player in URL mode.";
      return PP_ERROR_FAILED;
    }

    while (initialization_state_ == InitializationState::kNotInitialized)
      initialization_condition_.Wait();

    if (initialization_state_ == InitializationState::kFailure)
      return PP_ERROR_FAILED;
  }  // auto_lock
  state_ = PP_MEDIAPLAYERSTATE_READY;
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::Unprepare() {
  // Smplayer deletes pipeline on Stop so it is an equivalent to our Unprepare
  if (player_ && !player_->Stop()) {
    LOG(ERROR) << "A call to Stop failed.";
    return PP_ERROR_FAILED;
  }
  state_ = PP_MEDIAPLAYERSTATE_UNINITIALIZED;
  is_playing_ = false;
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::Play() {
  if (is_playing_) {
    if (!player_->Resume()) {
      LOG(ERROR) << "A call to Resume failed.";
      return PP_ERROR_FAILED;
    }
  } else {
    if (!player_->Play()) {
      LOG(ERROR) << "A call to Play failed.";
      return PP_ERROR_FAILED;
    }
    is_playing_ = true;
  }
  state_ = PP_MEDIAPLAYERSTATE_PLAYING;
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::Pause() {
  if (state_ != PP_MEDIAPLAYERSTATE_PLAYING || !player_->Pause()) {
    LOG(ERROR) << "A call to Pause failed.";
    return PP_ERROR_FAILED;
  }
  state_ = PP_MEDIAPLAYERSTATE_PAUSED;
  return PP_OK;
}

void PepperSMPlayerAdapter::OnSeekCompleted() {
  seek_in_progres_ = false;
  RunAndClearCallback<PepperPlayerCallbackType::SeekCompletedCallback>();
}

void PepperSMPlayerAdapter::OnSeekStartedBuffering() {
  seek_in_progres_ = true;
}

int32_t PepperSMPlayerAdapter::Seek(PP_TimeTicks time,
                                    const base::Closure& callback) {
  if (!player_->IsSeekAvailable())
    return PP_ERROR_NOTSUPPORTED;
  SetCallback<PepperPlayerCallbackType::SeekCompletedCallback>(callback);
  if (!player_->Seek(ToSMPlayerTimeTicksUnsignedMs(time))) {
    LOG(ERROR) << "A call to seek failed.";
    return PP_ERROR_FAILED;
  }
  seek_time_ = time;
  return PP_OK;
}

void PepperSMPlayerAdapter::SeekOnAppendThread(PP_TimeTicks time) {}

int32_t PepperSMPlayerAdapter::SelectTrack(
    PP_ElementaryStream_Type_Samsung stream_type,
    uint32_t track_index) {
  // TODO(p.balut): implement
  return PP_ERROR_NOTSUPPORTED;
}

int32_t PepperSMPlayerAdapter::SetPlaybackRate(float rate) {
  if (!player_->SetPlaySpeed(static_cast<float>(rate)))
    return PP_ERROR_FAILED;
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SubmitEOSPacket(
    PP_ElementaryStream_Type_Samsung track_type) {
  if (!player_->SubmitEOS(ToSMPlayerTrackType(track_type)))
    return PP_ERROR_FAILED;
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SubmitPacket(
    const ppapi::ESPacket* packet,
    PP_ElementaryStream_Type_Samsung track_type) {
  SmpTrackType sm_stream_type = ToSMPlayerTrackType(track_type);
  uint64_t pts = ToSMPlayerPts(packet->Pts());
  uint8_t* data = static_cast<uint8_t*>(const_cast<void*>(packet->Data()));

  if (!player_->SubmitPacket(data, packet->DataSize(), pts, sm_stream_type)) {
    LOG(ERROR) << "Failed to submit clear ES packet, pts = " << packet->Pts()
               << " s, pp_type = " << track_type;
    return PP_ERROR_FAILED;
  }
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SubmitEncryptedPacket(
    const ppapi::ESPacket* packet,
    ScopedHandleAndSize handle,
    PP_ElementaryStream_Type_Samsung track_type) {
  SmpTrackType sm_stream_type = ToSMPlayerTrackType(track_type);
  uint64_t pts = ToSMPlayerPts(packet->Pts());

  es_player_drm_info drm_info;
  memset(&drm_info, 0x00, sizeof(es_player_drm_info));
  drm_info.drm_type = SMP_DRMTYPE_EME;
  drm_info.tz_handle = handle->handle;

  if (!player_->SubmitPacket(nullptr, handle->size, pts, sm_stream_type,
                             &drm_info)) {
    LOG(ERROR) << "Failed to submit encrypted ES packet, pts = "
               << packet->Pts() << " s, pp_type = " << track_type;
    return PP_ERROR_FAILED;
  }
  ClearHandle(handle.get());
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::Stop() {
  // TODO(p.galiszewsk): make sure below code works properly
  if (state_ < PP_MEDIAPLAYERSTATE_PLAYING || !player_->Seek(0) ||
      !player_->Pause()) {
    LOG(ERROR) << "A call to Pause/Seek failed.";
    return PP_ERROR_FAILED;
  }
  state_ = PP_MEDIAPLAYERSTATE_READY;
  return PP_OK;
}

void PepperSMPlayerAdapter::Stop(
    const base::Callback<void(int32_t)>& callback) {
  auto result = Stop();
  callback.Run(result);
}

// Player setters
int32_t PepperSMPlayerAdapter::SetApplicationID(
    const std::string& application_id) {
  std::string desktop_id = "";
  std::string app_id = application_id;
  std::string widget_id = "";
  if (!player_->SetAppInfo(desktop_id, app_id, widget_id)) {
    LOG(ERROR) << "Failed to set application ID.";
    return PP_ERROR_FAILED;
  }
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SetAudioStreamInfo(
    const PepperAudioStreamInfo& stream_info) {
  SmpAudioStreamInfo smp_info = ToSMPlayerStreamInfo(stream_info);
  if (!player_->SetAudioStreamInfo(&smp_info)) {
    LOG(ERROR) << "Failed to set audio stream info.";
    return PP_ERROR_FAILED;
  }
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SetDisplay(void* display, bool is_windowless) {
  EwkExtensionSystemDelegate ewk_extension;
  ewk_extension.SetWindowId(static_cast<const Evas_Object*>(display));
  int window_id = ewk_extension.GetWindowId();
  if (!player_->SetDisplay(&window_id)) {
    LOG(ERROR) << "Failed to set display.";
    return PP_ERROR_FAILED;
  }
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SetDisplayRect(
    const PP_Rect& display_rect,
    const PP_FloatRect& crop_ratio_rect) {
  RectAreaInfo rect_area_info{display_rect.point.x, display_rect.point.y,
                              display_rect.size.width,
                              display_rect.size.height};

  if (!player_->SetAttribute(NULL, SMP_DISPLAY_AREA_DATA, &rect_area_info,
                             sizeof(RectAreaInfo), NULL)) {
    LOG(ERROR) << "Failed to set display area.";
    return PP_ERROR_FAILED;
  }

  /* TODO(m.jurkiewicz): crop not working
  if (!player->SetCropArea(crop_ratio_rect.point.x,
      crop_ratio_rect.point.y,
      crop_ratio_rect.size.width * display_rect.size.width,
      crop_ratio_rect.size.height * display_rect.size.height)) {
    LOG(ERROR) << "Failed to set crop area.";
    return PP_ERROR_FAILED;
  }
  */
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SetDisplayMode(
    PP_MediaPlayerDisplayMode display_mode) {
  return (display_mode == PP_MEDIAPLAYERDISPLAYMODE_STRETCH)
             ? PP_OK
             : PP_ERROR_NOTSUPPORTED;
}

bool PepperSMPlayerAdapter::IsDisplayModeSupported(
    PP_MediaPlayerDisplayMode display_mode) {
  return (display_mode == PP_MEDIAPLAYERDISPLAYMODE_STRETCH);
}

int32_t PepperSMPlayerAdapter::SetExternalSubtitlesPath(
    const std::string& file_path,
    const std::string& encoding) {
  // Passing subtitle callback here is obsolete according to docs.
  if (!player_->StartSubtitle(file_path.c_str(), nullptr)) {
    LOG(ERROR) << "Failed to set external subtitles.";
    return PP_ERROR_FAILED;
  }

  external_subs_attached_ = true;
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SetStreamingProperty(
    PP_StreamingProperty property,
    const std::string& data) {
  // TODO(p.balut): check which props are supported, if any
  return PP_ERROR_NOTSUPPORTED;
}

int32_t PepperSMPlayerAdapter::ValidateSubtitleEncoding(
    const std::string& encoding) {
  // TODO(p.balut): subtitle encoding
  return PP_ERROR_NOTSUPPORTED;
}

int32_t PepperSMPlayerAdapter::SetSubtitlesEncoding(
    const std::string& encoding) {
  // TODO(p.balut): subtitle encoding
  return PP_ERROR_NOTSUPPORTED;
}

int32_t PepperSMPlayerAdapter::SetSubtitlesDelay(PP_TimeDelta delay) {
  if (!player_->SetSubtitleSync(ToSMPlayerTimeDeltaMs(delay))) {
    LOG(ERROR) << "Failed to set subtitles delay.";
    return PP_ERROR_FAILED;
  }
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SetVideoStreamInfo(
    const PepperVideoStreamInfo& stream_info) {
  SmpVideoStreamInfo smp_info = ToSMPlayerStreamInfo(stream_info);
  if (!player_->SetVideoStreamInfo(&smp_info)) {
    LOG(ERROR) << "Failed to set video stream info.";
    return PP_ERROR_FAILED;
  }
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SetVr360Mode(PP_MediaPlayerVr360Mode) {
  return PP_ERROR_NOTSUPPORTED;
}

int32_t PepperSMPlayerAdapter::SetVr360Rotation(float, float) {
  return PP_ERROR_NOTSUPPORTED;
}

int32_t PepperSMPlayerAdapter::SetVr360ZoomLevel(uint32_t) {
  return PP_ERROR_NOTSUPPORTED;
}

int32_t PepperSMPlayerAdapter::GetAudioTracksList(
    std::vector<PP_AudioTrackInfo>* track_list) {
  assert(track_list);
  track_list->clear();
  unsigned count;

  if (!player_->GetTotalNumOfStreamID(SMP_STREAM_TYPE_AUDIO, &count)) {
    LOG(ERROR) << "Failed to read audio streams count.";
    return PP_ERROR_FAILED;
  }

  track_list->reserve(count);

  for (unsigned i = 0; i < count; ++i) {
    // Bitrate and channel can be obtained with player_->GetAudioTrackInfo, but
    // it is unsupported by PP_AudioTrackInfo ATM.
    std::string extra_data;
    std::string language;
    if (!player_->GetStreamLanguageCode(SMP_STREAM_TYPE_AUDIO, i, language,
                                        extra_data)) {
      LOG(ERROR) << "Failed to get lang code for audio track " << i << ".";
      return PP_ERROR_FAILED;
    }

    PP_AudioTrackInfo track_info;
    track_info.index = i;
    pepper_player_utils::CopyStringToArray(track_info.language,
                                           language.c_str());
    track_list->push_back(track_info);
  }

  return PP_OK;
}

int32_t PepperSMPlayerAdapter::GetAvailableBitrates(std::string* result) {
  assert(result != nullptr);
  result->clear();

  std::vector<unsigned int> available_bitrates;
  if (!player_->GetAvailableBitrates(available_bitrates)) {
    LOG(ERROR) << "Failed getting available bitrates.";
    return PP_ERROR_FAILED;
  }

  for (auto bitrate : available_bitrates) {
    if (!result->empty())
      result->append("|");
    result->append(std::to_string(bitrate));
  }

  return PP_OK;
}

int32_t PepperSMPlayerAdapter::GetCurrentTime(PP_TimeTicks* time) {
  // Since SMPlayer is unable to send current time when seeking
  // this must be emulated by adapter.
  if (seek_in_progres_) {
    *time = seek_time_;
    return PP_OK;
  }
  // NOLINTNEXTLINE(runtime/int)
  unsigned long time_ns = 0;
  if (!player_->GetCurrentPosition(&time_ns)) {
    LOG(ERROR) << "Failed to get current playback position.";
    return PP_ERROR_FAILED;
  }
  *time = pepper_player_utils::MilisecondsToPPTimeTicks(time_ns);
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::GetCurrentTrack(
    PP_ElementaryStream_Type_Samsung stream_type,
    int32_t* track_index) {
  unsigned cur_index = 0;
  SmpStreamType smp_stream_type;
  switch (stream_type) {
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
      smp_stream_type = SMP_STREAM_TYPE_VIDEO;
      break;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
      smp_stream_type = SMP_STREAM_TYPE_AUDIO;
      break;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_TEXT:
      smp_stream_type = external_subs_attached_
                            ? SMP_STREAM_TYPE_EXTERNAL_SUBTITLE
                            : SMP_STREAM_TYPE_INTERNAL_SUBTITLE;
      break;
    default:
      LOG(ERROR) << "Unknown pp_stream_type = " << stream_type;
      return PP_ERROR_FAILED;
  }
  if (!player_->GetCurrentStreamID(smp_stream_type, &cur_index)) {
    LOG(ERROR) << "Failed getting current stream id, smp_type = "
               << smp_stream_type;
    return PP_ERROR_FAILED;
  }
  *track_index = static_cast<int32_t>(cur_index);
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::GetDuration(PP_TimeDelta* duration) {
  // NOLINTNEXTLINE(runtime/int)
  unsigned long duration_ms = 0;
  if (!player_->GetDuration(&duration_ms)) {
    LOG(ERROR) << "Failed to get duration.";
    return PP_ERROR_FAILED;
  }

  *duration = pepper_player_utils::MilisecondsToPPTimeTicks(duration_ms);
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::SetDuration(PP_TimeDelta duration) {
  if (!player_->SetAppSrcDuration(ToSMPlayerTimeTicksUnsignedMs(duration))) {
    LOG(ERROR) << "Failed to set duration.";
    return PP_ERROR_FAILED;
  }
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::GetPlayerState(PP_MediaPlayerState* state) {
  *state = state_;
  return PP_OK;
}

int32_t PepperSMPlayerAdapter::GetStreamingProperty(
    PP_StreamingProperty property,
    std::string* property_value) {
  switch (property) {
    case PP_STREAMINGPROPERTY_AVAILABLE_BITRATES:
      return GetAvailableBitrates(property_value);
    default:
      // TODO(p.balut): check which props are supported, if any
      return PP_ERROR_NOTSUPPORTED;
  }
}

int32_t PepperSMPlayerAdapter::GetTextTracksList(
    std::vector<PP_TextTrackInfo>* track_list) {
  assert(track_list);
  track_list->clear();

  auto track_type = external_subs_attached_ ? SMP_STREAM_TYPE_EXTERNAL_SUBTITLE
                                            : SMP_STREAM_TYPE_INTERNAL_SUBTITLE;

  unsigned track_count;
  if (!player_->GetTotalNumOfStreamID(track_type, &track_count)) {
    LOG(ERROR) << "Failed to read text streams count.";
    return PP_ERROR_FAILED;
  }

  for (auto i = 0u; i != track_count; ++i) {
    std::string language;
    std::string extra_data;
    if (!player_->GetStreamLanguageCode(track_type, i, language, extra_data)) {
      LOG(ERROR) << "Failed to get lang code for text track " << i << ".";
      return PP_ERROR_FAILED;
    }

    PP_TextTrackInfo track_info;
    track_info.index = i;
    track_info.is_external =
        PP_FromBool(track_type == SMP_STREAM_TYPE_EXTERNAL_SUBTITLE);
    pepper_player_utils::CopyStringToArray(track_info.language, language);
    track_list->push_back(track_info);
  }

  return PP_OK;
}

int32_t PepperSMPlayerAdapter::GetVideoTracksList(
    std::vector<PP_VideoTrackInfo>* track_list) {
  assert(track_list);
  track_list->clear();

  int width = 0;
  int height = 0;
  if (!player_->GetResolution(&width, &height)) {
    LOG(ERROR) << "Failed to get resolution.";
    return PP_ERROR_FAILED;
  }

  // NOLINTNEXTLINE(runtime/int)
  unsigned long bitrate = 0;
  if (!player_->GetCurrentBitrates(&bitrate)) {
    LOG(ERROR) << "Failed to get current bitrate.";
    return PP_ERROR_FAILED;
  }

  PP_VideoTrackInfo track_info;
  track_info.index = 0;
  track_info.size = PP_MakeSize(width, height);
  track_info.bitrate = bitrate;
  track_list->push_back(track_info);
  return PP_OK;
}

// static
void PepperSMPlayerAdapter::OnAudioEnoughData(void* user_data) {
  if (!user_data)
    return;

  auto listener_wrapper =
      static_cast<PepperSMPlayerAdapter*>(user_data)->ListenerWrapper(
          PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO);

  if (!listener_wrapper)
    return;

  listener_wrapper->OnEnoughData();
}

// static
void PepperSMPlayerAdapter::OnAudioNeedData(unsigned bytes, void* user_data) {
  if (!user_data)
    return;

  auto listener_wrapper =
      static_cast<PepperSMPlayerAdapter*>(user_data)->ListenerWrapper(
          PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO);

  if (!listener_wrapper)
    return;

  listener_wrapper->OnNeedData(bytes);
}

// static
// NOLINTNEXTLINE(runtime/int)
bool PepperSMPlayerAdapter::OnAudioSeekData(unsigned long long offset,
                                            void* user_data) {
  if (!user_data)
    return false;

  PP_TimeTicks time_ticks =
      pepper_player_utils::NanosecondsToPPTimeTicks(offset);

  auto listener_wrapper =
      static_cast<PepperSMPlayerAdapter*>(user_data)->ListenerWrapper(
          PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO);

  if (!listener_wrapper)
    return false;

  listener_wrapper->OnSeekData(time_ticks);
  return true;
}

// static
void PepperSMPlayerAdapter::OnVideoEnoughData(void* user_data) {
  if (!user_data)
    return;

  auto listener_wrapper =
      static_cast<PepperSMPlayerAdapter*>(user_data)->ListenerWrapper(
          PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO);

  if (!listener_wrapper)
    return;

  listener_wrapper->OnEnoughData();
}

// static
void PepperSMPlayerAdapter::OnVideoNeedData(unsigned bytes, void* user_data) {
  if (!user_data)
    return;

  auto listener_wrapper =
      static_cast<PepperSMPlayerAdapter*>(user_data)->ListenerWrapper(
          PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO);

  if (!listener_wrapper)
    return;

  listener_wrapper->OnNeedData(bytes);
}

// static
// NOLINTNEXTLINE(runtime/int)
bool PepperSMPlayerAdapter::OnVideoSeekData(unsigned long long offset,
                                            void* user_data) {
  if (!user_data)
    return false;

  PP_TimeTicks time_ticks =
      pepper_player_utils::NanosecondsToPPTimeTicks(offset);

  auto listener_wrapper =
      static_cast<PepperSMPlayerAdapter*>(user_data)->ListenerWrapper(
          PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO);

  if (!listener_wrapper)
    return false;

  listener_wrapper->OnSeekData(time_ticks);

  return true;
}

int32_t PepperSMPlayerAdapter::SetErrorCallback(
    const base::Callback<void(PP_MediaPlayerError)>& callback) {
  SetCallback<PepperPlayerCallbackType::ErrorCallback>(callback);
  return PP_OK;
}

void PepperSMPlayerAdapter::RegisterMediaCallbacks(
    PP_ElementaryStream_Type_Samsung type) {
  switch (type) {
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO:
      player_->SetAppSrcAudioNeedDataCallback(OnAudioNeedData, this);
      player_->SetAppSrcAudioDataEnoughCallback(OnAudioEnoughData, this);
      player_->SetAppSrcAudioSeekCallback(OnAudioSeekData, this);
      break;
    case PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO:
      player_->SetAppSrcVideoNeedDataCallback(OnVideoNeedData, this);
      player_->SetAppSrcVideoDataEnoughCallback(OnVideoEnoughData, this);
      player_->SetAppSrcVideoSeekCallback(OnVideoSeekData, this);
      break;
    default:
      LOG(ERROR) << "Unknown stream type";
      break;
  }
}

void PepperSMPlayerAdapter::RegisterToBufferingEvents(
    bool /*should_register*/) {
  // This can be safely ignored. We are registred for events as long as
  // GetMediaEventsListener() returns a value. No explicit registration is
  // needed for SMPlayer.
}

void PepperSMPlayerAdapter::RegisterToMediaEvents(bool /*should_register*/) {
  // This can be safely ignored. We are registred for events as long as
  // GetMediaEventsListener() returns a value. No explicit registration is
  // needed for SMPlayer.
}

void PepperSMPlayerAdapter::RegisterToSubtitleEvents(bool /*should_register*/) {
  // This can be safely ignored. We are registred for events as long as
  // GetSubtitleListener() returns a value. No explicit registration is needed
  // for SMPlayer.
}

void PepperSMPlayerAdapter::OnError(PP_MediaPlayerError error,
                                    const char* msg) {
  LOG(ERROR) << msg;
  RunCallback<PepperPlayerCallbackType::ErrorCallback>(error);
  if (auto listener = GetMediaEventsListener())
    listener->OnError(error);
}

void PepperSMPlayerAdapter::OnInitComplete() {
  DCHECK(initialization_cb_runner_);

  {
    base::AutoLock auto_lock{initialization_lock_};
    initialization_state_ = InitializationState::kSuccess;
  }
  initialization_condition_.Signal();

  initialization_cb_runner_->PostTask(
      FROM_HERE, base::Bind(&PepperSMPlayerAdapter::SendBufferingEvents,
                            weak_ptr_factory_.GetWeakPtr()));
}

void PepperSMPlayerAdapter::SendBufferingEvents() {
  if (auto buffering_listener = GetBufferingListener()) {
    buffering_listener->OnBufferingStart();
    buffering_listener->OnBufferingComplete();
  }
}

void PepperSMPlayerAdapter::OnInitFailed() {
  OnError(PP_MEDIAPLAYERERROR_UNKNOWN, "Preparing player failed.");
  {
    base::AutoLock auto_lock{initialization_lock_};
    initialization_state_ = InitializationState::kFailure;
  }
  initialization_condition_.Signal();
}

void PepperSMPlayerAdapter::OnShowSubtitle(void* param) {
  auto msg = static_cast<SmpMessageParamType*>(param);
  // send only text subtitles
  if (msg->subtitle.type)
    return;

  PP_TimeDelta pp_duration =
      pepper_player_utils::MilisecondsToPPTimeTicks(msg->subtitle.duration);
  std::string subtitle_string(static_cast<const char*>(msg->data));

  auto listener = GetSubtitleListener();
  if (!listener || pp_duration <= 0) {
    return;
  }

  listener->OnShowSubtitle(pp_duration, std::move(subtitle_string));
}

void PepperSMPlayerAdapter::OnMessage(int msg_id, void* param) {
  switch (msg_id) {
    /* General: */
    case SMP_MESSAGE_BEGIN_OF_STREAM:
      break;
    case SMP_MESSAGE_END_OF_STREAM:
      if (is_playing_) {
        is_playing_ = false;
        if (auto listener = GetMediaEventsListener())
          listener->OnEnded();
      }
      break;

    case SMP_MESSAGE_BUFFERING:
      // SMPlayer does not generate buffering callbacks
      break;

    /* Complete notifications: */
    case SMP_MESSAGE_SEEK_COMPLETED:
      // SMP_MESSAGE_SEEK_COMPLETED is send by SMPlayer when playback is
      // in new place runing.
      OnSeekCompleted();
      break;
    case SMP_MESSAGE_SEEK_DONE:
      // SMP_MESSAGE_SEEK_DONE is send by SMPlayer when seek called on
      // Gstreamer.
      // For seek to actualy happen new packets must be buffered.
      OnSeekStartedBuffering();
      break;
    case SMP_MESSAGE_INIT_COMPLETE:
      OnInitComplete();
      break;
    case SMP_MESSAGE_PAUSE_COMPLETE:
      break;
    case SMP_MESSAGE_RESUME_COMPLETE:
      break;
    case SMP_MESSAGE_STOP_SUCCESS:
      break;

    /* Subtitles: */
    case SMP_MESSAGE_UPDATE_SUBTITLE:
      OnShowSubtitle(param);
      break;
    case SMP_MESSAGE_SUBTITLE_TEXT:
      // Deprecated. Subtitles are sent now with UPDATE_SUBTITLE message
      break;

    /* Errors: */
    case SMP_MESSAGE_INIT_FAILED:
      OnInitFailed();
      break;
    case SMP_MESSAGE_PLAY_FAILED:
      OnError(PP_MEDIAPLAYERERROR_UNKNOWN, "Play failed.");
      break;
    case SMP_MESSAGE_PAUSE_FAILED:
      OnError(PP_MEDIAPLAYERERROR_UNKNOWN, "Pause failed.");
      break;
    case SMP_MESSAGE_RESUME_FAILED:
      OnError(PP_MEDIAPLAYERERROR_UNKNOWN, "Resume Failed.");
      break;
    case SMP_MESSAGE_SEEK_FAILED:
      OnError(PP_MEDIAPLAYERERROR_UNKNOWN, "Seek failed.");
      break;
    case SMP_MESSAGE_STOP_FAILED:
      OnError(PP_MEDIAPLAYERERROR_UNKNOWN, "Stop failed.");
      break;
    case SMP_MESSAGE_UNSUPPORTED_CONTAINER:
      OnError(PP_MEDIAPLAYERERROR_UNSUPPORTED_CONTAINER,
              "Unsupported container.");
      break;
    case SMP_MESSAGE_UNSUPPORTED_VIDEO_CODEC:
      OnError(PP_MEDIAPLAYERERROR_UNSUPPORTED_CODEC,
              "Unsupported video codec.");
      break;
    case SMP_MESSAGE_UNSUPPORTED_AUDIO_CODEC:
      OnError(PP_MEDIAPLAYERERROR_UNSUPPORTED_CODEC,
              "Unsupported audio codec.");
      break;
    case SMP_MESSAGE_CONNECTION_FAILED:
      OnError(PP_MEDIAPLAYERERROR_NETWORK, "Connection failed.");
      break;
    case SMP_MESSAGE_UNAUTHORIZED:
      OnError(PP_MEDIAPLAYERERROR_UNKNOWN, "Unauthorized.");
      break;

    default:
      LOG(WARNING) << "Unknown message: " << msg_id;
      break;
  }
}

// static
int32_t PepperSMPlayerAdapter::CallOnMessage(int msg_id,
                                             void* param,
                                             void* thiz) {
  // this function must return 0
  if (!thiz)
    return 0;
  static_cast<PepperSMPlayerAdapter*>(thiz)->OnMessage(msg_id, param);
  return 0;
}

}  // namespace content
