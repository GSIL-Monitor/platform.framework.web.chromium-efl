// Copyright 2016 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/tizen/camera_device_tizen.h"

#include "media/capture/video/tizen/video_capture_device_tizen.h"

namespace {

enum { kMaxWidth = 1280 };
enum { kMaxHeight = 720 };
enum { kMaxFramerate = CAMERA_ATTR_FPS_30 };

const int kMinIso = 50;
const int kMaxIso = 3200;
const int kStepIso = 50;

media::VideoPixelFormat toChromiumType(camera_pixel_format_e e) {
  switch (e) {
    case CAMERA_PIXEL_FORMAT_NV12:
      return media::PIXEL_FORMAT_NV12;
    case CAMERA_PIXEL_FORMAT_NV21:
      return media::PIXEL_FORMAT_NV21;
    case CAMERA_PIXEL_FORMAT_YUYV:
      return media::PIXEL_FORMAT_YUY2;
    case CAMERA_PIXEL_FORMAT_RGB888:
      return media::PIXEL_FORMAT_RGB24;
    case CAMERA_PIXEL_FORMAT_UYVY:
      return media::PIXEL_FORMAT_UYVY;
    case CAMERA_PIXEL_FORMAT_ARGB:
      return media::PIXEL_FORMAT_ARGB;
    case CAMERA_PIXEL_FORMAT_I420:
      return media::PIXEL_FORMAT_I420;
    case CAMERA_PIXEL_FORMAT_YV12:
      return media::PIXEL_FORMAT_YV12;
    case CAMERA_PIXEL_FORMAT_JPEG:
      return media::PIXEL_FORMAT_MJPEG;
    default:
      NOTREACHED() << "Unknown format #" << e;
  }
  return media::PIXEL_FORMAT_UNKNOWN;
}

int FromIsoToInteger(camera_attr_iso_e mode) {
  switch (mode) {
    case CAMERA_ATTR_ISO_50:
      return 50;
    case CAMERA_ATTR_ISO_100:
      return 100;
    case CAMERA_ATTR_ISO_200:
      return 200;
    case CAMERA_ATTR_ISO_400:
      return 400;
    case CAMERA_ATTR_ISO_800:
      return 800;
    case CAMERA_ATTR_ISO_1600:
      return 1600;
    case CAMERA_ATTR_ISO_3200:
      return 3200;

    // return minimum value in default case
    default:
      LOG(ERROR) << "Unknown mode (" << mode << ") Returning " << kMinIso;
      return kMinIso;
  }
}

camera_attr_iso_e FromIntegerToCapiIsoFormat(unsigned int value) {
  if (value == kMinIso)
    return CAMERA_ATTR_ISO_50;
  if (value == 100)
    return CAMERA_ATTR_ISO_100;
  if (value == 200)
    return CAMERA_ATTR_ISO_200;
  if (value == 400)
    return CAMERA_ATTR_ISO_400;
  if (value == 800)
    return CAMERA_ATTR_ISO_800;
  if (value == 1600)
    return CAMERA_ATTR_ISO_1600;
  if (value == kMaxIso)
    return CAMERA_ATTR_ISO_3200;

  LOG(ERROR) << "Unknown value (" << value << ") Returning ISO Auto";
  return CAMERA_ATTR_ISO_AUTO;
}

bool OnCameraSupportedPreviewResolution(int width,
                                        int height,
                                        void* user_data) {
  std::vector<gfx::Size>* sizes =
      static_cast<std::vector<gfx::Size>*>(user_data);
  DCHECK(sizes);

  if ((width > kMaxWidth && height > kMaxHeight) ||
      (height > kMaxWidth && width > kMaxHeight)) {
    DVLOG(1) << "Ignore resolution [width:" << width << " x height:" << height
             << "] and continue to next resolution";
    return true;
  }
  sizes->push_back(gfx::Size(width, height));
  return true;
}

bool OnCameraSupportedPreviewFormat(camera_pixel_format_e format,
                                    void* user_data) {
  std::vector<media::VideoPixelFormat>* list_format =
      static_cast<std::vector<media::VideoPixelFormat>*>(user_data);
  DCHECK(list_format);

  list_format->push_back(toChromiumType(format));
  return true;
}

bool OnCameraSupportedFPS(camera_attr_fps_e fps, void* user_data) {
  std::vector<int>* list_fps = static_cast<std::vector<int>*>(user_data);
  DCHECK(list_fps);
  if (CAMERA_ATTR_FPS_AUTO == fps ||
      static_cast<camera_attr_fps_e>(kMaxFramerate) < fps) {
    // AUTO format is not defined on Chromium, so skip.
    DVLOG(1) << "Ignore fps: [CAMERA_ATTR_FPS_AUTO = " << CAMERA_ATTR_FPS_AUTO
             << "] OR fps: [" << fps << " > " << kMaxFramerate << "]";
    return true;
  }
  list_fps->push_back(static_cast<int>(fps));
  return true;
}

void GenerateChromiumVideoCaptureFormat(
    std::vector<media::CameraCapability>& capabilities,
    const std::vector<media::VideoPixelFormat>& formats,
    std::vector<media::VideoCaptureFormat>& outSupportedFormats) {
  for (auto supportedFrameInfo : capabilities) {
    for (auto supportedFormat : formats) {
      media::VideoCaptureFormat format;
      format.frame_size = supportedFrameInfo.resolution;
      format.frame_rate = supportedFrameInfo.fps;
      format.pixel_format = supportedFormat;
      outSupportedFormats.push_back(format);
      DVLOG(1) << " frame_size:" << format.frame_size.width() << "X"
               << format.frame_size.height()
               << " frame_rate:" << format.frame_rate
               << " pixel_format:" << format.pixel_format;
    }
  }
}

}  // anonymous namespace

