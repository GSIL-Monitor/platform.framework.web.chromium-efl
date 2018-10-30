// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/media/tizen_video_decode_accelerator.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "base/bind.h"
#include "base/memory/shared_memory.h"
#include "base/process/process.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "third_party/libyuv/include/libyuv/planar_functions.h"
#include "third_party/webrtc/common_video/h264/sps_parser.h"

namespace content {

enum {
  MAX_BITRATE = 2000000,  // bps.
  MAX_WIDTH = 1920,
  MAX_HEIGHT = 1080,
  INPUT_BUFFER_SIZE = MAX_BITRATE / 8,  // bytes. 1 sec for H.264 HD video.
  ID_LAST = 0x3FFFFFFF,                 // wrap round ID after this
};

media::VideoDecodeAccelerator* CreateTizenVideoDecodeAccelerator() {
  return new TizenVideoDecodeAccelerator();
}

media::VideoDecodeAccelerator::SupportedProfiles GetSupportedTizenProfiles() {
  media::VideoDecodeAccelerator::SupportedProfiles profiles;
  media::VideoDecodeAccelerator::SupportedProfile profile;
  profile.profile = media::H264PROFILE_MAIN;
  profile.min_resolution.SetSize(0, 0);
  profile.max_resolution.SetSize(MAX_WIDTH, MAX_HEIGHT);  // FHD
  profiles.push_back(profile);

  return profiles;
}

struct TizenVideoDecodeAccelerator::BitstreamBufferRef {
  BitstreamBufferRef(
      base::WeakPtr<media::VideoDecodeAccelerator::Client> client,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      base::SharedMemory* shm,
      size_t size,
      int32 input_id,
      base::TimeDelta timestamp)
      : client_(client),
        task_runner_(task_runner),
        shm_(shm),
        size_(size),
        bytes_used_(0),
        input_id_(input_id),
        timestamp_(timestamp) {}

  ~BitstreamBufferRef() {
    if (input_id_ >= 0) {
      task_runner_->PostTask(FROM_HERE,
                             base::Bind(&media::VideoDecodeAccelerator::Client::
                                            NotifyEndOfBitstreamBuffer,
                                        client_, input_id_));
    }
  }

  base::WeakPtr<media::VideoDecodeAccelerator::Client> client_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<base::SharedMemory> shm_;
  size_t size_;
  off_t bytes_used_;
  int32 input_id_;
  base::TimeDelta timestamp_;
};

struct TizenVideoDecodeAccelerator::Impl {
  Impl()
      : codec_handle_(nullptr),
        format_handle_(nullptr),
        frame_count_(0),
        task_runner_(base::ThreadTaskRunnerHandle::Get()),
        capi_thread_("CAPIDecoder"),
        can_feed_(true),
        is_destroying_(false),
        media_frame_height_(0),
        media_frame_width_(0) {}

  ~Impl() {
    is_destroying_ = true;
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
  }

  void DeliverVideoFrame(media::ScopedMediaPacket pkt);
  static void InputBufferUsedCB(media_packet_h pkt, void* user_data);
  static void BufferStatusCB(mediacodec_status_e status, void* user_data);
  static void OnMediaCodecErrorCB(mediacodec_error_e error, void* user_data);
  static void OutputBufferAvailableCB(media_packet_h pkt, void* user_data);

