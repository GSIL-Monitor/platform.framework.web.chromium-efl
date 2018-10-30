// Copyright 2014 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/gpu/media/tizen_mfc_video_decode_accelerator.h"

#include <vector>

#include "base/location.h"
#include "content/common/media/tizen_omx_video_decoder_utils.h"
#include "media/base/video_codecs.h"
#include "ui/gl/scoped_binders.h"

#ifndef EGL_NATIVE_SURFACE_TIZEN
#define EGL_NATIVE_SURFACE_TIZEN 0x32A1
#endif

namespace {

const unsigned kNumberOfTexturerPerPictureBuffer = 1;
const uint32_t kDefaultTextureTarget = GL_TEXTURE_EXTERNAL_OES;
const int kMaxDecodedWidth = 1920;   // FHD
const int kMaxDecodedHeight = 1080;  // FHD
const unsigned kOutputRecordCount = 8;
const int kInputFrameBufferSize = 1024 * 1024 * 2;
// Value of 12 proved smooth playback with NaCL applications
const int kInputFrameBuffersCount = 12;
const std::vector<media::VideoCodecProfile> kSupportedProfiles{
    media::H264PROFILE_BASELINE, media::H264PROFILE_MAIN,
    media::H264PROFILE_EXTENDED, media::H264PROFILE_HIGH,
    media::VP8PROFILE_ANY,
};

}  // namespace

