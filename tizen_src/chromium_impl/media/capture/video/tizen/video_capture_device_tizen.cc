// Copyright (c) 2012 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/tizen/video_capture_device_tizen.h"

#include "base/bind.h"
#include "third_party/libyuv/include/libyuv.h"
#include "tizen/system_info.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace {

#define ENUM_CASE(x) \
  case x:            \
    return #x;       \
    break

enum { kMjpegWidth = 640 };
enum { kMjpegHeight = 480 };
enum CameraOrientation {
  DEGREE_0 = 0,
  DEGREE_90 = 90,
  DEGREE_180 = 180,
  DEGREE_270 = 270,
  DEGREE_360 = 360,
};

camera_pixel_format_e ToCapiType(media::VideoPixelFormat e) {
  switch (e) {
    case media::PIXEL_FORMAT_NV12:
      return CAMERA_PIXEL_FORMAT_NV12;
    case media::PIXEL_FORMAT_NV21:
      return CAMERA_PIXEL_FORMAT_NV21;
    case media::PIXEL_FORMAT_YUY2:
      return CAMERA_PIXEL_FORMAT_YUYV;
    case media::PIXEL_FORMAT_RGB24:
      return CAMERA_PIXEL_FORMAT_RGB888;
    case media::PIXEL_FORMAT_UYVY:
      return CAMERA_PIXEL_FORMAT_UYVY;
    case media::PIXEL_FORMAT_ARGB:
      return CAMERA_PIXEL_FORMAT_ARGB;
    case media::PIXEL_FORMAT_I420:
      return CAMERA_PIXEL_FORMAT_I420;
    case media::PIXEL_FORMAT_YV12:
      return CAMERA_PIXEL_FORMAT_YV12;
    case media::PIXEL_FORMAT_MJPEG:
      return CAMERA_PIXEL_FORMAT_JPEG;
    default:
      NOTREACHED();
  }

  LOG(ERROR) << __func__ << " " << e;
  return CAMERA_PIXEL_FORMAT_INVALID;
}

struct CameraSpec {
  const char* device_id_;
  CameraOrientation orientation_;
};

const std::vector<CameraSpec>& GetCameraSpec() {
  const static std::vector<CameraSpec> kMobileCameraSpecs = {
      {media::VideoCaptureDeviceTizen::GetFrontCameraID().c_str(), DEGREE_270},
      {media::VideoCaptureDeviceTizen::GetBackCameraID().c_str(), DEGREE_90}};

  const static std::vector<CameraSpec> kCommonCameraSpecs = {
      {media::VideoCaptureDeviceTizen::GetFrontCameraID().c_str(), DEGREE_0}};

  if (IsMobileProfile() || IsWearableProfile())
    return kMobileCameraSpecs;
  else
    return kCommonCameraSpecs;
}

}  // unnamed namespace