 public:
  mediacodec_h codec_handle_;
  media_format_h format_handle_;
  int frame_count_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<base::WeakPtrFactory<Client> > io_client_weak_factory_;
  std::map<uint64_t, std::unique_ptr<BitstreamBufferRef>> buffer_id_map_;
  base::Thread capi_thread_;
  base::Lock can_feed_lock_;
  volatile bool can_feed_;
  volatile bool is_destroying_;
  uint16_t media_frame_height_;
  uint16_t media_frame_width_;
};

void TizenVideoDecodeAccelerator::Impl::DeliverVideoFrame(
    media::ScopedMediaPacket pkt) {
  if (!pkt) {
    LOG(ERROR) << "pkt is null.";
    return;
  }

  base::SharedMemory shared_memory;
  base::SharedMemoryHandle shared_memory_handle;
  uint64_t size;
  void* buffer_data;
  int32 bitstream_buffer_id;
  uint64_t timestamp;

  media_packet_get_pts(pkt.get(), &timestamp);
  auto result = buffer_id_map_.find(timestamp);
  if (result == buffer_id_map_.end()) {
    LOG(ERROR) << __func__
               << " Not Found buffer_id of timestamp : " << timestamp;
    return;
  }
  std::unique_ptr<BitstreamBufferRef> buffer_ref = std::move(result->second);
  bitstream_buffer_id = buffer_ref->input_id_;
  buffer_id_map_.erase(result);

  media_packet_get_buffer_size(pkt.get(), &size);
  media_packet_get_buffer_data_ptr(pkt.get(), &buffer_data);

  media_format_h fmt = nullptr;
  media_packet_get_format(pkt.get(), &fmt);
  media::ScopedMediaFormat format(fmt);

  media_format_mimetype_e mime;
  int width, height, avg_bps, max_bps;

  if (media_format_get_video_info(format.get(), &mime, &width, &height,
                                  &avg_bps,
                                  &max_bps) == MEDIA_FORMAT_ERROR_NONE) {
    LOG(INFO) << "media_format_get_video_info success! width = " << width
              << ", height = " << height;
  } else {
    LOG(ERROR) << "media_format_get_video_info failed.";
    return;
  }

  if (!shared_memory.CreateAndMapAnonymous(size)) {
    LOG(ERROR) << "Shared Memory creation failed.";
  } else {
    shared_memory_handle = shared_memory.handle().Duplicate();

    if (!shared_memory_handle.IsValid()) {
      LOG(ERROR) << "Couldn't duplicate shared memory handle";
    } else {
      gfx::Rect frame_size = gfx::Rect(width, height);
      int dest_stride_y = width;
      int dest_stride_uv = (dest_stride_y + 1) / 2;

      uint8_t* y_plane = static_cast<uint8_t*>(shared_memory.memory());
      uint8_t* u_plane = y_plane + dest_stride_y * height;
      uint8_t* v_plane = u_plane + dest_stride_uv * height / 2;
      uint8_t* data_uv = (uint8_t*)buffer_data + dest_stride_y * height;

      libyuv::NV12ToI420((uint8_t*)buffer_data, dest_stride_y, data_uv,
                         dest_stride_y, y_plane, dest_stride_y, u_plane,
                         dest_stride_uv, v_plane, dest_stride_uv, width,
                         height);

      task_runner_->PostTask(
          FROM_HERE,
          base::Bind(&media::VideoDecodeAccelerator::Client::NotifyDecodeDone,
                     io_client_weak_factory_->GetWeakPtr(),
                     shared_memory_handle, bitstream_buffer_id, size,
                     frame_size));
    }
  }
}

void TizenVideoDecodeAccelerator::Impl::InputBufferUsedCB(media_packet_h pkt,
                                                          void* user_data) {
  media_packet_destroy(pkt);
}

void TizenVideoDecodeAccelerator::Impl::BufferStatusCB(
    mediacodec_status_e status,
    void* user_data) {
  content::TizenVideoDecodeAccelerator::Impl* self =
      static_cast<TizenVideoDecodeAccelerator::Impl*>(user_data);
  base::AutoLock auto_lock(self->can_feed_lock_);
  self->can_feed_ = (status == MEDIACODEC_NEED_DATA);
}

void TizenVideoDecodeAccelerator::Impl::OnMediaCodecErrorCB(
    mediacodec_error_e error,
    void* user_data) {
  LOG(ERROR) << media::GetString(static_cast<mediacodec_error_e>(error));
}

void TizenVideoDecodeAccelerator::Impl::OutputBufferAvailableCB(
    media_packet_h pkt,
    void* user_data) {
  if (pkt == nullptr) {
    LOG(ERROR) << "There is no available media packet.";
    return;
  }

  content::TizenVideoDecodeAccelerator::Impl* self =
      static_cast<TizenVideoDecodeAccelerator::Impl*>(user_data);

  media_packet_h out_pkt;
  mediacodec_get_output(self->codec_handle_, &out_pkt, 0);

  media::ScopedMediaPacket packet_proxy(out_pkt);
  self->capi_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&TizenVideoDecodeAccelerator::Impl::DeliverVideoFrame,
                 base::Unretained(self), base::Passed(&packet_proxy)));
}

