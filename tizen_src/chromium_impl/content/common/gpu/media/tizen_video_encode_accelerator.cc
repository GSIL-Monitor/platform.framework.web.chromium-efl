// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/media/tizen_video_encode_accelerator.h"

#include <stdio.h>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/timer/timer.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "media/base/bitstream_buffer.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"
#include "third_party/webrtc/common_video/libyuv/include/webrtc_libyuv.h"

using media::VideoFrame;

namespace content {

enum {
  INPUT_BUFFER_COUNT = 30,  // default input buffer counts of omx_h264enc
  // Max bitrate in bps
  MAX_BITRATE = 2000000,
  MAX_WIDTH = 1280,
  MAX_HEIGHT = 720
};

media_format_mimetype_e ConverToCapiFormat(media::VideoPixelFormat format) {
  switch (format) {
    case media::VideoPixelFormat::PIXEL_FORMAT_I420:
      return MEDIA_FORMAT_I420;
    case media::VideoPixelFormat::PIXEL_FORMAT_YV12:
      return MEDIA_FORMAT_YV12;
    case media::VideoPixelFormat::PIXEL_FORMAT_NV12:
      return MEDIA_FORMAT_NV12;
    default:
      return MEDIA_FORMAT_NATIVE_VIDEO;
  }
}

media::VideoEncodeAccelerator* CreateTizenVideoEncodeAccelerator() {
  return new TizenVideoEncodeAccelerator();
}

struct TizenVideoEncodeAccelerator::BitstreamBufferRef {
  BitstreamBufferRef(const scoped_refptr<media::VideoFrame>& frame, size_t size)
      : frame_(frame), size_(size), bytes_used_(0) {}

  scoped_refptr<media::VideoFrame> frame_;
  size_t size_;
  off_t bytes_used_;
};

struct TizenVideoEncodeAccelerator::Impl {
  Impl(media::VideoEncodeAccelerator::Client* client,
       scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : capi_thread_("CAPIEncoder"),
        enable_framedrop_(false),
        io_client_weak_factory_(client),
        task_runner_(task_runner),
        capi_bitrate_(0),
        codec_handle_(nullptr),
        format_handle_(nullptr),
        is_destroying_(false),
        can_feed_(true) {}

  ~Impl() {
    {
      base::AutoLock auto_lock(destroy_lock_);
      is_destroying_ = true;
    }

    if (capi_thread_.IsRunning())
      capi_thread_.Stop();

    if (codec_handle_) {
      mediacodec_unprepare(codec_handle_);
      mediacodec_unset_input_buffer_used_cb(codec_handle_);
      mediacodec_unset_output_buffer_available_cb(codec_handle_);
      mediacodec_unset_error_cb(codec_handle_);
      mediacodec_unset_buffer_status_cb(codec_handle_);
      mediacodec_destroy(codec_handle_);
    }

    if (format_handle_)
      media_format_unref(format_handle_);

    base::AutoLock auto_lock(output_queue_lock_);
    while (!encoder_output_queue_.empty()) {
      media::BitstreamBuffer bitstream_buffer = encoder_output_queue_.back();
      base::SharedMemory::CloseHandle(bitstream_buffer.handle());
      encoder_output_queue_.pop_back();
    }
  }

  void DeliverVideoFrame(media::ScopedMediaPacket pkt, bool key_frame);
  static void InputBufferUsedCB(media_packet_h pkt, void* user_data);
  static void BufferStatusCB(mediacodec_status_e status, void* user_data);
  static void OnMediaCodecErrorCB(mediacodec_error_e error, void* user_data);
  static void OutputBufferAvailableCB(media_packet_h pkt, void* user_data);

  base::Thread capi_thread_;
  bool enable_framedrop_;
  std::vector<media::BitstreamBuffer> encoder_output_queue_;
  base::WeakPtrFactory<media::VideoEncodeAccelerator::Client>
      io_client_weak_factory_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::Lock destroy_lock_;
  gfx::Size view_size_;
  uint32_t capi_bitrate_;
  base::Lock output_queue_lock_;
  mediacodec_h codec_handle_;
  media_format_h format_handle_;
  base::Lock can_feed_lock_;
  volatile bool is_destroying_;
  volatile bool can_feed_;
};

void TizenVideoEncodeAccelerator::Impl::DeliverVideoFrame(
    media::ScopedMediaPacket pkt,
    bool key_frame) {
  media::BitstreamBuffer* bs_buffer = nullptr;
  std::unique_ptr<base::SharedMemory> shm;
  enable_framedrop_ = false;
  {
    base::AutoLock auto_lock(output_queue_lock_);
    bs_buffer = &encoder_output_queue_.back();
    encoder_output_queue_.pop_back();
  }

  shm.reset(new base::SharedMemory(bs_buffer->handle(), false));
  if (!shm->Map(bs_buffer->size())) {
    LOG(INFO) << "Failed to map SHM";
    io_client_weak_factory_.GetWeakPtr()->NotifyError(
        media::VideoEncodeAccelerator::kPlatformFailureError);
    return;
  }

  void* buffer_data;
  uint64_t size;
  media_packet_get_buffer_size(pkt.get(), &size);
  media_packet_get_buffer_data_ptr(pkt.get(), &buffer_data);

  if (!buffer_data || size > bs_buffer->size()) {
    LOG(INFO) << "Encoded buff too large: " << size << ">"
              << shm->mapped_size();
    io_client_weak_factory_.GetWeakPtr()->NotifyError(
        media::VideoEncodeAccelerator::kPlatformFailureError);
  } else {
    // copying data to shared memory.
    memcpy(static_cast<uint8_t*>(shm->memory()), buffer_data, size);

    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&media::VideoEncodeAccelerator::Client::BitstreamBufferReady,
                   io_client_weak_factory_.GetWeakPtr(), bs_buffer->id(), size,
                   key_frame, base::Time::Now() - base::Time()));
  }
}