namespace media {

#if TIZEN_MM_DEBUG_VIDEO_DUMP_DECODED_FRAME == 1
FrameDumper* frameDumper;
#endif

const std::string VideoCaptureDeviceTizen::kFrontCameraName = "front";
const std::string VideoCaptureDeviceTizen::kBackCameraName = "back";

const std::string VideoCaptureDeviceTizen::kCameraId0 = "0";
const std::string VideoCaptureDeviceTizen::kCameraId1 = "1";

// Note : Camera ID for Mobile or Wearable profile > Front : 1 / Back : 0
const std::string VideoCaptureDeviceTizen::GetFrontCameraID() {
  if (IsMobileProfile() || IsWearableProfile())
    return VideoCaptureDeviceTizen::kCameraId1;
  else
    return VideoCaptureDeviceTizen::kCameraId0;
}

// static
const std::string VideoCaptureDeviceTizen::GetBackCameraID() {
  if (IsMobileProfile() || IsWearableProfile())
    return VideoCaptureDeviceTizen::kCameraId0;
  else
    return VideoCaptureDeviceTizen::kCameraId1;
}

static CameraOrientation GetCameraOrientation(const char* device_id) {
  auto& camera_spec = GetCameraSpec();
  for (size_t i = 0; i < camera_spec.size(); i++) {
    const CameraSpec& cameraspec = camera_spec.at(i);
    if (strcmp(cameraspec.device_id_, device_id) == 0) {
      return cameraspec.orientation_;
    }
  }
  return DEGREE_0;
}

VideoCaptureDeviceTizen::VideoCaptureDeviceTizen(
    const VideoCaptureDeviceDescriptor& device_descriptor)
    : camera_instance_(nullptr),
      buffer_(),
      device_descriptor_(device_descriptor),
      worker_("VideoCapture"),
      is_capturing_(false),
      use_media_packet_(false),
      state_(kIdle) {
#if TIZEN_MM_DEBUG_VIDEO_DUMP_DECODED_FRAME == 1
  frameDumper = new FrameDumper();
#endif
}

VideoCaptureDeviceTizen::~VideoCaptureDeviceTizen() {
  if (camera_instance_ != nullptr)
    camera_instance_->UnsetClient(this);

  state_ = kIdle;
  DCHECK(!worker_.IsRunning());
  photo_callbacks_.clear();
}

void VideoCaptureDeviceTizen::OnStreamStopped() {
  SetErrorState("Camera handle reset", FROM_HERE);
}

void VideoCaptureDeviceTizen::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<VideoCaptureDevice::Client> client) {
  DCHECK(!worker_.IsRunning());
  worker_.Start();

  worker_.message_loop()->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&VideoCaptureDeviceTizen::OnAllocateAndStart,
                 base::Unretained(this),
                 params.requested_format.frame_size.width(),
                 params.requested_format.frame_size.height(),
                 params.requested_format.frame_rate,
                 params.requested_format.pixel_format, base::Passed(&client)));
}

void VideoCaptureDeviceTizen::StopAndDeAllocate() {
  DCHECK(worker_.IsRunning());
  worker_.message_loop()->task_runner()->PostTask(
      FROM_HERE, base::Bind(&VideoCaptureDeviceTizen::OnStopAndDeAllocate,
                            base::Unretained(this)));
  worker_.Stop();
}

void VideoCaptureDeviceTizen::GetPhotoState(GetPhotoStateCallback callback) {
  mojom::PhotoStatePtr photo_capabilities = mojom::PhotoState::New();
  CameraHandle* camera_instance = CameraHandle::GetInstance();

  photo_capabilities->iso = camera_instance->capabilities_->GetIso();
  photo_capabilities->height = camera_instance->capabilities_->GetImageHeight();
  photo_capabilities->width = camera_instance->capabilities_->GetImageWidth();
  photo_capabilities->zoom = camera_instance->capabilities_->GetZoom();
  photo_capabilities->current_focus_mode =
      camera_instance->capabilities_->GetFocusMode();
  photo_capabilities->current_exposure_mode =
      camera_instance->capabilities_->GetExposureMode();
  photo_capabilities->exposure_compensation =
      camera_instance->capabilities_->GetExposureCompensation();
  photo_capabilities->current_white_balance_mode =
      camera_instance->capabilities_->GetWhiteBalanceMode();
  photo_capabilities->red_eye_reduction =
      camera_instance->capabilities_->GetRedEyeReduction();
  photo_capabilities->color_temperature =
      camera_instance->capabilities_->GetColorTemperature();
  photo_capabilities->brightness =
      camera_instance->capabilities_->GetBrightness();
  photo_capabilities->contrast = camera_instance->capabilities_->GetContrast();
  photo_capabilities->saturation =
      camera_instance->capabilities_->GetSaturation();
  photo_capabilities->sharpness =
      camera_instance->capabilities_->GetSharpness();
  std::move(callback).Run(std::move(photo_capabilities));
}