TizenVideoDecodeAccelerator::TizenVideoDecodeAccelerator() : impl_(nullptr) {}

TizenVideoDecodeAccelerator::~TizenVideoDecodeAccelerator() {
  if (impl_ != nullptr) {
    delete impl_;
    impl_ = nullptr;
  }
}

bool TizenVideoDecodeAccelerator::Initialize(const Config& config,
                                             Client* client) {
  switch (config.profile) {
    case media::H264PROFILE_BASELINE:
      LOG(INFO) << "Initialize(): profile -> H264PROFILE_BASELINE";
      break;
    case media::H264PROFILE_MAIN:
      LOG(INFO) << "Initialize(): profile -> H264PROFILE_MAIN";
      break;
    default:
      LOG(ERROR) << "Initialize(): unsupported profile=" << config.profile;
      return false;
  };

  CHECK(impl_ == nullptr);
  impl_ = new Impl();
  impl_->io_client_weak_factory_.reset(
      new base::WeakPtrFactory<Client>(client));

  if (!impl_->capi_thread_.Start()) {
    LOG(ERROR) << " capi_thread_ failed to start";
    delete impl_;
    impl_ = nullptr;
    return false;
  }
  impl_->media_frame_height_ = config.initial_expected_coded_size.height();
  impl_->media_frame_width_ = config.initial_expected_coded_size.width();

  impl_->capi_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE, base::Bind(&TizenVideoDecodeAccelerator::StartDecoder,
                            base::Unretained(this)));
  return true;
}

void TizenVideoDecodeAccelerator::Decode(
    const media::BitstreamBuffer& bitstream_buffer) {
  if (bitstream_buffer.id() < 0) {
    LOG(ERROR) << "Invalid bitstream_buffer, id: " << bitstream_buffer.id();
    if (base::SharedMemory::IsHandleValid(bitstream_buffer.handle()))
      base::SharedMemory::CloseHandle(bitstream_buffer.handle());
    return;
  }

  std::unique_ptr<BitstreamBufferRef> buffer_ref;
  std::unique_ptr<base::SharedMemory> shm(
      new base::SharedMemory(bitstream_buffer.handle(), true));

  // Skip empty buffer.
  if (!bitstream_buffer.size())
    return;

  if (!shm->Map(bitstream_buffer.size())) {
    LOG(ERROR) << " could not map bitstream_buffer";
    NotifyError(media::VideoDecodeAccelerator::UNREADABLE_INPUT);
    return;
  }

  buffer_ref.reset(
      new BitstreamBufferRef(impl_->io_client_weak_factory_->GetWeakPtr(),
                             base::ThreadTaskRunnerHandle::Get(), shm.release(),
                             bitstream_buffer.size(), bitstream_buffer.id(),
                             bitstream_buffer.presentation_timestamp()));

  if (!buffer_ref) {
    return;
  }

  base::AutoLock auto_lock(impl_->can_feed_lock_);
  if (impl_->can_feed_ && !impl_->is_destroying_) {
    impl_->capi_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&TizenVideoDecodeAccelerator::OnDecode,
                   base::Unretained(this), base::Passed(&buffer_ref)));
  } else {
    LOG(INFO) << " Frame drop on decoder: feed " << impl_->can_feed_
              << " is_destroying_ " << impl_->is_destroying_;
  }
}

void TizenVideoDecodeAccelerator::AssignPictureBuffers(
    const std::vector<media::PictureBuffer>& buffers) {
  NOTIMPLEMENTED();
}

void TizenVideoDecodeAccelerator::ReusePictureBuffer(int32 picture_buffer_id) {
  NOTIMPLEMENTED();
}

void TizenVideoDecodeAccelerator::Flush() {
  FlushInternal();
  impl_->io_client_weak_factory_->GetWeakPtr()->NotifyFlushDone();
}

