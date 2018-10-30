// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/libav_video_decoder_tizen.h"

#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/limits.h"
#include "media/base/media_switches.h"
#include "media/base/video_codecs.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/base/video_types.h"
#include "media/base/video_util.h"

extern "C" {
#define FF_API_GET_BUFFER 1
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}  // extern "C"

namespace {
int LockManagerOperation(void** lock, enum AVLockOp op) {
  switch (op) {
    case AV_LOCK_CREATE:
      *lock = new base::Lock();
      return 0;

    case AV_LOCK_OBTAIN:
      static_cast<base::Lock*>(*lock)->Acquire();
      return 0;

    case AV_LOCK_RELEASE:
      static_cast<base::Lock*>(*lock)->Release();
      return 0;

    case AV_LOCK_DESTROY:
      delete static_cast<base::Lock*>(*lock);
      *lock = nullptr;
      return 0;
  }
  return 1;
}

// Libav must only be initialized once, so use a LazyInstance to ensure this.
class LibavInitializer {
 public:
  bool initialized() { return initialized_; }

 private:
  friend struct base::LazyInstanceTraitsBase<LibavInitializer>;

  LibavInitializer() : initialized_(false) {
    // Before doing anything disable logging as it interferes with layout tests.
    av_log_set_level(AV_LOG_QUIET);

    // Register our protocol glue code with Libav.
    if (av_lockmgr_register(&LockManagerOperation) != 0)
      return;

    // Now register the rest of Libav.
    av_register_all();

    initialized_ = true;
  }

  ~LibavInitializer() { NOTREACHED() << "LibavInitializer should be leaky!"; }

  bool initialized_;

  DISALLOW_COPY_AND_ASSIGN(LibavInitializer);
};

void InitializeLibav() {
  static base::LazyInstance<LibavInitializer>::Leaky li =
      LAZY_INSTANCE_INITIALIZER;
  CHECK(li.Get().initialized());
}

}  // namespace

