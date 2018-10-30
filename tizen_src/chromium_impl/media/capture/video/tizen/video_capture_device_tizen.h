// Copyright (c) 2012 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Tizen specific implementation of VideoCaptureDevice.
// Tizen Core API is used to capture the video frames from the device.

#ifndef MEDIA_VIDEO_CAPTURE_TIZEN_VIDEO_CAPTURE_DEVICE_TIZEN_H_
#define MEDIA_VIDEO_CAPTURE_TIZEN_VIDEO_CAPTURE_DEVICE_TIZEN_H_

#include "base/threading/thread.h"
#include "base/time/time.h"
#include "media/base/tizen/media_player_util_efl.h"
#include "media/capture/video/tizen/camera_device_tizen.h"
#include "media/capture/video/video_capture_device.h"

namespace media {

class VideoCaptureDeviceTizen : public VideoCaptureDevice,
                                public CameraHandleClient {
 public:
  const static std::string kFrontCameraName;
  const static std::string kBackCameraName;
  const static std::string kCameraId0;
  const static std::string kCameraId1;
  const static std::string GetFrontCameraID();
  const static std::string GetBackCameraID();

  explicit VideoCaptureDeviceTizen(
      const VideoCaptureDeviceDescriptor& device_descriptor);
  virtual ~VideoCaptureDeviceTizen() override;

  // Implements CameraHandleClient
  void OnStreamStopped() override;

  // Implements VideoCaptureDevice
  void AllocateAndStart(const VideoCaptureParams& params,
                        std::unique_ptr<Client> client) override;
  void StopAndDeAllocate() override;
  void GetPhotoState(GetPhotoStateCallback callback) override;
  void SetPhotoOptions(mojom::PhotoSettingsPtr settings,
                       SetPhotoOptionsCallback callback) override;
  void TakePhoto(TakePhotoCallback callback) override;

  static camera_device_e DeviceNameToCameraId(
      const VideoCaptureDeviceDescriptor& device_descriptor);

  static std::string GetCameraErrorMessage(int err_code);
  static void OnCameraCapturedWithMediaPacket(media_packet_h pkt, void* data);
  static void OnCameraCaptured(camera_preview_data_s* frame, void* data);

  static const char* GetErrorString(int err_code);
  static void CapturedCb(camera_image_data_s* image,
                         camera_image_data_s* postview,
                         camera_image_data_s* thumbnail,
                         void* userData);
  static void CaptureCompletedCb(void* userData);

 private:
  enum InternalState {
    kIdle,       // The device driver is opened but camera is not in use.
    kCapturing,  // Video is being captured.
    kError       // Error accessing HW functions.
                 // User needs to recover by destroying the object.
  };

  void OnAllocateAndStart(int width,
                          int height,
                          int frame_rate,
                          VideoPixelFormat format,
                          std::unique_ptr<Client> client);
  void OnStopAndDeAllocate();
  void OnCaptureCompletedCb();

  // For handling the camera preview callback returning mediapacket
  bool IsCapturing();
  void ChangeCapturingState(bool state);
  void PreviewCameraCaptured(ScopedMediaPacket pkt);

  void SetErrorState(const char* reason, const base::Location& location);

  CameraHandle* camera_instance_;
  std::unique_ptr<VideoCaptureDevice::Client> client_;
  VideoCaptureDevice::Client::Buffer buffer_;
  base::Lock capturing_state_lock_;

  VideoCaptureDeviceDescriptor device_descriptor_;
  base::Thread worker_;  // Thread used for reading data from the device.

  bool is_capturing_;
  bool use_media_packet_;
  InternalState state_;
  base::TimeTicks first_ref_time_;

  // List of |photo_callbacks_| in flight.
  std::list<std::unique_ptr<TakePhotoCallback>> photo_callbacks_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(VideoCaptureDeviceTizen);
};

}  // namespace media

#endif  // MEDIA_VIDEO_CAPTURE_TIZEN_VIDEO_CAPTURE_DEVICE_TIZEN_H_