void VideoCaptureDeviceTizen::SetPhotoOptions(
    mojom::PhotoSettingsPtr settings,
    SetPhotoOptionsCallback callback) {
  CameraHandle* camera_instance = CameraHandle::GetInstance();
  if (settings->has_zoom) {
    if (!camera_instance->SetZoom(settings->zoom)) {
      LOG(ERROR) << "Failed setting zoom";
      std::move(callback).Run(false);
      return;
    }
  }

  int width = settings->has_width ? settings->width : 640;
  int height = settings->has_height ? settings->height : 480;
  if (!camera_instance->SetResolution(width, height)) {
    LOG(ERROR) << "Failed setting resolution " << width << "x" << height;
    std::move(callback).Run(false);
    return;
  }

  if (settings->has_exposure_compensation) {
    if (!camera_instance->SetExposure(settings->exposure_compensation)) {
      LOG(ERROR) << "Failed setting exposure";
      std::move(callback).Run(false);
      return;
    }
  }

  if (settings->has_brightness) {
    if (!camera_instance->SetBrightness(settings->brightness)) {
      LOG(ERROR) << "Failed setting brightness";
      std::move(callback).Run(false);
      return;
    }
  }

  if (settings->has_contrast) {
    if (!camera_instance->SetContrast(settings->contrast)) {
      LOG(ERROR) << "Failed setting contrast";
      std::move(callback).Run(false);
      return;
    }
  }

  if (settings->has_iso) {
    if (!camera_instance->SetIso(settings->iso)) {
      LOG(ERROR) << "Failed setting iso";
      std::move(callback).Run(false);
      return;
    }
  }

  std::move(callback).Run(true);
}

void VideoCaptureDeviceTizen::TakePhoto(TakePhotoCallback callback) {
  int err = 0;
  if (CAMERA_ERROR_NONE != (err = camera_instance_->CameraSetCaptureFormat(
                                CAMERA_PIXEL_FORMAT_JPEG))) {
    SetErrorState(GetErrorString(err), FROM_HERE);
    return;
  }

  std::unique_ptr<TakePhotoCallback> heap_callback(
      new TakePhotoCallback(std::move(callback)));

  if (CAMERA_ERROR_NONE !=
      (err = camera_instance_->CameraStartCapture(
           VideoCaptureDeviceTizen::CapturedCb,
           VideoCaptureDeviceTizen::CaptureCompletedCb, this))) {
    SetErrorState(GetErrorString(err), FROM_HERE);
    return;
  }
  photo_callbacks_.push_back(std::move(heap_callback));
}

camera_device_e VideoCaptureDeviceTizen::DeviceNameToCameraId(
    const VideoCaptureDeviceDescriptor& device_descriptor) {
  if (device_descriptor.device_id.compare("0") == 0) {
    return CAMERA_DEVICE_CAMERA0;
  } else if (device_descriptor.device_id.compare("1") == 0) {
    return CAMERA_DEVICE_CAMERA1;
  } else {
    NOTREACHED();
  }
  return static_cast<camera_device_e>(-1);
}

bool VideoCaptureDeviceTizen::IsCapturing() {
  base::AutoLock auto_lock(capturing_state_lock_);
  return is_capturing_;
}

void VideoCaptureDeviceTizen::ChangeCapturingState(bool state) {
  base::AutoLock auto_lock(capturing_state_lock_);
  is_capturing_ = state;
}

void VideoCaptureDeviceTizen::OnCameraCapturedWithMediaPacket(
    media_packet_h pkt,
    void* data) {
  VideoCaptureDeviceTizen* self = static_cast<VideoCaptureDeviceTizen*>(data);

  if (self->IsCapturing()) {
    media_packet_destroy(pkt);
    return;
  }

  self->ChangeCapturingState(true);
  ScopedMediaPacket packet_proxy(pkt);
  self->worker_.message_loop()->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&VideoCaptureDeviceTizen::PreviewCameraCaptured,
                 base::Unretained(self), base::Passed(&packet_proxy)));
}