void TizenVideoEncodeAccelerator::Impl::InputBufferUsedCB(media_packet_h pkt,
                                                          void* user_data) {
  media_packet_destroy(pkt);
}

void TizenVideoEncodeAccelerator::Impl::BufferStatusCB(
    mediacodec_status_e status,
    void* user_data) {
  content::TizenVideoEncodeAccelerator::Impl* self =
      static_cast<TizenVideoEncodeAccelerator::Impl*>(user_data);
  base::AutoLock auto_lock(self->can_feed_lock_);
  self->can_feed_ = (status == MEDIACODEC_NEED_DATA);
}

void TizenVideoEncodeAccelerator::Impl::OnMediaCodecErrorCB(
    mediacodec_error_e error,
    void* user_data) {
  LOG(ERROR) << "OnMediaCodecErrorCB "
             << media::GetString(static_cast<mediacodec_error_e>(error));
}

void TizenVideoEncodeAccelerator::Impl::OutputBufferAvailableCB(
    media_packet_h pkt,
    void* user_data) {
  if (pkt == nullptr) {
    LOG(ERROR) << "There is no available media packet.";
    return;
  }

  content::TizenVideoEncodeAccelerator::Impl* self =
      static_cast<TizenVideoEncodeAccelerator::Impl*>(user_data);

  bool key_frame = false;
  media_packet_h out_pkt;

  mediacodec_get_output(self->codec_handle_, &out_pkt, 0);

  media_packet_is_sync_frame(out_pkt, &key_frame);
  media::ScopedMediaPacket packet_proxy(out_pkt);
  base::AutoLock auto_lock(self->output_queue_lock_);
  if (self->encoder_output_queue_.empty())
    return;
  self->capi_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&TizenVideoEncodeAccelerator::Impl::DeliverVideoFrame,
                 base::Unretained(self), base::Passed(&packet_proxy),
                 key_frame));
}

TizenVideoEncodeAccelerator::TizenVideoEncodeAccelerator() : impl_(nullptr) {}

TizenVideoEncodeAccelerator::~TizenVideoEncodeAccelerator() {
  if (impl_) {
    delete impl_;
    impl_ = nullptr;
  }
}

std::vector<media::VideoEncodeAccelerator::SupportedProfile>
TizenVideoEncodeAccelerator::GetSupportedProfiles() {
  std::vector<media::VideoEncodeAccelerator::SupportedProfile> profiles;
  media::VideoEncodeAccelerator::SupportedProfile profile;
  profile.profile = media::H264PROFILE_BASELINE;
  profile.max_resolution.SetSize(MAX_WIDTH, MAX_HEIGHT);
  profile.max_framerate_numerator = 30;
  profile.max_framerate_denominator = 1;
  profiles.push_back(profile);

  return profiles;
}