namespace content {

media::VideoDecodeAccelerator* CreateTizenMfcVideoDecodeAccelerator(
    EGLDisplay egl_display,
    EGLContext egl_context,
    const base::Callback<bool(void)>& make_context_current) {
  return new TizenMfcVideoDecodeAccelerator(egl_display, egl_context,
                                            make_context_current);
}

media::VideoDecodeAccelerator::SupportedProfiles
GetSupportedTizenMfcProfiles() {
  media::VideoDecodeAccelerator::SupportedProfiles profiles;
  for (auto profile : kSupportedProfiles) {
    media::VideoDecodeAccelerator::SupportedProfile tmp_profile;
    tmp_profile.profile = profile;
    tmp_profile.min_resolution.SetSize(0, 0);
    tmp_profile.max_resolution.SetSize(kMaxDecodedWidth, kMaxDecodedHeight);
    profiles.push_back(tmp_profile);
  }
  return profiles;
}

TizenMfcVideoDecodeAccelerator::MfcOutputRecord::MfcOutputRecord()
    : TizenOmxVideoDecodeAccelerator::OutputRecord(),
      tbm_surf(nullptr),
      egl_image(EGL_NO_IMAGE_KHR),
      egl_sync(EGL_NO_SYNC_KHR) {}

struct TizenMfcVideoDecodeAccelerator::EGLSyncKHRRef {
  EGLSyncKHRRef(EGLDisplay egl_display, EGLSyncKHR egl_sync);
  ~EGLSyncKHRRef();
  EGLDisplay const egl_display;
  EGLSyncKHR egl_sync;
};

TizenMfcVideoDecodeAccelerator::EGLSyncKHRRef::EGLSyncKHRRef(
    EGLDisplay egl_display,
    EGLSyncKHR egl_sync)
    : egl_display(egl_display), egl_sync(egl_sync) {}

TizenMfcVideoDecodeAccelerator::EGLSyncKHRRef::~EGLSyncKHRRef() {
  // We don't check for eglDestroySyncKHR failures, because if we get here
  // with a valid sync object, something went wrong and we are getting
  // destroyed anyway.
  if (egl_sync != EGL_NO_SYNC_KHR)
    eglDestroySyncKHR(egl_display, egl_sync);
}

TizenMfcVideoDecodeAccelerator::TizenMfcVideoDecodeAccelerator(
    EGLDisplay egl_display,
    EGLContext egl_context,
    const base::Callback<bool(void)>& make_context_current)
    : TizenOmxVideoDecodeAccelerator(),
      make_context_current_(make_context_current),
      egl_display_(egl_display),
      egl_context_(egl_context) {}

TizenMfcVideoDecodeAccelerator::~TizenMfcVideoDecodeAccelerator() {
  if (tbm_bufmgr_ != nullptr) {
    tbm_bufmgr_deinit(tbm_bufmgr_);
  }
}

unsigned TizenMfcVideoDecodeAccelerator::OutputRecordsCount() const {
  return kOutputRecordCount;
}

std::vector<media::VideoCodecProfile>
TizenMfcVideoDecodeAccelerator::SupportedProfiles() const {
  return kSupportedProfiles;
}

unsigned TizenMfcVideoDecodeAccelerator::InputFrameBufferSize() const {
  return kInputFrameBufferSize;
}

unsigned TizenMfcVideoDecodeAccelerator::InputFrameBuffersCount() const {
  return kInputFrameBuffersCount;
}

media::VideoPixelFormat TizenMfcVideoDecodeAccelerator::VideoPixelFormat()
    const {
  // Mfc decoder supports only one pixel data layout which is NV12.
  return media::VideoPixelFormat::PIXEL_FORMAT_NV12;
}

unsigned TizenMfcVideoDecodeAccelerator::TexturesPerPictureBuffer() const {
  // Chromium uses several textures per one picture to store each YUV plane
  // in each texture. Later shaders are used to draw them properly in RGB.
  //
  // OpenGL libraries on some TV targets (e.g JazzM, KantM, KantM2) natively
  // supports YUV textures when target GL_TEXTURE_EXTERNAL_OES is used.
  // Backed with native pixel data buffer (TBM surface) allows for drawing
  // with proper sampler (samplerExternalOES) directly to RGB, while YUV>RGB
  // conversion happening silently in GL libraries.
  return kNumberOfTexturerPerPictureBuffer;
}

uint32_t TizenMfcVideoDecodeAccelerator::TextureTarget() const {
  return kDefaultTextureTarget;
}

bool TizenMfcVideoDecodeAccelerator::FillRequestedResources(
    std::vector<TizenRequestedResource>* devices) {
  const media::VideoCodec codec =
      media::VideoCodecProfileToVideoCodec(video_profile_);
  if (codec == media::kUnknownVideoCodec)
    return false;

  // Request decoder that can fulfill maximum possible limits of resolution
  // as decoded stream can switch resolution at runtime.
  TizenVideoDecoderDescription decoder_desc;
  decoder_desc.codec = codec;
  decoder_desc.width = kMaxDecodedWidth;
  decoder_desc.height = kMaxDecodedHeight;
  // MFC decoder is able to decode up to 60fps in FHD. However every instance
  // works on timesharing basis of HW core. With two instances in total,
  // if we request more than 30, RM assumes that only one instance is available
  // in system to fulfill >30fps, as second one would slow down the first.
  decoder_desc.framerate = 30;
  decoder_desc.bpp = -1;  // use default
  decoder_desc.sampling_format = VIDEO_DECODER_SAMPLING_DEFAULT;

  devices->resize(devices->size() + 1);
  if (!TizenResourceManager::PrepareVideoDecoderRequest(
          &decoder_desc, &devices->back(), false)) {
    LOG(ERROR) << "Failed to get video decoder resource request";
    return false;
  }

  return true;
}

bool TizenMfcVideoDecodeAccelerator::EarlyInit() {
  tbm_bufmgr_ = tbm_bufmgr_init(-1);
  if (!tbm_bufmgr_) {
    LOG(ERROR) << "Failed to initialize default instance of tbm buffer manager";
    return false;
  }

  return true;
}

bool TizenMfcVideoDecodeAccelerator::PostInit() {
  // This implementation can work only with mfc decoder.
  // Check if Resource Manager returned component that was expected.
  if (omx_component_name_.find("mfc") == std::string::npos) {
    LOG(ERROR) << "Wrong decoder component was selected: "
               << omx_component_name_;
    return false;
  }

  return true;
}

void TizenMfcVideoDecodeAccelerator::ReusePictureBufferTask(
    int32_t picture_buffer_id,
    std::unique_ptr<EGLSyncKHRRef> egl_sync_ref) {
  DCHECK_EQ(decoder_output_thread_.message_loop(),
            base::MessageLoop::current());

  base::AutoLock lock(output_ctx_lock_);
  OutputContext& ctx = output_ctx_;
  if (ctx.is_error.load()) {
    LOG(ERROR) << "Early out: output ctx is error";
    return;
  }

  size_t index;
  for (index = 0; index < output_records_.size(); ++index)
    if (output_records_[index]->is_mapped &&
        output_records_[index]->picture_id == picture_buffer_id)
      break;

  if (index >= output_records_.size()) {
    // It's possible that we've already posted a DismissPictureBuffer for this
    // picture, but it has not yet executed when this ReusePictureBuffer was
    // posted to us by the client. In that case just ignore this (we've already
    // dismissed it and accounted for that) and let the sync object get
    // destroyed.
    LOG(ERROR) << "ReusePictureBufferTask(): got picture id= "
               << picture_buffer_id << " not in use (anymore?).";
    return;
  }

  MfcOutputRecord* output_record =
      static_cast<MfcOutputRecord*>(output_records_[index].get());
  if (!output_record->at_client) {
    LOG(ERROR) << "ReusePictureBufferTask(): picture_buffer_id not reusable"
               << " index=" << index
               << " at_client=" << output_record->at_client
               << " is_mapped=" << output_record->is_mapped;
    SetErrorState(FROM_HERE, INVALID_ARGUMENT);
    return;
  }

  DCHECK_EQ(output_record->egl_sync, EGL_NO_SYNC_KHR);
  output_record->at_client = false;
  output_record->egl_sync = egl_sync_ref->egl_sync;
  ctx.output_records_free.push_back(index);
  // Take ownership of the EGLSync.
  egl_sync_ref->egl_sync = EGL_NO_SYNC_KHR;

  if (ctx.decoder_state == kResetting) {
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(
            base::IgnoreResult(&TizenMfcVideoDecodeAccelerator::ResetDoneTask),
            base::Unretained(this)));
  } else if (output_record->should_destroy) {
    LOG(ERROR) << "Scheduling destroy of output record " << index;
    ctx.output_records_free.pop_back();
    child_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&TizenMfcVideoDecodeAccelerator::PruneOutputRecordsTask,
                   base::Unretained(this)));
  } else if (!ctx.last_frame.is_sent &&
             output_record->buffer_desc == PictureBufferDesc(&ctx.last_frame)) {
    // there was decoded frame posted via DoFillBufferDone
    // but not sent to client due to lack of free output buffers.
    // Process it now.
    ProcessLastFrame();
  }
}