void TizenVideoDecodeAccelerator::Reset() {
  FlushInternal();
  impl_->io_client_weak_factory_->GetWeakPtr()->NotifyResetDone();
}

void TizenVideoDecodeAccelerator::FlushInternal() {
  mediacodec_flush_buffers(impl_->codec_handle_);
  impl_->buffer_id_map_.clear();
}

void TizenVideoDecodeAccelerator::Destroy() {
  delete this;
}

bool TizenVideoDecodeAccelerator::TryToSetupDecodeOnSeparateThread(
    const base::WeakPtr<Client>& decode_client,
    const scoped_refptr<base::SingleThreadTaskRunner>& decode_task_runner) {
  return false;
}

void TizenVideoDecodeAccelerator::StartDecoder() {
  int err = 0;

  if ((err = mediacodec_create(&impl_->codec_handle_)) !=
      MEDIACODEC_ERROR_NONE) {
    LOG(ERROR) << "mediacodec_create failed"
               << media::GetString(static_cast<mediacodec_error_e>(err));
    return;
  }

  if ((err = mediacodec_set_codec(
           impl_->codec_handle_, MEDIACODEC_H264,
           MEDIACODEC_DECODER | MEDIACODEC_SUPPORT_TYPE_HW)) !=
      MEDIACODEC_ERROR_NONE) {
    LOG(ERROR) << "mediacodec_set_codec failed error"
               << media::GetString(static_cast<mediacodec_error_e>(err));
    return;
  }

  if ((err = mediacodec_set_vdec_info(
           impl_->codec_handle_, impl_->media_frame_width_,
           impl_->media_frame_height_)) != MEDIACODEC_ERROR_NONE) {
    LOG(ERROR) << "mediacodec_set_vdec_info failed error"
               << media::GetString(static_cast<mediacodec_error_e>(err));
    return;
  }

  mediacodec_set_input_buffer_used_cb(impl_->codec_handle_,
                                      Impl::InputBufferUsedCB, impl_);
  mediacodec_set_output_buffer_available_cb(
      impl_->codec_handle_, Impl::OutputBufferAvailableCB, impl_);
  mediacodec_set_buffer_status_cb(impl_->codec_handle_, Impl::BufferStatusCB,
                                  impl_);
  mediacodec_set_error_cb(impl_->codec_handle_, Impl::OnMediaCodecErrorCB,
                          impl_);
}