bool TizenVideoEncodeAccelerator::Initialize(
    media::VideoPixelFormat input_format,
    const gfx::Size& input_visible_size,
    media::VideoCodecProfile output_profile,
    uint32_t initial_bitrate,
    Client* client) {
  LOG(INFO) << " size :" << input_visible_size.ToString()
            << " max bitrate :" << MAX_BITRATE << "bps";
  DCHECK(impl_ == nullptr);
  if (media::H264PROFILE_MIN > output_profile ||
      media::H264PROFILE_MAX < output_profile) {
    NOTREACHED();
    return false;
  }

  impl_ = new Impl(client, base::ThreadTaskRunnerHandle::Get());
  impl_->capi_bitrate_ = initial_bitrate;
  impl_->view_size_ = input_visible_size;
  impl_->capi_thread_.Start();

  if (!StartEncoder()) {
    LOG(INFO) << "StartEncoder Fail";
    delete impl_;
    impl_ = nullptr;
    return false;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(
          &media::VideoEncodeAccelerator::Client::RequireBitstreamBuffers,
          impl_->io_client_weak_factory_.GetWeakPtr(),
          static_cast<unsigned int>(INPUT_BUFFER_COUNT), input_visible_size,
          MAX_BITRATE / 8));  // Maximum bytes for a frame by MAX_BITRATE.
  return true;
}

void TizenVideoEncodeAccelerator::Encode(
    const scoped_refptr<media::VideoFrame>& frame,
    bool force_keyframe) {
  size_t frame_size = VideoFrame::AllocationSize(
      media::VideoPixelFormat::PIXEL_FORMAT_I420, frame->coded_size());
  LOG(INFO) << " coded_size :" << frame->coded_size().ToString()
            << " natural_size :" << frame->natural_size().ToString()
            << " frame_size :" << frame_size;

  std::unique_ptr<BitstreamBufferRef> buffer_ref;
  BitstreamBufferRef* obj = new BitstreamBufferRef(frame, frame_size);
  if (!obj) {
    LOG(ERROR) << "BitstreamBufferRef allocation failed";
    return;
  }

  buffer_ref.reset(obj);

  base::AutoLock auto_lock(impl_->can_feed_lock_);
  if (impl_->can_feed_ && !impl_->is_destroying_) {
    impl_->capi_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE, base::Bind(&TizenVideoEncodeAccelerator::OnEncode,
                              base::Unretained(this), base::Passed(&buffer_ref),
                              force_keyframe));
  } else {
    LOG(INFO) << " [WEBRTC] . ENC FRAME DROP : feed " << impl_->can_feed_
              << " destroying " << impl_->is_destroying_;
  }
}

void TizenVideoEncodeAccelerator::UseOutputBitstreamBuffer(
    const media::BitstreamBuffer& buffer) {
  impl_->capi_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&TizenVideoEncodeAccelerator::OnUseOutputBitstreamBuffer,
                 base::Unretained(this), buffer));
}

void TizenVideoEncodeAccelerator::RequestEncodingParametersChange(
    uint32_t bitrate,
    uint32_t framerate) {
  if (bitrate > 0 && bitrate != impl_->capi_bitrate_) {
    base::AutoLock auto_lock(impl_->can_feed_lock_);
    impl_->capi_bitrate_ = bitrate;
    impl_->can_feed_ = false;
    media_format_set_video_avg_bps(impl_->format_handle_, bitrate);
    impl_->can_feed_ = true;
  }
}

void TizenVideoEncodeAccelerator::Destroy() {
  delete this;
}

void TizenVideoEncodeAccelerator::OnEncode(
    std::unique_ptr<BitstreamBufferRef> buffer_ref,
    bool force_keyframe) {
  void* buffer_data;
  int err;
  media_packet_h packet;
  if ((err = media_packet_create_alloc(impl_->format_handle_, nullptr, nullptr,
                                       &packet)) == MEDIA_PACKET_ERROR_NONE) {
    media_packet_get_buffer_data_ptr(packet, &buffer_data);

    int dst_stride_y = buffer_ref->frame_->stride(VideoFrame::kYPlane);
    uint8_t* dst_uv = (uint8_t*)buffer_data +
                      buffer_ref->frame_->stride(VideoFrame::kYPlane) *
                          buffer_ref->frame_->rows(VideoFrame::kYPlane);

    int dst_stride_uv = buffer_ref->frame_->stride(VideoFrame::kUPlane) * 2;

    libyuv::I420ToNV12(buffer_ref->frame_->data(VideoFrame::kYPlane),
                       buffer_ref->frame_->stride(VideoFrame::kYPlane),
                       buffer_ref->frame_->data(VideoFrame::kUPlane),
                       buffer_ref->frame_->stride(VideoFrame::kUPlane),
                       buffer_ref->frame_->data(VideoFrame::kVPlane),
                       buffer_ref->frame_->stride(VideoFrame::kVPlane),
                       (uint8_t*)buffer_data, dst_stride_y, dst_uv,
                       dst_stride_uv, impl_->view_size_.width(),
                       impl_->view_size_.height());
  } else {
    LOG(ERROR) << "media_packet_create() failed!"
               << media::GetString(static_cast<mediacodec_error_e>(err));
    return;
  }

  if (force_keyframe || impl_->enable_framedrop_) {
    LOG(INFO) << "force_keyframe = " << force_keyframe
              << " enable_framedrop_ = " << impl_->enable_framedrop_;
    if ((err = media_packet_set_flags(packet, MEDIA_PACKET_SYNC_FRAME)) !=
        MEDIA_PACKET_ERROR_NONE) {
      if (packet)
        media_packet_destroy(packet);
      LOG(ERROR) << "media_packet_set_flags() failed!"
                 << media::GetString(static_cast<mediacodec_error_e>(err));
      return;
    }
  }

  media_packet_set_buffer_size(packet, buffer_ref->size_);
  if ((err = mediacodec_process_input(impl_->codec_handle_, packet, 0)) !=
      MEDIACODEC_ERROR_NONE) {
    if (packet)
      media_packet_destroy(packet);
    LOG(ERROR) << "mediacodec_process_input() failed!"
               << media::GetString(static_cast<mediacodec_error_e>(err));
  }
}