namespace media {

namespace {
AVCodecID VideoCodecToCodecID(VideoCodec video_codec) {
  switch (video_codec) {
    case kCodecH264:
      return AV_CODEC_ID_H264;
#if BUILDFLAG(ENABLE_HEVC_DEMUXING)
    case kCodecHEVC:
      return AV_CODEC_ID_HEVC;
#endif
    case kCodecTheora:
      return AV_CODEC_ID_THEORA;
    case kCodecMPEG4:
      return AV_CODEC_ID_MPEG4;
    case kCodecVP8:
      return AV_CODEC_ID_VP8;
    case kCodecVP9:
      return AV_CODEC_ID_VP9;
    default:
      LOG(ERROR) << "Unknown VideoCodec: " << video_codec;
  }
  return AV_CODEC_ID_NONE;
}

VideoPixelFormat AVPixelFormatToVideoPixelFormat(AVPixelFormat pixel_format) {
  // The YUVJ alternatives are FFmpeg's (deprecated, but still in use) way to
  // specify a pixel format and full range color combination.
  switch (pixel_format) {
    case AV_PIX_FMT_YUV422P:
    case AV_PIX_FMT_YUVJ422P:
      return PIXEL_FORMAT_YV16;
    case AV_PIX_FMT_YUV444P:
    case AV_PIX_FMT_YUVJ444P:
      return PIXEL_FORMAT_YV24;
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
      return PIXEL_FORMAT_YV12;
    case AV_PIX_FMT_YUVA420P:
      return PIXEL_FORMAT_YV12A;
    default:
      LOG(ERROR) << "Unsupported AVPixelFormat: " << pixel_format;
  }
  return PIXEL_FORMAT_UNKNOWN;
}

AVPixelFormat VideoPixelFormatToAVPixelFormat(VideoPixelFormat video_format) {
  switch (video_format) {
    case PIXEL_FORMAT_YV16:
      return AV_PIX_FMT_YUV422P;
    case PIXEL_FORMAT_YV12:
      return AV_PIX_FMT_YUV420P;
    case PIXEL_FORMAT_YV12A:
      return AV_PIX_FMT_YUVA420P;
    case PIXEL_FORMAT_YV24:
      return AV_PIX_FMT_YUV444P;
    default:
      LOG(ERROR) << "Unsupported Format: " << video_format;
  }
  return AV_PIX_FMT_NONE;
}

int VideoCodecProfileToProfileID(VideoCodecProfile profile) {
  switch (profile) {
    case H264PROFILE_BASELINE:
      return FF_PROFILE_H264_BASELINE;
    case H264PROFILE_MAIN:
      return FF_PROFILE_H264_MAIN;
    case H264PROFILE_EXTENDED:
      return FF_PROFILE_H264_EXTENDED;
    case H264PROFILE_HIGH:
      return FF_PROFILE_H264_HIGH;
    case H264PROFILE_HIGH10PROFILE:
      return FF_PROFILE_H264_HIGH_10;
    case H264PROFILE_HIGH422PROFILE:
      return FF_PROFILE_H264_HIGH_422;
    case H264PROFILE_HIGH444PREDICTIVEPROFILE:
      return FF_PROFILE_H264_HIGH_444_PREDICTIVE;
    default:
      LOG(ERROR) << "Unknown VideoCodecProfile: " << profile;
  }
  return FF_PROFILE_UNKNOWN;
}

bool VideoDecoderConfigToAVCodecContext(const VideoDecoderConfig& config,
                                        AVCodecContext* codec_context) {
  codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_context->codec_id = VideoCodecToCodecID(config.codec());
  codec_context->profile = VideoCodecProfileToProfileID(config.profile());
  codec_context->coded_width = config.coded_size().width();
  codec_context->coded_height = config.coded_size().height();
  codec_context->pix_fmt = VideoPixelFormatToAVPixelFormat(config.format());
  if (config.color_space() == COLOR_SPACE_JPEG)
    codec_context->color_range = AVCOL_RANGE_JPEG;

  if (config.extra_data().empty()) {
    codec_context->extradata = nullptr;
    codec_context->extradata_size = 0;
  } else {
    codec_context->extradata_size = config.extra_data().size();
    codec_context->extradata = reinterpret_cast<uint8_t*>(
        av_malloc(config.extra_data().size() + FF_INPUT_BUFFER_PADDING_SIZE));

    if (!codec_context->extradata) {
      LOG(ERROR) << "Failed to allocate memory for extradata";
      return false;
    }

    memcpy(codec_context->extradata, &config.extra_data()[0],
           config.extra_data().size());
    memset(codec_context->extradata + config.extra_data().size(), '\0',
           FF_INPUT_BUFFER_PADDING_SIZE);
  }
  return true;
}

}  // namespace

inline void ScopedPtrTizenAVFreeContext::operator()(void* x) const {
  AVCodecContext* codec_context = static_cast<AVCodecContext*>(x);
  avcodec_free_context(&codec_context);
}

inline void ScopedPtrTizenAVFreeFrame::operator()(void* x) const {
  AVFrame* frame = static_cast<AVFrame*>(x);
  av_frame_free(&frame);
}

// Always try to use three threads for video decoding.  There is little reason
// not to since current day CPUs tend to be multi-core and we measured
// performance benefits on older machines such as P4s with hyperthreading.
//
// Handling decoding on separate threads also frees up the pipeline thread to
// continue processing. Although it'd be nice to have the option of a single
// decoding thread, Libav treats having one thread the same as having zero
// threads (i.e., avcodec_decode_video() will execute on the calling thread).
// Yet another reason for having two threads :)
static const int kDecodeThreads = 2;
static const int kMaxDecodeThreads = 16;

// Returns the number of threads given the Libav CodecID. Also inspects the
// command line for a valid --video-threads flag.
static int GetThreadCount(AVCodecID codec_id) {
  // Refer to http://crbug.com/93932 for tsan suppressions on decoding.
  int decode_threads = kDecodeThreads;

  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  std::string threads(cmd_line->GetSwitchValueASCII(switches::kVideoThreads));
  if (threads.empty() || !base::StringToInt(threads, &decode_threads))
    return decode_threads;

  decode_threads = std::max(decode_threads, 0);
  decode_threads = std::min(decode_threads, kMaxDecodeThreads);
  return decode_threads;
}

static int GetVideoBufferImpl(struct AVCodecContext* s,
                              AVFrame* frame,
                              int flags) {
  LibavVideoDecoderTizen* decoder =
      static_cast<LibavVideoDecoderTizen*>(s->opaque);
  return decoder->GetVideoBuffer(s, frame, flags);
}

static void ReleaseVideoBufferImpl(void* opaque, uint8_t* data) {
  if (opaque)
    static_cast<VideoFrame*>(opaque)->Release();
}

// static
bool LibavVideoDecoderTizen::IsCodecSupported(VideoCodec codec) {
  InitializeLibav();
  return avcodec_find_decoder(VideoCodecToCodecID(codec)) != nullptr;
}

LibavVideoDecoderTizen::LibavVideoDecoderTizen(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : task_runner_(task_runner), state_(kUninitialized), decode_nalus_(false) {}

int LibavVideoDecoderTizen::GetVideoBuffer(struct AVCodecContext* codec_context,
                                           AVFrame* frame,
                                           int flags) {
  // Don't use |codec_context_| here! With threaded decoding,
  // it will contain unsynchronized width/height/pix_fmt values,
  // whereas |codec_context| contains the current threads's
  // updated width/height/pix_fmt, which can change for adaptive
  // content.
  const VideoPixelFormat format =
      AVPixelFormatToVideoPixelFormat(codec_context->pix_fmt);
  if (format == PIXEL_FORMAT_UNKNOWN) {
    LOG(ERROR) << "Unknown pixel format";
    return AVERROR(EINVAL);
  }

  gfx::Size size(codec_context->width, codec_context->height);
  const int ret = av_image_check_size(size.width(), size.height(), 0, NULL);
  if (ret < 0) {
    LOG(ERROR) << "Wrong image size. Width: " << size.width()
               << ", height: " << size.height();
    return ret;
  }

  gfx::Size natural_size;
  if (codec_context->sample_aspect_ratio.num > 0) {
    natural_size = GetNaturalSize(size, codec_context->sample_aspect_ratio.num,
                                  codec_context->sample_aspect_ratio.den);
  } else {
    natural_size = config_.natural_size();
  }

  // Libav has specific requirements on the allocation size of the frame.  The
  // following logic replicates Libav's allocation strategy to ensure buffers
  // are not overread / overwritten.  See ff_init_buffer_info() for details.
  //
  // When lowres is non-zero, dimensions should be divided by 2^(lowres), but
  // since we don't use this, just DCHECK that it's zero.
  DCHECK_EQ(codec_context->lowres, 0);
  gfx::Size coded_size(std::max(size.width(), codec_context->coded_width),
                       std::max(size.height(), codec_context->coded_height));

  if (!VideoFrame::IsValidConfig(format, VideoFrame::STORAGE_UNKNOWN,
                                 coded_size, gfx::Rect(size), natural_size)) {
    LOG(ERROR) << "Invalid config";
    return AVERROR(EINVAL);
  }

  scoped_refptr<VideoFrame> video_frame = frame_pool_.CreateFrame(
      format, coded_size, gfx::Rect(size), natural_size, kNoTimestamp);

  if (!video_frame) {
    LOG(ERROR) << "Video frame is empty";
    return AVERROR(EINVAL);
  }

  for (int i = 0; i < 3; i++) {
    frame->data[i] = video_frame->data(i);
    frame->linesize[i] = video_frame->stride(i);
  }

  frame->width = coded_size.width();
  frame->height = coded_size.height();
  frame->format = codec_context->pix_fmt;
  frame->reordered_opaque = codec_context->reordered_opaque;

  // Now create an AVBufferRef for the data just allocated. It will own the
  // reference to the VideoFrame object.
  VideoFrame* opaque = video_frame.get();
  opaque->AddRef();
  frame->buf[0] = av_buffer_create(
      frame->data[0], VideoFrame::AllocationSize(format, coded_size),
      ReleaseVideoBufferImpl, opaque, 0);

  return 0;
}

std::string LibavVideoDecoderTizen::GetDisplayName() const {
  return "LibavVideoDecoderTizen";
}

void LibavVideoDecoderTizen::Initialize(const VideoDecoderConfig& config,
                                        bool low_delay,
                                        CdmContext* cdm_context,
                                        const InitCB& init_cb,
                                        const OutputCB& output_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!config.is_encrypted());
  DCHECK(!output_cb.is_null());

  InitializeLibav();

  config_ = config;
  InitCB bound_init_cb = BindToCurrentLoop(init_cb);

  if (!config.IsValidConfig() || !ConfigureDecoder(low_delay)) {
    bound_init_cb.Run(false);
    LOG(ERROR) << "TizenLibavVidoeDecoder initialization failed";
    return;
  }

  output_cb_ = BindToCurrentLoop(output_cb);

  // Success!
  state_ = kNormal;
  bound_init_cb.Run(true);
}

void LibavVideoDecoderTizen::Decode(const scoped_refptr<DecoderBuffer>& buffer,
                                    const DecodeCB& decode_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(buffer);
  DCHECK(!decode_cb.is_null());
  CHECK_NE(state_, kUninitialized);

  DecodeCB decode_cb_bound = BindToCurrentLoop(decode_cb);

  if (state_ == kError) {
    decode_cb_bound.Run(DecodeStatus::DECODE_ERROR);
    LOG(ERROR) << "Decode error";
    return;
  }

  if (state_ == kDecodeFinished) {
    output_cb_.Run(VideoFrame::CreateEOSFrame());
    decode_cb_bound.Run(DecodeStatus::OK);
    return;
  }

  DCHECK_EQ(state_, kNormal);

  // During decode, because reads are issued asynchronously, it is possible to
  // receive multiple end of stream buffers since each decode is acked. When the
  // first end of stream buffer is read, FFmpeg may still have frames queued
  // up in the decoder so we need to go through the decode loop until it stops
  // giving sensible data.  After that, the decoder should output empty
  // frames.  There are three states the decoder can be in:
  //
  //   kNormal: This is the starting state. Buffers are decoded. Decode errors
  //            are discarded.
  //   kDecodeFinished: All calls return empty frames.
  //   kError: Unexpected error happened.
  //
  // These are the possible state transitions.
  //
  // kNormal -> kDecodeFinished:
  //     When EOS buffer is received and the codec has been flushed.
  // kNormal -> kError:
  //     A decoding error occurs and decoding needs to stop.
  // (any state) -> kNormal:
  //     Any time Reset() is called.
  bool has_produced_frame;
  do {
    has_produced_frame = false;
    if (!LibavDecode(buffer, &has_produced_frame)) {
      state_ = kError;
      decode_cb_bound.Run(DecodeStatus::DECODE_ERROR);
      LOG(ERROR) << "Decode error";
      return;
    }
    // Repeat to flush the decoder after receiving EOS buffer.
  } while (buffer->end_of_stream() && has_produced_frame);

  if (buffer->end_of_stream()) {
    output_cb_.Run(VideoFrame::CreateEOSFrame());
    state_ = kDecodeFinished;
  }

  decode_cb_bound.Run(DecodeStatus::OK);
}

void LibavVideoDecoderTizen::Reset(const base::Closure& closure) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  avcodec_flush_buffers(codec_context_.get());
  state_ = kNormal;
  task_runner_->PostTask(FROM_HERE, closure);
}