void TizenMfcVideoDecodeAccelerator::ReusePictureBuffer(
    int32_t picture_buffer_id) {
  // Must be run on child thread, as we'll insert a sync in the EGL context.
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  if (!make_context_current_.Run()) {
    LOG(ERROR) << "Could not make context current";
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return;
  }

  // ReusePictureBuffer called so client has finished issuing any gl commands
  // regarding associated texture. Those commands however can be still be in
  // pipeline. Insert sync object so we can later wait till commands are
  // actually consumed by pipeline.
  EGLSyncKHR egl_sync = EGL_NO_SYNC_KHR;
#if defined(ARCH_CPU_ARMEL)
  egl_sync = eglCreateSyncKHR(egl_display_, EGL_SYNC_FENCE_KHR, NULL);
  if (egl_sync == EGL_NO_SYNC_KHR) {
    LOG(ERROR) << "ReusePictureBuffer(): eglCreateSyncKHR() failed";
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return;
  }
#endif

  std::unique_ptr<EGLSyncKHRRef> egl_sync_ref(
      new EGLSyncKHRRef(egl_display_, egl_sync));
  decoder_output_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&TizenMfcVideoDecodeAccelerator::ReusePictureBufferTask,
                 base::Unretained(this), picture_buffer_id,
                 base::Passed(&egl_sync_ref)));
}

bool TizenMfcVideoDecodeAccelerator::CopyOmxBufToTbm(
    const DecodedVideoFrame* frame_info,
    const MfcOutputRecord* rec) {
  tbm_surface_h surf = rec->tbm_surf;
  tbm_bo bos[2] = {0};
  void* ptrs[2] = {0};

  DCHECK_EQ(tbm_surface_internal_get_num_bos(surf), 2);

  // get bo's
  // Y plane
  bos[0] = tbm_surface_internal_get_bo(surf, 0);
  if (!bos[0]) {
    LOG(ERROR) << "Faled to get bo for Y plane";
    return false;
  }
  // UV plane
  bos[1] = tbm_surface_internal_get_bo(surf, 1);
  if (!bos[1]) {
    LOG(ERROR) << "Faled to get bo for UV plane";
    return false;
  }

  // map bo's
  // Y plane
  tbm_bo_handle handle = tbm_bo_map(bos[0], TBM_DEVICE_CPU, TBM_OPTION_WRITE);
  ptrs[0] = handle.ptr;
  if (!ptrs[0]) {
    LOG(ERROR) << "Failed to map bo for Y plane";
    return false;
  }
  // UV plane
  handle = tbm_bo_map(bos[1], TBM_DEVICE_CPU, TBM_OPTION_WRITE);
  ptrs[1] = handle.ptr;
  if (!ptrs[1]) {
    LOG(ERROR) << "Failed to map bo for UV plane";
    tbm_bo_unmap(bos[0]);
    return false;
  }

  const int height = frame_info->res.height();
  const int y_size = height * frame_info->y_linesize;
  const int uv_size = (height * frame_info->u_linesize / 2);

  DCHECK(y_size <= tbm_bo_size(bos[0]));
  DCHECK(uv_size <= tbm_bo_size(bos[1]));

  // copy planes data
  memcpy(ptrs[0], reinterpret_cast<void*>(frame_info->y_virtaddr), y_size);
  memcpy(ptrs[1], reinterpret_cast<void*>(frame_info->u_virtaddr), uv_size);

  tbm_bo_unmap(bos[0]);
  tbm_bo_unmap(bos[1]);

  return true;
}