void TizenVideoEncodeAccelerator::OnUseOutputBitstreamBuffer(
    const media::BitstreamBuffer& buffer) {
  base::AutoLock auto_lock(impl_->output_queue_lock_);
  impl_->encoder_output_queue_.push_back(buffer);

  LOG(INFO) << " output buffer is ready to use: " << buffer.id()
            << " out queue size: " << impl_->encoder_output_queue_.size();
}

bool TizenVideoEncodeAccelerator::StartEncoder() {
  int err = 0;
  if ((err = mediacodec_create(&impl_->codec_handle_)) !=
      MEDIACODEC_ERROR_NONE) {
    LOG(ERROR) << "mediacodec_create failed"
               << media::GetString(static_cast<mediacodec_error_e>(err));
    return false;
  }

  if ((err = mediacodec_set_codec(
           impl_->codec_handle_, MEDIACODEC_H264,
           MEDIACODEC_ENCODER | MEDIACODEC_SUPPORT_TYPE_HW)) !=
      MEDIACODEC_ERROR_NONE) {
    LOG(ERROR) << "mediacodec_set_codec failed error"
               << media::GetString(static_cast<mediacodec_error_e>(err));
    return false;
  }

  if ((err = mediacodec_set_venc_info(
           impl_->codec_handle_, impl_->view_size_.width(),
           impl_->view_size_.height(), 30, 0)) != MEDIACODEC_ERROR_NONE) {
    LOG(ERROR) << "mediacodec_set_venc_info failed error"
               << media::GetString(static_cast<mediacodec_error_e>(err));
    return false;
  }
  mediacodec_set_input_buffer_used_cb(impl_->codec_handle_,
                                      Impl::InputBufferUsedCB, impl_);
  mediacodec_set_output_buffer_available_cb(
      impl_->codec_handle_, Impl::OutputBufferAvailableCB, impl_);
  mediacodec_set_error_cb(impl_->codec_handle_, Impl::OnMediaCodecErrorCB,
                          impl_);
  mediacodec_set_buffer_status_cb(impl_->codec_handle_, Impl::BufferStatusCB,
                                  impl_);
  if ((err = media_format_create(&impl_->format_handle_)) ==
      MEDIA_FORMAT_ERROR_NONE) {
    LOG(INFO) << "media_format_create success";
    if ((err = media_format_set_video_mime(
             impl_->format_handle_,
             ConverToCapiFormat(media::VideoPixelFormat::PIXEL_FORMAT_NV12))) !=
        MEDIA_FORMAT_ERROR_NONE) {
      LOG(ERROR) << "media_format_set_video_mime failed!"
                 << media::GetString(static_cast<mediacodec_error_e>(err));
      return false;
    }
    media_format_set_video_width(impl_->format_handle_,
                                 impl_->view_size_.width());
    media_format_set_video_height(impl_->format_handle_,
                                  impl_->view_size_.height());
    media_format_set_video_avg_bps(impl_->format_handle_, impl_->capi_bitrate_);
    media_format_set_video_max_bps(impl_->format_handle_, impl_->capi_bitrate_);
  } else {
    LOG(ERROR) << "media_format_create() failed!"
               << media::GetString(static_cast<mediacodec_error_e>(err));
    return false;
  }

  if ((err = mediacodec_prepare(impl_->codec_handle_)) !=
      MEDIACODEC_ERROR_NONE) {
    LOG(ERROR) << "mediacodec_prepare failed error"
               << media::GetString(static_cast<mediacodec_error_e>(err));
    return false;
  }
  return true;
}

}  // namespace content
