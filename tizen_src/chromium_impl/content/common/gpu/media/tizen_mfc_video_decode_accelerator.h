// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_GPU_MEDIA_TIZEN_MFC_VIDEO_DECODE_ACCELERATOR_H_
#define CONTENT_COMMON_GPU_MEDIA_TIZEN_MFC_VIDEO_DECODE_ACCELERATOR_H_

#include "content/common/media/tizen_omx_video_decoder.h"
#include "content/common/tizen_resource_manager.h"

#include <vector>

#include "tbm_surface.h"
#include "tbm_surface_internal.h"

#include "ui/gl/egl_util.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"

// It needs to be included after rest of GL stuff
#include "EGL/eglext.h"

namespace content {

class CONTENT_EXPORT TizenMfcVideoDecodeAccelerator
    : public content::TizenOmxVideoDecodeAccelerator {
 public:
  TizenMfcVideoDecodeAccelerator(
      EGLDisplay egl_display,
      EGLContext egl_context,
      const base::Callback<bool(void)>& make_context_current);

  // VideoDecoder implementation
  void AssignPictureBuffers(
      const std::vector<media::PictureBuffer>& buffers) override;
  void ReusePictureBuffer(int32_t picture_buffer_id) override;

 private:
  // Auto-destruction reference for EGLSync (for message-passing).
  struct EGLSyncKHRRef;

  struct MfcOutputRecord : public OutputRecord {
    MfcOutputRecord();
    ~MfcOutputRecord() override = default;
    tbm_surface_h tbm_surf;
    EGLImageKHR egl_image;  // EGLImageKHR for the output buffer.
    EGLSyncKHR egl_sync;    // sync the compositor's use of the EGLImage.
  };

  // Output related functions
  unsigned TexturesPerPictureBuffer() const override;
  media::VideoPixelFormat VideoPixelFormat() const override;
  uint32_t TextureTarget() const override;
  bool ProcessOutputPicture(media::Picture* picture,
                            DecodedVideoFrame* last_frame,
                            OutputRecord* free_record) override;
  void ReusePictureBufferTask(int32_t picture_buffer_id,
                              std::unique_ptr<EGLSyncKHRRef> egl_sync_ref);

  // TizenResourceManager related functions
  bool FillRequestedResources(
      std::vector<TizenRequestedResource>* devices) override;

  // Buffer stuff
  unsigned OutputRecordsCount() const override;
  unsigned InputFrameBufferSize() const override;
  unsigned InputFrameBuffersCount() const override;
  void DestroyOutputRecords() override;
  bool DestroyOutputRecord(int idx) override;
  MfcOutputRecord* AllocateOutputRecord() override;
  bool InitOutputRecord(const media::PictureBuffer& picture_buffer,
                        OutputRecord* new_record) override;
  bool WaitForSyncKHR(MfcOutputRecord* rec);
  bool CopyOmxBufToTbm(const DecodedVideoFrame* frame_info,
                       const MfcOutputRecord* rec);
  bool CreateEGLImage(EGLDisplay egl_display,
                      EGLContext egl_context,
                      gfx::Size frame_buffer_size,
                      MfcOutputRecord* rec);
  bool InitTbmSurface(MfcOutputRecord* rec, const PictureBufferDesc& desc);

  // Other functions
  bool EarlyInit() override;
  bool PostInit() override;
  std::vector<media::VideoCodecProfile> SupportedProfiles() const override;

  base::Callback<bool(void)> make_context_current_;
  EGLDisplay egl_display_;
  EGLContext egl_context_;

  tbm_bufmgr tbm_bufmgr_{nullptr};

  ~TizenMfcVideoDecodeAccelerator() override;

  DISALLOW_COPY_AND_ASSIGN(TizenMfcVideoDecodeAccelerator);
};

}  // namespace content

#endif  // CONTENT_COMMON_GPU_MEDIA_TIZEN_MFC_VIDEO_DECODE_ACCELERATOR_H_