namespace media {

mojom::RangePtr SetRange(double min, double max, double current, double step) {
  mojom::RangePtr range_ptr = mojom::Range::New();
  range_ptr->min = min;
  range_ptr->max = max;
  range_ptr->current = current;
  range_ptr->step = step;
  return range_ptr;
}

mojom::RedEyeReduction PhotoCapabilities::GetRedEyeReduction() {
  camera_attr_flash_mode_e mode = CAMERA_ATTR_FLASH_MODE_OFF;
  camera_attr_get_flash_mode(camera_, &mode);
  if (mode != CAMERA_ATTR_FLASH_MODE_REDEYE_REDUCTION)
    return mojom::RedEyeReduction::CONTROLLABLE;
  return mojom::RedEyeReduction::NEVER;
}

mojom::RangePtr PhotoCapabilities::GetExposureCompensation() {
  if (exposureCompensation_.min == -1)
    camera_attr_get_exposure_range(camera_, &exposureCompensation_.min,
                                   &exposureCompensation_.max);

  if (exposureCompensation_.min != -1) {
    int current = -1;
    if (CAMERA_ERROR_NONE == camera_attr_get_exposure(camera_, &current))
      return SetRange(exposureCompensation_.min, exposureCompensation_.max,
                      current, 1.0);
  }
  return mojom::Range::New();
}

mojom::RangePtr PhotoCapabilities::GetIso() {
  camera_attr_iso_e current_iso = CAMERA_ATTR_ISO_AUTO;
  if (CAMERA_ERROR_NONE == camera_attr_get_iso(camera_, &current_iso))
    return SetRange(kMinIso, kMaxIso, FromIsoToInteger(current_iso), kStepIso);

  return mojom::Range::New();
}

mojom::RangePtr PhotoCapabilities::GetBrightness() {
  if (brightness_.min == -1)
    camera_attr_get_brightness_range(camera_, &brightness_.min,
                                     &brightness_.max);

  if (brightness_.min != -1) {
    int current = -1;
    if (CAMERA_ERROR_NONE == camera_attr_get_brightness(camera_, &current))
      return SetRange(brightness_.min, brightness_.max, current, 1.0);
  }
  return mojom::Range::New();
}

mojom::RangePtr PhotoCapabilities::GetContrast() {
  if (contrast_.min == -1)
    camera_attr_get_contrast_range(camera_, &contrast_.min, &contrast_.max);

  if (contrast_.min != -1) {
    int current = -1;
    if (CAMERA_ERROR_NONE == camera_attr_get_contrast(camera_, &current))
      return SetRange(contrast_.min, contrast_.max, current, 1.0);
  }
  return mojom::Range::New();
}

mojom::RangePtr PhotoCapabilities::GetImageHeight() {
  // TODO: Remove hardcoding height and width values
  return SetRange(480, 480, 480, 0.0);
}

mojom::RangePtr PhotoCapabilities::GetImageWidth() {
  return SetRange(640, 640, 640, 0.0);
}

mojom::RangePtr PhotoCapabilities::GetZoom() {
  if (zoom_.min == -1)
    camera_attr_get_zoom_range(camera_, &zoom_.min, &zoom_.max);

  if (zoom_.min != -1) {
    int current = -1;
    if (CAMERA_ERROR_NONE == camera_attr_get_zoom(camera_, &current))
      return SetRange(zoom_.min, zoom_.max, current, 1.0);
  }
  return mojom::Range::New();
}

CameraHandle::CameraHandle() : camera_handle_(nullptr), client_(nullptr) {
  LOG(INFO) << "Creating the camera instance";
  device_name_ = CAMERA_DEVICE_CAMERA0;
  ResetHandle(device_name_);
}

CameraHandle::~CameraHandle() {
  if (camera_handle_ != nullptr) {
    camera_destroy(camera_handle_);
    camera_handle_ = nullptr;
  }
}

void CameraHandle::ResetHandle(camera_device_e device_name) {
  int err = 0;
  if (camera_handle_ != nullptr) {
    if (client_ != nullptr)
      client_->OnStreamStopped();

    if (CAMERA_ERROR_NONE != (err = camera_destroy(camera_handle_))) {
      LOG(ERROR) << "camera_destroy error : "
                 << media::VideoCaptureDeviceTizen::GetErrorString(err);
    }

    camera_handle_ = nullptr;
  }

  if (CAMERA_ERROR_NONE !=
      (err = camera_create(device_name, &camera_handle_))) {
    camera_handle_ = nullptr;
    LOG(ERROR) << "Cannot create camera, Error:"
               << media::VideoCaptureDeviceTizen::GetErrorString(err);
  } else {
    device_name_ = device_name;
  }
  capabilities_.reset(new PhotoCapabilities(camera_handle_));
}

void CameraHandle::SetClient(CameraHandleClient* client) {
  client_ = client;
}

void CameraHandle::UnsetClient(CameraHandleClient* client) {
  if (client_ == client)
    client_ = nullptr;
}

CameraHandle* CameraHandle::GetInstance() {
  return base::Singleton<CameraHandle>::get();
}

bool CameraHandle::IsValid() {
  return camera_handle_ != nullptr;
}

bool CameraHandle::GetSupportedPreviewResolutions(
    std::vector<CameraCapability::Resolution>& supported_resolutions) const {
  int err = 0;
  if (CAMERA_ERROR_NONE !=
      (err = camera_foreach_supported_preview_resolution(
           camera_handle_, OnCameraSupportedPreviewResolution,
           &supported_resolutions))) {
    LOG(ERROR) << "Cannot get the supported resolutions for camera, Error:"
               << media::VideoCaptureDeviceTizen::GetErrorString(err);
    return false;
  }
  return true;
}

bool CameraHandle::GetSupportedPreviewPixelFormats(
    std::vector<media::VideoPixelFormat>& supported_formats) const {
  int err = 0;
  if (CAMERA_ERROR_NONE != (err = camera_foreach_supported_preview_format(
                                camera_handle_, OnCameraSupportedPreviewFormat,
                                &supported_formats))) {
    LOG(ERROR) << "Cannot get the supported formats for camera, Error:"
               << media::VideoCaptureDeviceTizen::GetErrorString(err);
    return false;
  }
  return true;
}

bool CameraHandle::GetSupportedPreviewCapabilities(
    std::vector<CameraCapability>& supported_capabilities) const {
  std::vector<CameraCapability::Resolution> preview_resolutions;
  if (!GetSupportedPreviewResolutions(preview_resolutions))
    return false;

  for (auto resolution : preview_resolutions) {
    std::vector<int> frame_rates;
    int width = resolution.width();
    int height = resolution.height();
    if (CAMERA_ERROR_NONE != camera_attr_foreach_supported_fps_by_resolution(
                                 camera_handle_, width, height,
                                 OnCameraSupportedFPS, &frame_rates)) {
      LOG(WARNING) << "Cannot get the supported FPS for this resolution : "
                   << width << "X" << height;
      continue;
    }

    for (auto rate : frame_rates)
      supported_capabilities.push_back(CameraCapability(resolution, rate));
  }

  if (supported_capabilities.empty())
    return false;

  return true;
}

void CameraHandle::GetDeviceSupportedFormats(
    media::VideoCaptureFormats& supported_formats) {
  std::vector<media::VideoPixelFormat> supported_pixel_formats;
  std::vector<CameraCapability> supported_capabilities;

  if (!IsValid()) {
    LOG(ERROR) << "Cannot use camera";
    return;
  }

  if (!GetSupportedPreviewPixelFormats(supported_pixel_formats))
    return;

  if (!GetSupportedPreviewCapabilities(supported_capabilities))
    return;

  supported_formats.clear();
  GenerateChromiumVideoCaptureFormat(
      supported_capabilities, supported_pixel_formats, supported_formats);
}

int CameraHandle::GetMaxFrameRate(
    CameraCapability::Resolution resolution) const {
  std::vector<int> frame_rates;
  int err = 0;
  if (CAMERA_ERROR_NONE !=
      (err = camera_attr_foreach_supported_fps_by_resolution(
           camera_handle_, resolution.width(), resolution.height(),
           OnCameraSupportedFPS, &frame_rates))) {
    LOG(ERROR) << "Cannot get the supported FPS for this resolution : "
               << resolution.width() << "X" << resolution.height() << " Error: "
               << media::VideoCaptureDeviceTizen::GetErrorString(err);
    return 0;
  }

  return *std::max_element(frame_rates.begin(), frame_rates.end());
}

int CameraHandle::GetDeviceCounts() {
  int device_count = 0;
  int err = 0;
  if (CAMERA_ERROR_NONE !=
      (err = camera_get_device_count(camera_handle_, &device_count))) {
    device_count = 0;
    LOG(ERROR) << "Cannot read camera count, Error:"
               << media::VideoCaptureDeviceTizen::GetErrorString(err);
  }
  return device_count;
}

// Set Photo Capabilities
bool CameraHandle::SetZoom(int value) {
  return CAMERA_ERROR_NONE == camera_attr_set_zoom(camera_handle_, value);
}

bool CameraHandle::SetResolution(int width, int height) {
  return CAMERA_ERROR_NONE ==
         camera_set_capture_resolution(camera_handle_, width, height);
}

bool CameraHandle::SetBrightness(int value) {
  return CAMERA_ERROR_NONE == camera_attr_set_brightness(camera_handle_, value);
}

bool CameraHandle::SetContrast(int value) {
  return CAMERA_ERROR_NONE == camera_attr_set_contrast(camera_handle_, value);
}

bool CameraHandle::SetIso(int value) {
  return CAMERA_ERROR_NONE ==
         camera_attr_set_iso(camera_handle_, FromIntegerToCapiIsoFormat(value));
}

bool CameraHandle::SetExposure(int value) {
  return CAMERA_ERROR_NONE == camera_attr_set_exposure(camera_handle_, value);
}

int CameraHandle::CameraStartPreview() {
  return camera_start_preview(camera_handle_);
}

int CameraHandle::CameraStopPreview() {
  return camera_stop_preview(camera_handle_);
}

camera_state_e CameraHandle::CameraGetState() {
  camera_state_e state = CAMERA_STATE_NONE;
  if (CAMERA_ERROR_NONE != camera_get_state(camera_handle_, &state))
    LOG(ERROR) << "FAILED to get camera state";
  return state;
}

int CameraHandle::CameraGetPreviewResolution(int* width, int* height) {
  return camera_get_preview_resolution(camera_handle_, width, height);
}

int CameraHandle::CameraGetPreviewFormat(camera_pixel_format_e* format) {
  return camera_get_preview_format(camera_handle_, format);
}

camera_attr_fps_e CameraHandle::CameraGetPreviewFps() {
  camera_attr_fps_e current_fps = static_cast<camera_attr_fps_e>(30);
  camera_attr_get_preview_fps(camera_handle_, &current_fps);
  return current_fps;
}

int CameraHandle::CameraSetDisplay(camera_display_type_e type,
                                   camera_display_h display) {
  return camera_set_display(camera_handle_, type, display);
}

int CameraHandle::CameraSetCaptureResolution(int width, int height) {
  return camera_set_capture_resolution(camera_handle_, width, height);
}

int CameraHandle::CameraSetCaptureFormat(camera_pixel_format_e format) {
  return camera_set_capture_format(camera_handle_, format);
}

int CameraHandle::CameraStartCapture(camera_capturing_cb capturingCB,
                                     camera_capture_completed_cb completedCB,
                                     void* data) {
  return camera_start_capture(camera_handle_, capturingCB, completedCB, data);
}

int CameraHandle::CameraSetPreviewResolution(int width, int height) {
  return camera_set_preview_resolution(camera_handle_, width, height);
}

int CameraHandle::CameraSetPreviewFormat(camera_pixel_format_e format) {
  return camera_set_preview_format(camera_handle_, format);
}

int CameraHandle::CameraSetPreviewFps(camera_attr_fps_e fps) {
  return camera_attr_set_preview_fps(camera_handle_, fps);
}

int CameraHandle::CameraSetPreviewCb(camera_preview_cb callback, void* data) {
  return camera_set_preview_cb(camera_handle_, callback, data);
}

int CameraHandle::CameraUnsetPreviewCb() {
  return camera_unset_preview_cb(camera_handle_);
}

int CameraHandle::CameraSetMediapacketPreviewCb(
    camera_media_packet_preview_cb callback,
    void* data) {
  return camera_set_media_packet_preview_cb(camera_handle_, callback, data);
}

int CameraHandle::CameraUnsetMediapacketPreviewCb() {
  return camera_unset_media_packet_preview_cb(camera_handle_);
}

int CameraHandle::CameraSetStateChangedCb(camera_state_changed_cb callback,
                                          void* data) {
  return camera_set_state_changed_cb(camera_handle_, callback, data);
}

int CameraHandle::CameraUnsetStateChangedCb() {
  return camera_unset_state_changed_cb(camera_handle_);
}

int CameraHandle::CameraSetErrorCb(camera_error_cb callback, void* data) {
  return camera_set_error_cb(camera_handle_, callback, data);
}

int CameraHandle::CameraUnsetErrorCb() {
  return camera_unset_error_cb(camera_handle_);
}

int CameraHandle::CameraSupportedFormatsCB(
    camera_supported_capture_format_cb callback,
    void* data) {
  return camera_foreach_supported_capture_format(camera_handle_, callback,
                                                 data);
}

int CameraHandle::CameraSupportedCaptureResolution(
    camera_supported_capture_resolution_cb callback,
    void* data) {
  return camera_foreach_supported_capture_resolution(camera_handle_, callback,
                                                     data);
}

int CameraHandle::CameraSetStreamFlip(camera_flip_e flip) {
  return camera_attr_set_stream_flip(camera_handle_, flip);
}

}  // namespace media