void VideoCaptureDeviceTizen::PreviewCameraCaptured(ScopedMediaPacket pkt) {
  if (!client_) {
    ChangeCapturingState(false);
    return;
  }
  ScopedMediaFormat format;
  media_format_h fmt = nullptr;
  media_packet_get_format(pkt.get(), &fmt);
  format.reset(fmt);

  media_format_mimetype_e mime;
  int width, height, avg_bps, max_bps;

  media_format_get_video_info(format.get(), &mime, &width, &height, &avg_bps,
                              &max_bps);
  CHECK((mime == MEDIA_FORMAT_I420) || (mime == MEDIA_FORMAT_NV12));

  camera_attr_fps_e current_fps = camera_instance_->CameraGetPreviewFps();

  const display::Display display =
      display::Screen::GetScreen()->GetPrimaryDisplay();
  int orientation = display.RotationAsDegree();
  gfx::Size target_resolution(width, height);

  int target_rotation =
      (orientation +
       GetCameraOrientation(device_descriptor_.device_id.c_str())) %
      DEGREE_360;

  int src_stride_y = width;
  int src_stride_uv = (src_stride_y + 1) / 2;

  media::VideoCaptureFormat videocaptureformat;
  videocaptureformat.frame_size = gfx::Size(width, height);
  videocaptureformat.frame_rate = static_cast<int>(current_fps);
  videocaptureformat.pixel_format = media::PIXEL_FORMAT_I420;

  int dest_width = width;
  int dest_height = height;
  if (target_rotation == 90 || target_rotation == 270)
    std::swap(dest_height, dest_width);

  videocaptureformat.frame_size = gfx::Size(dest_width, dest_height);
  buffer_ = client_->ReserveOutputBuffer(videocaptureformat.frame_size,
                                         media::PIXEL_FORMAT_I420,
                                         media::PIXEL_STORAGE_CPU, 0);

  if (!buffer_.is_valid()) {
    LOG(ERROR) << "Failed to reserve I420 output buffer.";
    ChangeCapturingState(false);
    return;
  }

  libyuv::RotationMode mode =
      static_cast<libyuv::RotationMode>(target_rotation);

  int dest_stride_y = dest_width;
  int dest_stride_uv = (dest_stride_y + 1) / 2;

  std::unique_ptr<VideoCaptureBufferHandle> buffer_handle =
      buffer_.handle_provider->GetHandleForInProcessAccess();

  uint8* y_plane = static_cast<uint8*>(buffer_handle->data());
  uint8* u_plane = y_plane + dest_stride_y * dest_height;
  uint8* v_plane = u_plane + dest_stride_uv * dest_height / 2;

  if (mime == MEDIA_FORMAT_NV12) {
    src_stride_uv = src_stride_y;

    void *data_y, *data_uv;
    media_packet_get_video_plane_data_ptr(pkt.get(), 0, &data_y);
    media_packet_get_video_plane_data_ptr(pkt.get(), 1, &data_uv);

    libyuv::NV12ToI420Rotate(reinterpret_cast<uint8*>(data_y), src_stride_y,
                             reinterpret_cast<uint8*>(data_uv), src_stride_uv,
                             y_plane, dest_stride_y, u_plane, dest_stride_uv,
                             v_plane, dest_stride_uv, width, height, mode);
    base::TimeTicks now = base::TimeTicks::Now();
    if (first_ref_time_.is_null())
      first_ref_time_ = now;

    client_->OnIncomingCapturedBuffer(std::move(buffer_), videocaptureformat,
                                      now, now - first_ref_time_);
  } else if (mime == MEDIA_FORMAT_I420) {
    // FIXME: Verify if I420 format is working.
    void *data_y, *data_u, *data_v;
    media_packet_get_video_plane_data_ptr(pkt.get(), 0, &data_y);
    media_packet_get_video_plane_data_ptr(pkt.get(), 1, &data_u);
    media_packet_get_video_plane_data_ptr(pkt.get(), 2, &data_v);

    if (libyuv::I420Rotate(reinterpret_cast<uint8*>(data_y), src_stride_y,
                           reinterpret_cast<uint8*>(data_u), src_stride_uv,
                           reinterpret_cast<uint8*>(data_v), src_stride_uv,
                           y_plane, src_stride_y, u_plane, src_stride_uv,
                           v_plane, src_stride_uv, width, height, mode)) {
      LOG(ERROR) << "Failed to I420Rotate buffer";
      ChangeCapturingState(false);
      return;
    }

    base::TimeTicks now = base::TimeTicks::Now();
    if (first_ref_time_.is_null())
      first_ref_time_ = now;

    client_->OnIncomingCapturedBuffer(std::move(buffer_), videocaptureformat,
                                      now, now - first_ref_time_);
  } else {
    LOG(ERROR) << "frame->format( " << mime
               << ") is not implemented or not supported by chromium.";
  }

  ChangeCapturingState(false);
}