void LibavVideoDecoderTizen::Stop() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (state_ == kUninitialized)
    return;

  ReleaseLibavResources();
  state_ = kUninitialized;
}

LibavVideoDecoderTizen::~LibavVideoDecoderTizen() {
  DCHECK_EQ(kUninitialized, state_);
  DCHECK(!codec_context_);
  DCHECK(!av_frame_);
}

bool LibavVideoDecoderTizen::LibavDecode(
    const scoped_refptr<DecoderBuffer>& buffer,
    bool* has_produced_frame) {
  DCHECK(!*has_produced_frame);

  // Create a packet for input data.
  // Due to Libav API changes we no longer have const read-only pointers.
  AVPacket packet;
  av_init_packet(&packet);
  if (buffer->end_of_stream()) {
    packet.data = NULL;
    packet.size = 0;
  } else {
    packet.data = const_cast<uint8_t*>(buffer->data());
    packet.size = buffer->data_size();

    // Let Libav handle presentation timestamp reordering.
    codec_context_->reordered_opaque = buffer->timestamp().InMicroseconds();
  }

  int frame_decoded = 0;
  int result = avcodec_decode_video2(codec_context_.get(), av_frame_.get(),
                                     &frame_decoded, &packet);
  // Log the problem if we can't decode a video frame and exit early.
  if (result < 0) {
    LOG(ERROR) << "Error decoding video: " << buffer->AsHumanReadableString();
    return false;
  }

  // FFmpeg says some codecs might have multiple frames per packet.  Previous
  // discussions with rbultje@ indicate this shouldn't be true for the codecs
  // we use.
  DCHECK_EQ(result, packet.size);

  // If no frame was produced then signal that more data is required to
  // produce more frames. This can happen under two circumstances:
  //   1) Decoder was recently initialized/flushed
  //   2) End of stream was reached and all internal frames have been output
  if (frame_decoded == 0) {
    return true;
  }

  if (!av_frame_->data[VideoFrame::kYPlane] ||
      !av_frame_->data[VideoFrame::kUPlane] ||
      !av_frame_->data[VideoFrame::kVPlane]) {
    LOG(ERROR) << "Video frame was produced yet has invalid frame data.";
    av_frame_unref(av_frame_.get());
    return false;
  }

  scoped_refptr<VideoFrame> frame =
      reinterpret_cast<VideoFrame*>(av_buffer_get_opaque(av_frame_->buf[0]));
  frame->set_timestamp(
      base::TimeDelta::FromMicroseconds(av_frame_->reordered_opaque));
  *has_produced_frame = true;
  output_cb_.Run(frame);

  av_frame_unref(av_frame_.get());
  return true;
}

