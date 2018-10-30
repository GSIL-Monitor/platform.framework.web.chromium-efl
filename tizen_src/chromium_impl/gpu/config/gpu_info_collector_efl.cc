// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/gpu_info_collector.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "gpu/config/scoped_restore_non_owned_egl_context.h"

#if defined(TIZEN_MULTIMEDIA_SUPPORT)
#include "content/public/browser/gpu_utils.h"
#include "media/gpu/ipc/service/gpu_video_decode_accelerator.h"
#include "media/gpu/ipc/service/gpu_video_encode_accelerator.h"
#endif

namespace {

std::string GetDriverVersionFromString(const std::string& version_string) {
  // Extract driver version from the second number in a string like:
  // "OpenGL ES 2.0 V@6.0 AU@ (CL@2946718)"

  // Exclude first "2.0".
  size_t begin = version_string.find_first_of("0123456789");
  if (begin == std::string::npos)
    return "0";
  size_t end = version_string.find_first_not_of("01234567890.", begin);

  // Extract number of the form "%d.%d"
  begin = version_string.find_first_of("0123456789", end);
  if (begin == std::string::npos)
    return "0";
  end = version_string.find_first_not_of("01234567890.", begin);
  std::string sub_string;
  if (end != std::string::npos)
    sub_string = version_string.substr(begin, end - begin);
  else
    sub_string = version_string.substr(begin);
  std::vector<std::string> pieces = base::SplitString(
      sub_string, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (pieces.size() < 2)
    return "0";
  return pieces[0] + "." + pieces[1];
}

}  // namespace

namespace gpu {

CollectInfoResult CollectBasicGraphicsInfo(GPUInfo* gpu_info) {
  DCHECK(gpu_info);

  gpu_info->basic_info_state = kCollectInfoSuccess;
  // TODO(c.padhi) If required, figure out a way to collect basic GPU info
  // without creating a GL context.
  return kCollectInfoSuccess;
}

CollectInfoResult CollectContextGraphicsInfo(GPUInfo* gpu_info) {
  DCHECK(gpu_info);

  // Create a short-lived context on the GPU thread to collect the GL strings.
  // Make sure we restore the existing context if there is one.
  ScopedRestoreNonOwnedEGLContext restore_context;
  CollectInfoResult result = CollectGraphicsInfoGL(gpu_info);
  gpu_info->basic_info_state = result;
  gpu_info->context_info_state = result;
#if defined(TIZEN_MULTIMEDIA_SUPPORT)
  gpu::GpuPreferences gpu_preferences =
      content::GetGpuPreferencesFromCommandLine();
  gpu::GpuDriverBugWorkarounds work_arounds;
  gpu_info->video_encode_accelerator_supported_profiles =
      media::GpuVideoEncodeAccelerator::GetSupportedProfiles(gpu_preferences);
  gpu_info->video_decode_accelerator_capabilities =
      media::GpuVideoDecodeAccelerator::GetCapabilities(gpu_preferences,
                                                        work_arounds);
#endif
  return result;
}

CollectInfoResult CollectDriverInfoGL(GPUInfo* gpu_info) {
  DCHECK(gpu_info);
  gpu_info->driver_version = GetDriverVersionFromString(gpu_info->gl_version);
  gpu_info->gpu.vendor_string = gpu_info->gl_vendor;
  gpu_info->gpu.device_string = gpu_info->gl_renderer;
  return kCollectInfoSuccess;
}

void MergeGPUInfo(GPUInfo* basic_gpu_info, const GPUInfo& context_gpu_info) {
  MergeGPUInfoGL(basic_gpu_info, context_gpu_info);
}

}  // namespace gpu