bool TizenMfcVideoDecodeAccelerator::WaitForSyncKHR(MfcOutputRecord* rec) {
  // We are not called on child thread, however below fence functions are
  // allowed to be called on any thread.
  if (rec->egl_sync != EGL_NO_SYNC_KHR) {
    // If we have to wait for completion, wait.  Note that
    // free_output_buffers_ is a FIFO queue, so we always wait on the
    // buffer that has been in the queue the longest.
    if (eglClientWaitSyncKHR(egl_display_, rec->egl_sync, 0, EGL_FOREVER_KHR) ==
        EGL_FALSE) {
      // This will cause tearing, but is safe otherwise.
      LOG(ERROR) << " eglClientWaitSyncKHR failed!";
      return true;
    }
    if (eglDestroySyncKHR(egl_display_, rec->egl_sync) != EGL_TRUE) {
      LOG(ERROR) << " eglDestroySyncKHR failed!";
      return false;
    }
    rec->egl_sync = EGL_NO_SYNC_KHR;
  }
  return true;
}

bool TizenMfcVideoDecodeAccelerator::ProcessOutputPicture(
    media::Picture* picture,
    DecodedVideoFrame* last_frame,
    OutputRecord* free_record) {
  MfcOutputRecord* mfc_rec = static_cast<MfcOutputRecord*>(free_record);

  // We must wait on sync object before changing pixel data buffer that is
  // backing client texture.
  if (!WaitForSyncKHR(mfc_rec) || !CopyOmxBufToTbm(last_frame, mfc_rec))
    return false;

  return true;
}

bool TizenMfcVideoDecodeAccelerator::DestroyOutputRecord(int idx) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  output_ctx_lock_.AssertAcquired();
  MfcOutputRecord* rec =
      static_cast<MfcOutputRecord*>(output_records_[idx].get());

  // Do nothing if not mapped
  if (!rec->is_mapped)
    return true;

  bool success = true;
  if (rec->at_client) {
    LOG(ERROR) << "About to destroy texture that is still at the client!"
               << " Crash is very probable if it's still drawn somewhere"
               << " idx=" << idx << " picture_id=" << rec->picture_id;
  }

  if (rec->egl_image != EGL_NO_IMAGE_KHR &&
      (eglDestroyImageKHR(egl_display_, rec->egl_image) != EGL_TRUE)) {
    LOG(ERROR) << "Failed to destroy EGLImage on output record " << idx;
    success = false;
  }
  rec->egl_image = EGL_NO_IMAGE_KHR;

  if (rec->egl_sync != EGL_NO_SYNC_KHR &&
      eglDestroySyncKHR(egl_display_, rec->egl_sync) != EGL_TRUE) {
    LOG(ERROR) << "Failed to destroy EGLSync on output record " << idx;
    success = false;
  }
  rec->egl_sync = EGL_NO_SYNC_KHR;

  if (rec->tbm_surf != nullptr &&
      tbm_surface_destroy(rec->tbm_surf) != TBM_SURFACE_ERROR_NONE) {
    LOG(ERROR) << "Failed to destroy TBM surface on output record " << idx;
    success = false;
  }
  rec->tbm_surf = nullptr;

  if (!TizenOmxVideoDecodeAccelerator::DestroyOutputRecord(idx))
    success = false;

  output_records_[idx].reset();

  return success;
}

void TizenMfcVideoDecodeAccelerator::DestroyOutputRecords() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  // Destroying each MfcOutputRecord will require GL calls
  if (!make_context_current_.Run()) {
    LOG(ERROR) << "Could not make context current."
                  " A leak of EGLImages will happen.";
    return;
  }

  TizenOmxVideoDecodeAccelerator::DestroyOutputRecords();
}

bool TizenMfcVideoDecodeAccelerator::CreateEGLImage(EGLDisplay egl_display,
                                                    EGLContext egl_context,
                                                    gfx::Size frame_buffer_size,
                                                    MfcOutputRecord* rec) {
  EGLint attrs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_FALSE, EGL_NONE};
  tbm_surface_h surface = rec->tbm_surf;

  if (rec->egl_image != EGL_NO_IMAGE_KHR)
    return false;

  EGLImageKHR egl_image =
      eglCreateImageKHR(egl_display, egl_context, EGL_NATIVE_SURFACE_TIZEN,
                        static_cast<EGLClientBuffer>(surface), &attrs[0]);
  if (egl_image == EGL_NO_IMAGE_KHR) {
    LOG(ERROR) << "Failed creating EGL image: " << ui::GetLastEGLErrorString();
    return false;
  }
  glBindTexture(TextureTarget(), rec->texture_id);
  glEGLImageTargetTexture2DOES(TextureTarget(), egl_image);

  rec->egl_image = egl_image;

  return true;
}