void VideoCaptureDeviceTizen::OnCameraCaptured(camera_preview_data_s* frame,
                                               void* data) {
  CHECK((frame->format == CAMERA_PIXEL_FORMAT_I420) ||
        (frame->format == CAMERA_PIXEL_FORMAT_NV12));
#if TIZEN_MM_DEBUG_VIDEO_DUMP_DECODED_FRAME == 1
  frameDumper->DumpFrame(frame);
#endif
  VideoCaptureDeviceTizen* self = static_cast<VideoCaptureDeviceTizen*>(data);
  if (!self->client_)
    return;
  camera_attr_fps_e current_fps = self->camera_instance_->CameraGetPreviewFps();

  DVLOG(3) << " width:" << frame->width << " height:" << frame->height;

  const display::Display display =
      display::Screen::GetScreen()->GetPrimaryDisplay();
  int orientation = display.RotationAsDegree();
  gfx::Size target_resolution(frame->width, frame->height);
  int target_rotation =
      (orientation +
       GetCameraOrientation(self->device_descriptor_.device_id.c_str())) %
      DEGREE_360;

  int src_stride_y = frame->width;
  int src_stride_uv = (src_stride_y + 1) / 2;

  media::VideoCaptureFormat videocaptureformat;
  videocaptureformat.frame_size = gfx::Size(frame->width, frame->height);
  videocaptureformat.frame_rate = static_cast<int>(current_fps);
  videocaptureformat.pixel_format = media::PIXEL_FORMAT_I420;
  videocaptureformat.pixel_storage = media::PIXEL_STORAGE_CPU;

  int dest_width = frame->width;
  int dest_height = frame->height;
  if (target_rotation == 90 || target_rotation == 270)
    std::swap(dest_height, dest_width);

  videocaptureformat.frame_size = gfx::Size(dest_width, dest_height);

  self->buffer_ = self->client_->ReserveOutputBuffer(
      videocaptureformat.frame_size, media::PIXEL_FORMAT_I420,
      media::PIXEL_STORAGE_CPU, 0);

  DLOG_IF(ERROR, !self->buffer_.is_valid())
      << "Couldn't allocate Capture Buffer";

  libyuv::RotationMode mode =
      static_cast<libyuv::RotationMode>(target_rotation);

  int dest_stride_y = dest_width;
  int dest_stride_uv = (dest_stride_y + 1) / 2;

  std::unique_ptr<VideoCaptureBufferHandle> buffer_handle =
      self->buffer_.handle_provider->GetHandleForInProcessAccess();

  DCHECK(buffer_handle->data()) << "Buffer has NO backing memory";

  uint8* y_plane = static_cast<uint8*>(buffer_handle->data());
  uint8* u_plane = y_plane + dest_stride_y * dest_height;
  uint8* v_plane = u_plane + dest_stride_uv * dest_height / 2;

  if (frame->format == CAMERA_PIXEL_FORMAT_NV12) {
    src_stride_uv = src_stride_y;

    libyuv::NV12ToI420Rotate(
        reinterpret_cast<uint8*>(frame->data.double_plane.y), src_stride_y,
        reinterpret_cast<uint8*>(frame->data.double_plane.uv), src_stride_uv,
        y_plane, dest_stride_y, u_plane, dest_stride_uv, v_plane,
        dest_stride_uv, frame->width, frame->height, mode);
  } else if (frame->format == CAMERA_PIXEL_FORMAT_I420) {
    // FIXME: Verify if I420 format is working.
    src_stride_uv = (src_stride_y + 1) / 2;

    libyuv::I420Rotate(
        reinterpret_cast<uint8*>(frame->data.triple_plane.y), src_stride_y,
        reinterpret_cast<uint8*>(frame->data.triple_plane.u), src_stride_uv,
        reinterpret_cast<uint8*>(frame->data.triple_plane.v), src_stride_uv,
        y_plane, dest_stride_y, u_plane, dest_stride_uv, v_plane,
        dest_stride_uv, frame->width, frame->height, mode);
  } else {
    LOG(INFO) << "Unsupported Format";
    return;
  }

  base::TimeTicks now = base::TimeTicks::Now();
  if (self->first_ref_time_.is_null())
    self->first_ref_time_ = now;

  self->client_->OnIncomingCapturedBuffer(std::move(self->buffer_),
                                          videocaptureformat, now,
                                          now - self->first_ref_time_);
}

