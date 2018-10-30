// Copyright 2014 The Samsung Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/tizen/video_capture_device_factory_tizen.h"

#include <camera.h>

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "media/capture/video/tizen/camera_device_tizen.h"
#include "media/capture/video/tizen/video_capture_device_tizen.h"

namespace media {

VideoCaptureDeviceFactoryTizen::VideoCaptureDeviceFactoryTizen(
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner)
    : ui_task_runner_(ui_task_runner) {}

VideoCaptureDeviceFactoryTizen::~VideoCaptureDeviceFactoryTizen() {}

std::unique_ptr<VideoCaptureDevice>
VideoCaptureDeviceFactoryTizen::CreateDevice(
    const VideoCaptureDeviceDescriptor& device_descriptor) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // FIXME: is device discriptor needed? or just the name is sufficient?
  return std::unique_ptr<VideoCaptureDevice>(
      new VideoCaptureDeviceTizen(device_descriptor));
}

void VideoCaptureDeviceFactoryTizen::GetDeviceDescriptors(
    VideoCaptureDeviceDescriptors* device_descriptors) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(device_descriptors->empty());
  CameraHandle* camera_handle = CameraHandle::GetInstance();
  if (camera_handle == nullptr || !camera_handle->IsValid()) {
    LOG(ERROR) << "Failed to get camera instance";
    return;
  }

  LOG(INFO) << "Received the camera handle:" << camera_handle;

  if (!camera_handle->IsValid()) {
    LOG(ERROR) << "Cannot use camera";
    return;
  }

  int device_count = camera_handle->GetDeviceCounts();
  if (device_count == 0) {
    LOG(ERROR) << "No camera on this device.";
    return;
  }
  VideoCaptureDeviceDescriptor primary_camera(
      VideoCaptureDeviceTizen::kFrontCameraName,
      VideoCaptureDeviceTizen::GetFrontCameraID(), VideoCaptureApi::UNKNOWN,
      VideoCaptureTransportType::OTHER_TRANSPORT);

  device_descriptors->push_back(primary_camera);
  if (device_count == 2) {
    VideoCaptureDeviceDescriptor secondary_camera(
        VideoCaptureDeviceTizen::kBackCameraName,
        VideoCaptureDeviceTizen::GetBackCameraID(), VideoCaptureApi::UNKNOWN,
        VideoCaptureTransportType::OTHER_TRANSPORT);
    device_descriptors->push_back(secondary_camera);
  }
}

void VideoCaptureDeviceFactoryTizen::GetSupportedFormats(
    const VideoCaptureDeviceDescriptor& device_descriptor,
    VideoCaptureFormats* supported_formats) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(supported_formats != nullptr);

  CameraHandle* camera_handle = CameraHandle::GetInstance();

  if (!camera_handle->IsValid()) {
    LOG(ERROR) << "Cannot use camera";
    return;
  }
  camera_handle->GetDeviceSupportedFormats(*supported_formats);
}

// static
VideoCaptureDeviceFactory*
VideoCaptureDeviceFactory::CreateVideoCaptureDeviceFactory(
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager) {
  return new VideoCaptureDeviceFactoryTizen(ui_task_runner);
}

}  // namespace media
