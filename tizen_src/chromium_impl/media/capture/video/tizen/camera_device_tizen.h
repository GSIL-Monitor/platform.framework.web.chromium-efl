// Copyright 2016 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_TIZEN_CAMERA_DEVICE_TIZEN_H_
#define MEDIA_CAPTURE_VIDEO_TIZEN_CAMERA_DEVICE_TIZEN_H_

#include <camera.h>
#include <map>
#include <vector>

#include "base/memory/singleton.h"
#include "media/base/video_types.h"
#include "media/capture/mojo/image_capture.mojom.h"
#include "ui/gfx/gpu_memory_buffer.h"

using media::mojom::MeteringMode;

namespace media {

struct VideoCaptureFormat;
typedef std::vector<VideoCaptureFormat> VideoCaptureFormats;

struct CameraCapability {
  typedef gfx::Size Resolution;
  typedef int Fps;

  CameraCapability() : fps(0) {}

  CameraCapability(gfx::Size frame, int rate) : resolution(frame), fps(rate) {}

  Resolution resolution;
  Fps fps;
};

struct Range {
  int min = -1;
  int max = -1;
};

class PhotoCapabilities {
 public:
  mojom::RedEyeReduction GetRedEyeReduction();
  mojom::MeteringMode GetWhiteBalanceMode() {
    return mojom::MeteringMode::NONE;
  }
  mojom::MeteringMode GetExposureMode() { return MeteringMode::NONE; }
  mojom::MeteringMode GetFocusMode() { return MeteringMode::NONE; }
  mojom::RangePtr GetColorTemperature() { return mojom::Range::New(); }
  mojom::RangePtr GetExposureCompensation();
  mojom::RangePtr GetIso();
  mojom::RangePtr GetBrightness();
  mojom::RangePtr GetContrast();
  mojom::RangePtr GetSaturation() { return mojom::Range::New(); }
  mojom::RangePtr GetSharpness() { return mojom::Range::New(); }
  mojom::RangePtr GetImageHeight();
  mojom::RangePtr GetImageWidth();
  mojom::RangePtr GetZoom();

 private:
  PhotoCapabilities(camera_h camera) : camera_(camera) {}

  camera_h camera_;
  Range exposureCompensation_;
  Range brightness_;
  Range contrast_;
  Range zoom_;

  friend class CameraHandle;
};

class CameraHandleClient {
 public:
  virtual void OnStreamStopped() {}

 protected:
  virtual ~CameraHandleClient() {}
};

class CameraHandle final {
 public:
  static CameraHandle* GetInstance();

  camera_device_e GetDeviceName() { return device_name_; };

  bool IsValid();
  bool GetSupportedPreviewPixelFormats(
      std::vector<media::VideoPixelFormat>& supported_formats) const;
  void GetDeviceSupportedFormats(media::VideoCaptureFormats& supported_formats);
  int GetMaxFrameRate(CameraCapability::Resolution) const;
  int GetDeviceCounts();
  void ResetHandle(camera_device_e device_name);
  void SetClient(CameraHandleClient* client);
  void UnsetClient(CameraHandleClient* client);

  // Set Photo Capabilities
  bool SetZoom(int value);
  bool SetResolution(int width, int height);
  bool SetBrightness(int value);
  bool SetContrast(int value);
  bool SetIso(int value);
  bool SetExposure(int value);

  std::unique_ptr<PhotoCapabilities> capabilities_;

  int CameraStartCapture(camera_capturing_cb capturingCB,
                         camera_capture_completed_cb completedCB,
                         void* data);
  int CameraSetCaptureFormat(camera_pixel_format_e format);
  int CameraSetCaptureResolution(int width, int height);

  // camera apis
  int CameraStartPreview();
  int CameraStopPreview();

  // getter
  camera_state_e CameraGetState();
  int CameraGetPreviewResolution(int* width, int* height);
  int CameraGetPreviewFormat(camera_pixel_format_e* format);
  camera_attr_fps_e CameraGetPreviewFps();

  // setter
  int CameraSetDisplay(camera_display_type_e type, camera_display_h display);
  int CameraSetPreviewResolution(int width, int height);
  int CameraSetPreviewFormat(camera_pixel_format_e format);
  int CameraSetPreviewFps(camera_attr_fps_e fps);

  // callbacks
  int CameraSetPreviewCb(camera_preview_cb callback, void* data);
  int CameraUnsetPreviewCb();
  int CameraSetMediapacketPreviewCb(camera_media_packet_preview_cb callback,
                                    void* data);
  int CameraUnsetMediapacketPreviewCb();
  int CameraSetStateChangedCb(camera_state_changed_cb callback, void* data);
  int CameraUnsetStateChangedCb();
  int CameraSetErrorCb(camera_error_cb callback, void* data);
  int CameraUnsetErrorCb();

  int CameraSupportedFormatsCB(camera_supported_capture_format_cb callback,
                               void* data);
  int CameraSupportedCaptureResolution(
      camera_supported_capture_resolution_cb callback,
      void* data);
  int CameraSetStreamFlip(camera_flip_e flip);

 private:
  CameraHandle();
  ~CameraHandle();

  bool GetSupportedPreviewResolutions(
      std::vector<CameraCapability::Resolution>& supported_resolutions) const;
  bool GetSupportedPreviewCapabilities(std::vector<CameraCapability>&) const;

  camera_h camera_handle_;
  camera_device_e device_name_;
  CameraHandleClient* client_;

  friend struct base::DefaultSingletonTraits<CameraHandle>;
  DISALLOW_COPY_AND_ASSIGN(CameraHandle);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_TIZEN_CAMERA_DEVICE_TIZEN_H_