bool TizenMfcVideoDecodeAccelerator::InitTbmSurface(
    MfcOutputRecord* rec,
    const PictureBufferDesc& desc) {
  tbm_bo bos[2] = {0};
  const int height = desc.size.height();

  // Y plane
  const int y_size = height * desc.y_linesize;
  bos[0] = tbm_bo_alloc(tbm_bufmgr_, y_size, TBM_BO_DEFAULT);
  if (!bos[0])
    return false;

  // UV plane
  const int uv_size = (height / 2 * desc.u_linesize);
  bos[1] = tbm_bo_alloc(tbm_bufmgr_, uv_size, TBM_BO_DEFAULT);
  if (!bos[1]) {
    tbm_bo_unref(bos[0]);
    return false;
  }

  tbm_surface_info_s tbmsi = {0};
  tbmsi.bpp = tbm_surface_internal_get_bpp(TBM_FORMAT_NV12);
  tbmsi.num_planes = tbm_surface_internal_get_num_planes(TBM_FORMAT_NV12);
  tbmsi.bpp = tbm_surface_internal_get_bpp(TBM_FORMAT_NV12);
  tbmsi.format = TBM_FORMAT_NV12;
  tbmsi.width = desc.size.width();
  tbmsi.height = height;

  // Y plane
  tbmsi.planes[0].offset = 0;
  tbmsi.planes[0].size = y_size;
  tbmsi.planes[0].stride = desc.y_linesize;
  tbmsi.size += y_size;

  // UV plane
  tbmsi.planes[1].offset = 0;
  tbmsi.planes[1].size = uv_size;
  tbmsi.planes[1].stride = desc.u_linesize;
  tbmsi.size += uv_size;

  rec->tbm_surf = tbm_surface_internal_create_with_bos(&tbmsi, bos, 2);
  if (!rec->tbm_surf) {
    LOG(ERROR) << "Failed to create tbm surface for output record";
    tbm_bo_unref(bos[0]);
    tbm_bo_unref(bos[1]);
    return false;
  }

  // Creating surface with bo's adds additional reference to bo's. Unref bo's
  // now so they will be released automatically when surface is destroyed.
  tbm_bo_unref(bos[0]);
  tbm_bo_unref(bos[1]);

  if (!tbm_surface_internal_is_valid(rec->tbm_surf))
    LOG(WARNING) << "Created invalid tbm surface";

  return true;
}

TizenMfcVideoDecodeAccelerator::MfcOutputRecord*
TizenMfcVideoDecodeAccelerator::AllocateOutputRecord() {
  return new MfcOutputRecord();
}

bool TizenMfcVideoDecodeAccelerator::InitOutputRecord(
    const media::PictureBuffer& picture_buffer,
    OutputRecord* new_rec) {
  MfcOutputRecord* mfc_rec = static_cast<MfcOutputRecord*>(new_rec);

  if (!InitTbmSurface(mfc_rec, mfc_rec->buffer_desc)) {
    LOG(ERROR) << "Failed to create tbm surface for output record with desc: "
               << mfc_rec->buffer_desc.ToString();
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return false;
  }
  if (!CreateEGLImage(egl_display_, EGL_NO_CONTEXT, mfc_rec->buffer_desc.size,
                      mfc_rec)) {
    LOG(ERROR) << "Failed to create egl image for output record with desc: "
               << mfc_rec->buffer_desc.ToString();
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return false;
  }

  return true;
}

void TizenMfcVideoDecodeAccelerator::AssignPictureBuffers(
    const std::vector<media::PictureBuffer>& buffers) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  if (output_ctx_.is_error.load()) {
    LOG(ERROR) << "Early out: output ctx is error";
    pictures_assigned_.Signal();
    return;
  }

  if (!make_context_current_.Run()) {
    LOG(ERROR) << "Could not make context current";
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    pictures_assigned_.Signal();
    return;
  }

  // Base implementation will call InitOutputRecord() that requires binding of
  // a texture. Take care that we don't change current binding.
  gl::ScopedTextureBinder bind_restore(TextureTarget(), 0);

  TizenOmxVideoDecodeAccelerator::AssignPictureBuffers(buffers);
}

}  // namespace content