void VideoCaptureDeviceTizen::OnAllocateAndStart(
    int width,
    int height,
    int frame_rate,
    VideoPixelFormat format,
    std::unique_ptr<Client> client) {
  DVLOG(3) << " width:" << width << " height:" << height
           << " frame_rate:" << frame_rate
           << " format:" << media::VideoPixelFormatToString(format).c_str();

  DCHECK_EQ(worker_.message_loop(), base::MessageLoop::current());

  client_ = std::move(client);

  camera_instance_ = CameraHandle::GetInstance();
  if (camera_instance_ == nullptr) {
    SetErrorState("Failed to get camera instance", FROM_HERE);
    return;
  }

  if (camera_instance_->GetDeviceName() !=
      DeviceNameToCameraId(device_descriptor_)) {
    camera_instance_->ResetHandle(DeviceNameToCameraId(device_descriptor_));
  }

  camera_instance_->SetClient(this);
  if (!camera_instance_->IsValid()) {
    SetErrorState("Camera handle is invalid", FROM_HERE);
    return;
  }

  int err =
      camera_instance_->CameraSetDisplay(CAMERA_DISPLAY_TYPE_NONE, nullptr);
  if (CAMERA_ERROR_NONE != err) {
    SetErrorState(GetErrorString(err), FROM_HERE);
    return;
  }

  err = camera_instance_->CameraSetPreviewFormat(ToCapiType(format));
  if (CAMERA_ERROR_NONE != err) {
    LOG(ERROR) << "camera_set_preview_format (" << format << ") failed. Error# "
               << GetErrorString(err);

    std::vector<media::VideoPixelFormat> supported_formats;
    if (!camera_instance_->GetSupportedPreviewPixelFormats(supported_formats)) {
      SetErrorState(
          "Camera internal error: GetSupportedPreviewPixelFormats Failed",
          FROM_HERE);
      return;
    }

    LOG(ERROR) << "Trying with format (" << supported_formats[0] << ")";
    err = camera_instance_->CameraSetPreviewFormat(
        ToCapiType(supported_formats[0]));
    if (CAMERA_ERROR_NONE != err) {
      SetErrorState(GetErrorString(err), FROM_HERE);
      return;
    }
  }

  err = camera_instance_->CameraSetPreviewResolution(width, height);
  if (CAMERA_ERROR_NONE != err) {
    LOG(ERROR) << "camera_set_preview_resolution failed. Error#" << err
               << ". Trying default resolution: " << kMjpegWidth << " x "
               << kMjpegHeight;

    width = kMjpegWidth;
    height = kMjpegHeight;
    err = camera_instance_->CameraSetPreviewResolution(width, height);
    if (CAMERA_ERROR_NONE != err) {
      SetErrorState(GetErrorString(err), FROM_HERE);
      return;
    }
  }

  err = camera_instance_->CameraSetMediapacketPreviewCb(
      OnCameraCapturedWithMediaPacket, this);
  if (CAMERA_ERROR_NONE == err)
    use_media_packet_ = true;
  else {
    err = camera_instance_->CameraSetPreviewCb(OnCameraCaptured, this);
    if (CAMERA_ERROR_NONE != err) {
      SetErrorState(GetErrorString(err), FROM_HERE);
      return;
    }
    use_media_packet_ = false;
  }

  err = camera_instance_->CameraSetPreviewFps(
      static_cast<camera_attr_fps_e>(frame_rate));
  if (CAMERA_ERROR_NONE != err) {
    LOG(ERROR) << "cameraSetPreviewFps failed. Error# " << err;

    int max_supported_fps = camera_instance_->GetMaxFrameRate(
        CameraCapability::Resolution(width, height));
    if (!max_supported_fps) {
      SetErrorState("Camera internal error : GetMaxFrameRate Failed",
                    FROM_HERE);
      return;
    }

    err = camera_instance_->CameraSetPreviewFps(
        static_cast<camera_attr_fps_e>(max_supported_fps));
    if (CAMERA_ERROR_NONE != err) {
      SetErrorState(GetErrorString(err), FROM_HERE);
      return;
    }
  }

  if (IsMobileProfile() && !IsEmulatorArch() &&
      (camera_instance_->GetDeviceName() == CAMERA_DEVICE_CAMERA1)) {
    err = camera_instance_->CameraSetStreamFlip(CAMERA_FLIP_VERTICAL);
    if (CAMERA_ERROR_NONE != err) {
      SetErrorState(GetErrorString(err), FROM_HERE);
      return;
    }
  }

  state_ = kCapturing;

  err = camera_instance_->CameraStartPreview();
  if (CAMERA_ERROR_NONE != err) {
    SetErrorState(GetErrorString(err), FROM_HERE);
    return;
  }

  if (IsMobileProfile() || IsWearableProfile())
    WakeUpDisplayAndAcquireDisplayLock();
}

