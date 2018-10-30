// Copyright 2014 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implementation of a VideoCaptureDeviceFactoryTizen class.

#ifndef MEDIA_VIDEO_CAPTURE_TIZEN_VIDEO_CAPTURE_DEVICE_FACTORY_TIZEN_H_
#define MEDIA_VIDEO_CAPTURE_TIZEN_VIDEO_CAPTURE_DEVICE_FACTORY_TIZEN_H_

#include "media/capture/video/video_capture_device_factory.h"

namespace media {

// Extension of VideoCaptureDeviceFactory to create and manipulate Tizen
// devices.
class MEDIA_EXPORT VideoCaptureDeviceFactoryTizen
    : public VideoCaptureDeviceFactory {
 public:
  explicit VideoCaptureDeviceFactoryTizen(
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner);
  ~VideoCaptureDeviceFactoryTizen() override;

  std::unique_ptr<VideoCaptureDevice> CreateDevice(
      const VideoCaptureDeviceDescriptor& device_descriptor) override;
  void GetDeviceDescriptors(
      VideoCaptureDeviceDescriptors* device_descriptors) override;
  void GetSupportedFormats(
      const VideoCaptureDeviceDescriptor& device_descriptor,
      VideoCaptureFormats* supported_formats) override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureDeviceFactoryTizen);
};

}  // namespace media

#endif  // MEDIA_VIDEO_CAPTURE_TIZEN_VIDEO_CAPTURE_DEVICE_FACTORY_TIZEN_H_