void TizenVideoDecodeAccelerator::OnDecode(
    std::unique_ptr<BitstreamBufferRef> buffer_ref) {
  int err;

  uint8_t* buffer = static_cast<uint8_t*>(buffer_ref->shm_->memory());
  if (!buffer) {
    LOG(ERROR) << "empty decode frame buffer!";
    return;
  }

  LOG(INFO) << "To Decode Frame Size = " << buffer_ref->size_
            << " , timestamp : " << buffer_ref->timestamp_
            << " , input_id : " << buffer_ref->input_id_;

  if (NeedsUpdateFormat(buffer, buffer_ref->size_)) {
    if (impl_->codec_handle_ != nullptr)
      mediacodec_unprepare(impl_->codec_handle_);

    if ((err = mediacodec_set_vdec_info(
             impl_->codec_handle_, impl_->media_frame_width_,
             impl_->media_frame_height_)) != MEDIACODEC_ERROR_NONE) {
      LOG(ERROR) << "mediacodec_set_vdec_info failed error"
                 << media::GetString(static_cast<mediacodec_error_e>(err));
      return;
    }

    // set width and height
    if (impl_->format_handle_ != nullptr)
      media_format_unref(impl_->format_handle_);

    if ((err = media_format_create(&impl_->format_handle_)) !=
        MEDIA_FORMAT_ERROR_NONE) {
      LOG(ERROR) << "media_format_create() failed!"
                 << media::GetString(static_cast<mediacodec_error_e>(err));
      return;
    }

    if ((err = media_format_set_video_mime(impl_->format_handle_,
                                           MEDIA_FORMAT_H264_SP)) !=
        MEDIA_FORMAT_ERROR_NONE) {
      LOG(ERROR) << "media_format_set_video_mime failed!"
                 << media::GetString(static_cast<mediacodec_error_e>(err));
      return;
    }

    if ((err = media_format_set_video_width(impl_->format_handle_,
                                            impl_->media_frame_width_)) !=
        MEDIA_FORMAT_ERROR_NONE) {
      LOG(ERROR) << "media_format_set_video_width failed!"
                 << media::GetString(static_cast<mediacodec_error_e>(err));
      return;
    }

    if ((err = media_format_set_video_height(impl_->format_handle_,
                                             impl_->media_frame_height_)) !=
        MEDIA_FORMAT_ERROR_NONE) {
      LOG(ERROR) << "media_format_set_video_height failed!"
                 << media::GetString(static_cast<mediacodec_error_e>(err));
      return;
    }

    if ((err = mediacodec_prepare(impl_->codec_handle_)) !=
        MEDIACODEC_ERROR_NONE) {
      LOG(ERROR) << "mediacodec_prepare failed error"
                 << media::GetString(static_cast<mediacodec_error_e>(err));
      return;
    }
  }

  media_packet_h packet;
  if ((err = media_packet_create_alloc(impl_->format_handle_, nullptr, nullptr,
                                       &packet)) != MEDIA_PACKET_ERROR_NONE) {
    LOG(ERROR) << "media_packet_create_alloc() failed!"
               << media::GetString(static_cast<mediacodec_error_e>(err));
    return;
  }

  media_packet_set_flags(packet, !impl_->frame_count_
                                     ? MEDIA_PACKET_CODEC_CONFIG
                                     : MEDIA_PACKET_SYNC_FRAME);
  impl_->frame_count_++;

  void* buffer_data;
  uint64_t timestamp = buffer_ref->timestamp_.ToInternalValue();
  media_packet_get_buffer_data_ptr(packet, &buffer_data);
  memcpy(buffer_data, buffer, buffer_ref->size_);
  buffer_ref->shm_.reset();
  media_packet_set_buffer_size(packet, buffer_ref->size_);
  media_packet_set_pts(packet, timestamp);
  impl_->buffer_id_map_.insert(
      std::make_pair(timestamp, std::move(buffer_ref)));
  if ((err = mediacodec_process_input(impl_->codec_handle_, packet, 0)) !=
      MEDIACODEC_ERROR_NONE) {
    if (packet)
      media_packet_destroy(packet);
    LOG(ERROR) << "mediacodec_process_input() failed!"
               << media::GetString(static_cast<mediacodec_error_e>(err));
  }
}

void TizenVideoDecodeAccelerator::NotifyError(
    media::VideoDecodeAccelerator::Error error) {
  if (impl_->io_client_weak_factory_->GetWeakPtr()) {
    impl_->io_client_weak_factory_->GetWeakPtr()->NotifyError(error);
  }
}

bool TizenVideoDecodeAccelerator::NeedsUpdateFormat(uint8_t* data, int length) {
  const int SEQUENCE_PARAMETER_SET = 7;
  const int START_CODE_LENGTH = 4;
  const int NAL_UNIT_HEADER_LENGTH = 1;

  uint8_t* buffer = data;
  int nal_unit_type = *(buffer + START_CODE_LENGTH) & 0x1F;

  if (nal_unit_type == SEQUENCE_PARAMETER_SET) {
    // SPS Parsing
    rtc::Optional<webrtc::SpsParser::SpsState> sps =
        webrtc::SpsParser::ParseSps(
            (uint8_t*)buffer + START_CODE_LENGTH + NAL_UNIT_HEADER_LENGTH,
            length);

    if (sps && (impl_->media_frame_width_ != sps->width ||
                impl_->media_frame_height_ != sps->height)) {
      LOG(INFO) << "Success : width : " << sps->width
                << " height : " << sps->height;
      impl_->media_frame_width_ = sps->width;
      impl_->media_frame_height_ = sps->height;
      return true;
    }
  }

  if (!impl_->format_handle_)
    return true;

  return false;
}

}  // namespace content