void VideoCaptureDeviceTizen::OnStopAndDeAllocate() {
  DCHECK_EQ(worker_.message_loop(), base::MessageLoop::current());
  if (use_media_packet_)
    camera_instance_->CameraUnsetMediapacketPreviewCb();
  else
    camera_instance_->CameraUnsetPreviewCb();
  camera_instance_->CameraStopPreview();

  if (IsMobileProfile() || IsWearableProfile())
    ReleaseDisplayLock();

  state_ = kIdle;
  client_.reset();
}

const char* VideoCaptureDeviceTizen::GetErrorString(int err_code) {
  switch (err_code) {
    ENUM_CASE(CAMERA_ERROR_NONE);
    ENUM_CASE(CAMERA_ERROR_INVALID_PARAMETER);
    ENUM_CASE(CAMERA_ERROR_INVALID_STATE);
    ENUM_CASE(CAMERA_ERROR_OUT_OF_MEMORY);
    ENUM_CASE(CAMERA_ERROR_DEVICE);
    ENUM_CASE(CAMERA_ERROR_INVALID_OPERATION);
    ENUM_CASE(CAMERA_ERROR_SECURITY_RESTRICTED);
    ENUM_CASE(CAMERA_ERROR_DEVICE_BUSY);
    ENUM_CASE(CAMERA_ERROR_DEVICE_NOT_FOUND);
    ENUM_CASE(CAMERA_ERROR_ESD);
    ENUM_CASE(CAMERA_ERROR_PERMISSION_DENIED);
    ENUM_CASE(CAMERA_ERROR_NOT_SUPPORTED);
    ENUM_CASE(CAMERA_ERROR_RESOURCE_CONFLICT);
    ENUM_CASE(CAMERA_ERROR_SERVICE_DISCONNECTED);
  };
  NOTREACHED() << "Unknown camera error code #" << err_code;
  return "";
}

void VideoCaptureDeviceTizen::SetErrorState(const char* error,
                                            const base::Location& location) {
  LOG(ERROR) << "Camera error# " << error << " from "
             << location.function_name() << "(" << location.line_number()
             << ")";

  DCHECK(!worker_.IsRunning() ||
         worker_.message_loop() == base::MessageLoop::current());
  state_ = kError;
  ChangeCapturingState(false);
  client_->OnError(location, error);
}

void VideoCaptureDeviceTizen::CapturedCb(camera_image_data_s* image,
                                         camera_image_data_s* postview,
                                         camera_image_data_s* thumbnail,
                                         void* userData) {
  LOG(INFO) << "CapturedCb!";
  VideoCaptureDeviceTizen* handle =
      static_cast<VideoCaptureDeviceTizen*>(userData);

  if (handle->photo_callbacks_.empty())
    return;

  mojom::BlobPtr blob = mojom::Blob::New();
  blob->data.resize(image->size);
  memcpy(blob->data.data(), image->data, image->size);

  if (blob) {
    TakePhotoCallback cb = std::move(*(handle->photo_callbacks_.front().get()));
    handle->photo_callbacks_.pop_front();
    std::move(cb).Run(std::move(blob));
  }
}

void VideoCaptureDeviceTizen::CaptureCompletedCb(void* userData) {
  LOG(INFO) << "Capture Completed!";
  VideoCaptureDeviceTizen* handle =
      static_cast<VideoCaptureDeviceTizen*>(userData);

  handle->worker_.message_loop()->task_runner()->PostTask(
      FROM_HERE, base::Bind(&VideoCaptureDeviceTizen::OnCaptureCompletedCb,
                            base::Unretained(handle)));
}

void VideoCaptureDeviceTizen::OnCaptureCompletedCb() {
  if (!camera_instance_) {
    SetErrorState("Failed to get camera instance!", FROM_HERE);
    return;
  }

  int err = 0;
  if (CAMERA_ERROR_NONE != (err = camera_instance_->CameraStartPreview())) {
    SetErrorState(GetErrorString(err), FROM_HERE);
    return;
  }
}

}  // namespace media