void LibavVideoDecoderTizen::ReleaseLibavResources() {
  codec_context_.reset();
  av_frame_.reset();
}

bool LibavVideoDecoderTizen::ConfigureDecoder(bool low_delay) {
  // Release existing decoder resources if necessary.
  ReleaseLibavResources();

  // Initialize AVCodecContext structure.
  codec_context_.reset(avcodec_alloc_context3(NULL));
  if (!VideoDecoderConfigToAVCodecContext(config_, codec_context_.get()))
    return false;

  // Enable motion vector search (potentially slow), strong deblocking filter
  // for damaged macroblocks, and set our error detection sensitivity.
  codec_context_->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
  codec_context_->thread_count = GetThreadCount(codec_context_->codec_id);
  codec_context_->thread_type = low_delay ? FF_THREAD_SLICE : FF_THREAD_FRAME;
  codec_context_->opaque = this;
  codec_context_->flags |= CODEC_FLAG_EMU_EDGE;
  codec_context_->get_buffer2 = GetVideoBufferImpl;
  codec_context_->refcounted_frames = 1;

  if (decode_nalus_)
    codec_context_->flags2 |= CODEC_FLAG2_CHUNKS;

  AVCodec* codec = avcodec_find_decoder(codec_context_->codec_id);
  if (!codec || avcodec_open2(codec_context_.get(), codec, NULL) < 0) {
    ReleaseLibavResources();
    LOG(ERROR) << "Configuring Libav video decoder failed";
    return false;
  }

  av_frame_.reset(av_frame_alloc());
  return true;
}

}  // namespace media
