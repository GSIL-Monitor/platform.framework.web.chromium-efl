// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/media/tizen_omx_video_decoder.h"

#include "OMX_Component.h"
#include "linux/videodev2_tztv.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/common/media/tizen_omx_video_decoder_utils.h"
#include "media/base/bitstream_buffer.h"
#include "media/base/limits.h"
#include "media/base/video_types.h"

namespace content {

namespace {

const OMX_U32 kOmxInputPortIndex = 0;
const OMX_U32 kOmxOutputPortIndex = 1;
const uintptr_t kOutputBufferPrivate = 1337;
const int kComponentStateSwitchTimeout = 3;
const int kFlushBufferId = -2;
const int kSamsungPtsWorkAroundMagic = 5001;

// Entry points for OMX callbacks
OMX_ERRORTYPE OmxEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_PTR pAppData,
                              OMX_IN OMX_EVENTTYPE eEvent,
                              OMX_IN OMX_U32 nData1,
                              OMX_IN OMX_U32 nData2,
                              OMX_IN OMX_PTR pEventData) {
  TizenOmxVideoDecodeAccelerator* dec =
      reinterpret_cast<TizenOmxVideoDecodeAccelerator*>(pAppData);
  if (dec)
    dec->OnOmxEvent(hComponent, eEvent, nData1, nData2, pEventData);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE OmxEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                 OMX_IN OMX_PTR pAppData,
                                 OMX_IN OMX_BUFFERHEADERTYPE* pBuffer) {
  TizenOmxVideoDecodeAccelerator* dec =
      reinterpret_cast<TizenOmxVideoDecodeAccelerator*>(pAppData);
  if (dec)
    dec->DoEmptyBufferDone(pBuffer);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE OmxFillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent,
                                OMX_OUT OMX_PTR pAppData,
                                OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer) {
  TizenOmxVideoDecodeAccelerator* dec =
      reinterpret_cast<TizenOmxVideoDecodeAccelerator*>(pAppData);
  if (dec)
    dec->DoFillBufferDone(pBuffer);
  return OMX_ErrorNone;
}

inline struct v4l2_drm_dec_info* dec_info(
    const OMX_BUFFERHEADERTYPE* omx_buf_hdr) {
  struct v4l2_drm* drm =
      static_cast<struct v4l2_drm*>(omx_buf_hdr->pPlatformPrivate);
  return &drm->u.dec_info;
}

inline struct v4l2_private_frame_info* plane_info(
    const OMX_BUFFERHEADERTYPE* omx_buf_hdr,
    int plane_idx) {
  return &(dec_info(omx_buf_hdr)->pFrame[plane_idx]);
}

}  // namespace

struct TizenOmxVideoDecodeAccelerator::BitstreamBufferRef {
  BitstreamBufferRef(
      const base::WeakPtr<Client>& client,
      const scoped_refptr<base::SingleThreadTaskRunner>& client_task_runner,
      base::SharedMemory* shm,
      size_t size,
      int32_t input_id);
  ~BitstreamBufferRef();
  const base::WeakPtr<Client> client;
  const scoped_refptr<base::SingleThreadTaskRunner> client_task_runner;
  const std::unique_ptr<base::SharedMemory> shm;
  const size_t size;
  size_t bytes_used;
  const int32_t input_id;
};

TizenOmxVideoDecodeAccelerator::BitstreamBufferRef::BitstreamBufferRef(
    const base::WeakPtr<Client>& client,
    const scoped_refptr<base::SingleThreadTaskRunner>& client_task_runner,
    base::SharedMemory* shm,
    size_t size,
    int32_t input_id)
    : client(client),
      client_task_runner(client_task_runner),
      shm(shm),
      size(size),
      bytes_used(0),
      input_id(input_id) {}

TizenOmxVideoDecodeAccelerator::BitstreamBufferRef::~BitstreamBufferRef() {
  if (input_id >= 0) {
    client_task_runner->PostTask(
        FROM_HERE,
        base::Bind(&Client::NotifyEndOfBitstreamBuffer, client, input_id));
  }
}

TizenOmxVideoDecodeAccelerator::PictureBufferDesc::PictureBufferDesc(
    DecodedVideoFrame* frame)
    : size(frame->res),
      y_linesize(frame->y_linesize),
      u_linesize(frame->u_linesize),
      v_linesize(frame->v_linesize),
      is_valid(frame->IsValid() && frame->contains_picture_info) {}

TizenOmxVideoDecodeAccelerator::PictureBufferDesc::PictureBufferDesc(void)
    : y_linesize(0), u_linesize(0), v_linesize(0), is_valid(false) {}

bool TizenOmxVideoDecodeAccelerator::PictureBufferDesc::operator==(
    const PictureBufferDesc& other) {
  return ((this->size == other.size) &&
          (this->y_linesize == other.y_linesize) &&
          (this->u_linesize == other.u_linesize) &&
          (this->v_linesize == other.v_linesize));
}

bool TizenOmxVideoDecodeAccelerator::PictureBufferDesc::operator!=(
    const PictureBufferDesc& other) {
  return ((this->size != other.size) ||
          (this->y_linesize != other.y_linesize) ||
          (this->u_linesize != other.u_linesize) ||
          (this->v_linesize != other.v_linesize));
}

std::string TizenOmxVideoDecodeAccelerator::PictureBufferDesc::ToString() {
  std::string ret("buf valid:");
  ret += std::to_string(is_valid);
  ret += " res:";
  ret += size.ToString();
  ret += " y_stride:";
  ret += std::to_string(y_linesize);
  ret += " u_stride:";
  ret += std::to_string(u_linesize);
  ret += " v_stride:";
  ret += std::to_string(v_linesize);
  return ret;
}

TizenOmxVideoDecodeAccelerator::OutputRecord::OutputRecord()
    : at_client(false),
      is_mapped(false),
      picture_id(-1),
      texture_id(-1),
      should_destroy(false) {}

TizenOmxVideoDecodeAccelerator::OutputRecord::~OutputRecord() {}

TizenOmxVideoDecodeAccelerator::DecodedVideoFrame::DecodedVideoFrame()
    : timestamp(0),
      framerate(0),
      is_eos(false),
      show_frame(false),
      y_virtaddr(0),
      u_virtaddr(0),
      v_virtaddr(0),
      y_linesize(0),
      u_linesize(0),
      v_linesize(0),
      is_sent(false),
      contains_picture_info(false),
      is_valid(false) {}

TizenOmxVideoDecodeAccelerator::DecodedVideoFrame::DecodedVideoFrame(
    const DecodedVideoFrame& other) {
  // Video frame info
  timestamp = other.timestamp;
  res = other.res;
  effective_res = other.effective_res;
  next_res = other.next_res;
  framerate = other.framerate;
  is_eos = other.is_eos;
  show_frame = other.show_frame;
  // Buffer info
  y_virtaddr = other.y_virtaddr;
  u_virtaddr = other.u_virtaddr;
  v_virtaddr = other.v_virtaddr;
  y_linesize = other.y_linesize;
  u_linesize = other.u_linesize;
  v_linesize = other.v_linesize;
  is_sent = other.is_sent;
  contains_picture_info = other.contains_picture_info;
  is_valid = other.is_valid;
}

TizenOmxVideoDecodeAccelerator::DecodedVideoFrame::DecodedVideoFrame(
    const OMX_BUFFERHEADERTYPE* pBuffer)
    : timestamp(static_cast<int32_t>(pBuffer->nTimeStamp)),
      is_eos(pBuffer->nFlags & OMX_BUFFERFLAG_EOS),
      is_sent(false),
      is_valid(true) {
  if (pBuffer->pPlatformPrivate == nullptr) {
    contains_picture_info = false;
    // default constructors for: res, effective_res
    framerate = 0;
    show_frame = false;
    y_virtaddr = 0;
    u_virtaddr = 0;
    v_virtaddr = 0;
    y_linesize = 0;
    u_linesize = 0;
    v_linesize = 0;
  } else {
    contains_picture_info = true;
    res = gfx::Size(dec_info(pBuffer)->hres, dec_info(pBuffer)->vres);
    effective_res = gfx::Size(dec_info(pBuffer)->effective_hres,
                              dec_info(pBuffer)->effective_vres);
    next_res = gfx::Size(dec_info(pBuffer)->next_res.next_frm_hres,
                         dec_info(pBuffer)->next_res.next_frm_vres);
    framerate = dec_info(pBuffer)->framerate;
    show_frame = plane_info(pBuffer, 0)->showframe;
    y_virtaddr = plane_info(pBuffer, 0)->y_viraddr;
    u_virtaddr = plane_info(pBuffer, 0)->u_viraddr;
    v_virtaddr = plane_info(pBuffer, 0)->v_viraddr;
    y_linesize = plane_info(pBuffer, 0)->y_linesize;
    u_linesize = plane_info(pBuffer, 0)->u_linesize;
    v_linesize = plane_info(pBuffer, 0)->v_linesize;
  }
}

bool TizenOmxVideoDecodeAccelerator::DecodedVideoFrame::IsChangingResolution() {
  return next_res != res;
}

bool TizenOmxVideoDecodeAccelerator::DecodedVideoFrame::IsValid() {
  return is_valid;
}

void TizenOmxVideoDecodeAccelerator::NotifyError(Error error) {
  if (!child_task_runner_->BelongsToCurrentThread()) {
    child_task_runner_->PostTask(
        FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::NotifyError,
                              weak_this_factory_.GetWeakPtr(), error));
    return;
  }

  if (client_) {
    client_->NotifyError(error);
    client_ptr_factory_.reset();
  }
}

void TizenOmxVideoDecodeAccelerator::SetDecoderState(State new_state) {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());
  if (decoder_state_ == new_state)
    LOG(WARNING) << "Setting the same decoder state=" << new_state;

  decoder_state_ = new_state;
  base::AutoLock lock(output_ctx_lock_);
  output_ctx_.decoder_state = new_state;
}

void TizenOmxVideoDecodeAccelerator::SetErrorState(const base::Location& from,
                                                   Error error) {
  LOG(ERROR) << "Setting state (error=" << error
             << ") state in:" << from.function_name()
             << " line:" << from.line_number();
  DoSetErrorState(error);
}

void TizenOmxVideoDecodeAccelerator::DoSetErrorState(Error error) {
  // We can touch decoder_state_ only if this is the decoder thread or the
  // decoder thread isn't running.
  if (decoder_thread_.message_loop() != NULL &&
      decoder_thread_.message_loop() != base::MessageLoop::current()) {
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::DoSetErrorState,
                              base::Unretained(this), error));
    return;
  }

  // Post NotifyError only if we are already initialized, as the API does
  // not allow doing so before that.
  if (decoder_state_ != kError && decoder_state_ != kUninitialized &&
      decoder_state_ != kInitializing)
    NotifyError(error);

  input_ctx_.is_error.store(true);
  output_ctx_.is_error.store(true);
  decoder_state_ = kError;
}

TizenOmxVideoDecodeAccelerator::TizenOmxVideoDecodeAccelerator()
    : child_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_this_factory_(this),
      decoder_thread_("tizen_omx_decoder_thread"),
      decoder_output_thread_("tizen_omx_decoder_output_thread"),
      init_done_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                 base::WaitableEvent::InitialState::NOT_SIGNALED),
      init_success_(false),
      decoder_state_(kUninitialized),
      decoder_flushing_(false),
      video_profile_(media::VIDEO_CODEC_PROFILE_UNKNOWN),
      decoder_delay_bitstream_buffer_id_(-1),
      decoder_current_input_buffer_(-1),
      decoder_decode_buffer_tasks_scheduled_(0),
      pictures_assigned_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                         base::WaitableEvent::InitialState::NOT_SIGNALED),
      resource_released_event_(nullptr),
      callbacks_({&OmxEventHandler, &OmxEmptyBufferDone, &OmxFillBufferDone}),
      component_((OMX_HANDLETYPE) nullptr),
      component_state_(OMX_StateMax),
      component_req_state_(OMX_StateMax),
      component_flush_requested_(false),
      omx_core_init_done_(false) {}

TizenOmxVideoDecodeAccelerator::~TizenOmxVideoDecodeAccelerator() = default;

bool TizenOmxVideoDecodeAccelerator::IsProfileSupported(
    media::VideoCodecProfile profile) const {
  std::vector<media::VideoCodecProfile> supported = SupportedProfiles();

  for (media::VideoCodecProfile cur_profile : supported)
    if (cur_profile == profile)
      return true;

  return false;
}

bool TizenOmxVideoDecodeAccelerator::EarlyInit() {
  return true;
}

bool TizenOmxVideoDecodeAccelerator::PostInit() {
  return true;
}

bool TizenOmxVideoDecodeAccelerator::Initialize(const Config& config,
                                                Client* client) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  client_ptr_factory_.reset(new base::WeakPtrFactory<Client>(client));
  client_ = client_ptr_factory_->GetWeakPtr();

  if (!IsProfileSupported(config.profile)) {
    LOG(ERROR) << "Initialize(): unsupported profile="
               << media::GetProfileName(config.profile);
    return false;
  }
  video_profile_ = config.profile;

#if defined(TIZEN_PEPPER_EXTENSIONS)
  latency_mode_ = config.latency_mode;
#endif

  if (!EarlyInit()) {
    LOG(ERROR) << "Early init failed.";
    return false;
  }

  if (!decoder_thread_.Start()) {
    LOG(ERROR) << "Failed to start decoder_thread_";
    return false;
  }

  decoder_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::InitTask,
                            base::Unretained(this)));
  init_done_.Wait();
  if (!init_success_) {
    LOG(ERROR) << "Initialization failed";
    // All cleanup will be handled in Destroy()
    return false;
  }

  if (!PostInit()) {
    LOG(ERROR) << "There is error condition after initialization completed.";
    return false;
  }

  LOG(INFO) << "Initialization success profile="
            << media::GetProfileName(config.profile);

  return true;
}

bool TizenOmxVideoDecodeAccelerator::TryToSetupDecodeOnSeparateThread(
    const base::WeakPtr<Client>& decode_client,
    const scoped_refptr<base::SingleThreadTaskRunner>& decode_task_runner) {
  io_client_ = decode_client;
  io_task_runner_ = decode_task_runner;
  return true;
}

void TizenOmxVideoDecodeAccelerator::ContinueInitIfNeeded() {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());
  if (decoder_state_ == kUninitialized || decoder_state_ == kInitializing) {
    if (decoder_state_ == kUninitialized)
      LOG(WARNING) << "Init resumed in kUninitialized. Unexpected omx event?";
    InitTask();
  }
}

void TizenOmxVideoDecodeAccelerator::InitTask() {
  if (decoder_state_ == kError) {
    LOG(ERROR) << "Early out decoder_state_=" << decoder_state_;
    init_done_.Signal();
    return;
  }

  InitResult res = DoInit();

  switch (res) {
    case kInitPending:
      // InitTask will be resumed by async events posted to decoder_thread_:
      // - OnStateSet()
      // Below events will continue init while also setting state to kError:
      // - OnOmxError()
      // During init might also happen:
      // - RMResourceCallback() - it will set kError state and init will be
      //                          resumed by one of events described above
      return;
    case kInitOk:
      init_success_ = true;
      break;
    case kInitError:
    default:
      LOG(ERROR) << "DoInit failed";
      SetDecoderState(kError);
      break;
  }

  init_done_.Signal();
}

TizenOmxVideoDecodeAccelerator::InitResult
TizenOmxVideoDecodeAccelerator::DoInit() {
  if (decoder_state_ == kUninitialized) {
    // For simplicity sake keep this lock until in kUninitialized state.
    base::AutoLock lock(resource_lock_);

    // Request HW decoder resource
    if (allocated_resources_.empty()) {
      if (!RequestDecoderResource())
        return kInitError;
      DCHECK(allocated_resources_.size());
    }

    // Synchronous OMX component init
    if (!InitializeOmxComponent()) {
      LOG(ERROR) << "Failed to initialize OMX component";
      return kInitError;
    }

    // Start decoder output thread
    if (!decoder_output_thread_.Start()) {
      LOG(ERROR) << "Failed to start decoder_output_thread_";
      return kInitError;
    }

    SetDecoderState(kInitializing);
  }

  if (decoder_state_ == kInitializing) {
    // Asynchronous OMX component init
    InitResult res = InitializeOmxComponentAsync();
    if (res != kInitOk)
      return res;

    // Tell component to fill our output buffer
    {
      base::AutoLock lock(output_ctx_lock_);
      DoFillThisBuffer();
      if (decoder_state_ == kError)
        return kInitError;
    }

    SetDecoderState(kInitialized);
    return kInitOk;
  }

  return kInitPending;
}

TizenOmxVideoDecodeAccelerator::InitResult
TizenOmxVideoDecodeAccelerator::InitializeOmxComponentAsync() {
  switch (component_state_) {
    case OMX_StateLoaded:
      // OMX_StateLoaded > OMX_StateIdle
      if (!SwitchComponentState(OMX_StateIdle))
        return kInitError;
      break;
    case OMX_StateIdle:
      // OMX_StateIdle -> OMX_StateExecuting
      // Ports are enabled now
      {
        base::AutoLock out_lock(output_ctx_lock_);
        base::AutoLock in_lock(input_ctx_lock_);
        output_ctx_.output_port_state = kPortEnabled;
        input_ctx_.input_port_state = kPortEnabled;
      }
      if (!SwitchComponentState(OMX_StateExecuting))
        return kInitError;
      break;
    case OMX_StateExecuting:
      // OMX component is ready
      return kInitOk;
    default:
      LOG(ERROR) << "Unknown omx state during init="
                 << OmxStateStr(component_state_);
      return kInitError;
  }

  return kInitPending;
}

bool TizenOmxVideoDecodeAccelerator::RequestDecoderResource() {
  if (!resource_manager_) {
    LOG(ERROR) << "No resource manager available!";
    return false;
  }

  std::vector<TizenRequestedResource> devices;
  if (!FillRequestedResources(&devices)) {
    LOG(ERROR) << "Failed to obtain requested resources!";
    return false;
  }

  if (resource_manager_->AllocateResources(this, devices,
                                           &allocated_resources_)) {
    LOG(ERROR) << "Resources allocated successfuly";
  } else {
    LOG(ERROR) << "Error allocating resources";
    return false;
  }

  // Find video decoder in returned resources
  for (auto& resource : allocated_resources_) {
    if (resource->omx_comp_name.find("video_decoder") != std::string::npos) {
      omx_component_name_ = resource->omx_comp_name;
      LOG(INFO) << "Got decoder OMX component=" << omx_component_name_;
      return true;
    }
  }

  LOG(ERROR) << "Video decoder not found in allocated resources!";
  // Resources will be released in ReleaseDecoderResource()
  return false;
}

void TizenOmxVideoDecodeAccelerator::ReleaseDecoderResource() {
  base::AutoLock lock(resource_lock_);
  if (allocated_resources_.empty() || !resource_manager_)
    return;

  std::vector<int> ids =
      TizenResourceManager::ResourceListToIds(&allocated_resources_);
  if (!resource_manager_->ReleaseResources(this, ids))
    LOG(ERROR) << "Failed to release resources.";
  else
    allocated_resources_.clear();

  // Signal event even in case of error. If resource conflict callback was
  // was issued and we didn't release properly, this means trouble and browser
  // process will be killed.
  if (resource_released_event_) {
    resource_released_event_->Signal();
    resource_released_event_ = nullptr;
  }
}

bool TizenOmxVideoDecodeAccelerator::RMResourceCallback(
    base::WaitableEvent* wait,
    TizenResourceConflictType type,
    const std::vector<int>& ids) {
  base::AutoLock lock(resource_lock_);
  if (allocated_resources_.empty()) {
    // Already released so destroy has started. No need to do anything
    return true;
  } else {
    resource_released_event_ = wait;
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return false;
  }
}

bool TizenOmxVideoDecodeAccelerator::SetOmxDecoderInputType() {
  OMX_ERRORTYPE err;
  OMX_VIDEO_PARAM_DECODERINPUTTYPE param;

  uint32_t video_decoding_type = 0x00;

#if defined(TIZEN_PEPPER_EXTENSIONS)
  switch (latency_mode_) {
    case VideoLatencyMode::NORMAL:
      video_decoding_type = 0x00;  // NORMAL_MODE
      break;
    case VideoLatencyMode::LOW:
      video_decoding_type = 0x01;  // CLIP_MODE
      break;
    default:
      LOG(ERROR) << "Unsupported latency mode = "
                 << static_cast<int32_t>(latency_mode_);
      return false;
      break;
  }
#endif

  param.nVideoDecodingType = video_decoding_type;
  param.bSecureMode = OMX_FALSE;
  param.bMultiDecoding = OMX_FALSE;
  param.bPresetMode = OMX_TRUE;
  param.nCodecExtraDataSize = 0;
  param.bErrorConcealment = OMX_TRUE;
  param.videoCodecTag = 0;
  err = OMX_SetParameter(component_, OMX_IndexVideoDecInputParam, &param);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed on OMX_SetParameter(OMX_IndexVideoDecInputParam) err="
               << OmxErrStr(err);
    return false;
  }
  return true;
}

bool TizenOmxVideoDecodeAccelerator::SwitchComponentState(
    OMX_STATETYPE new_state) {
  if (component_req_state_ != OMX_StateMax) {
    LOG(ERROR)
        << "Component state change requested while other change was ongoing."
        << " new_state=" << OmxStateStr(new_state)
        << " component_req_state_=" << OmxStateStr(component_req_state_);
    return false;
  }
  if (component_state_ == new_state) {
    LOG(WARNING) << "Component state change requested to the same state"
                 << " new_state=" << OmxStateStr(new_state);
    return true;
  }
  OMX_STATETYPE cur_state;
  OMX_GetState(component_, &cur_state);

  if (cur_state != component_state_) {
    LOG(WARNING) << "Component actual state if different than ours"
                 << " cur_state=" << OmxStateStr(cur_state)
                 << " expected=" << OmxStateStr(component_state_);
  }

  OMX_ERRORTYPE err =
      OMX_SendCommand(component_, OMX_CommandStateSet, new_state, NULL);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed on OMX_CommandStateSet(" << OmxStateStr(new_state)
               << ") err=" << OmxErrStr(err);
    return false;
  }

  LOG(INFO) << "Transition requested " << OmxStateStr(component_state_) << ">"
            << OmxStateStr(new_state);
  component_req_state_ = new_state;
  return true;
}

bool TizenOmxVideoDecodeAccelerator::CreateInputBuffers() {
  DCHECK_EQ(decoder_state_, kUninitialized);

  OMX_ERRORTYPE err;
  OMX_PARAM_PORTDEFINITIONTYPE portDef = {0};

  portDef.nVersion = component_ver_;
  portDef.nSize = sizeof(portDef);
  portDef.nPortIndex = kOmxInputPortIndex;

  // We must ignore result from this function due to bug in
  // OMX internal function SDP_MfcDec_GetParameter. Instead we will
  // check if cMIMEType is not null as it should be allocated by OMX.
  // If it's not null it means we got valid port definition.
  OMX_GetParameter(component_, OMX_IndexParamPortDefinition, &portDef);
  if (portDef.format.video.cMIMEType == nullptr) {
    LOG(ERROR) << "Failed on OMX_GetParameter for input port";
    return false;
  }

  const unsigned input_buffer_count = InputFrameBuffersCount();
  const unsigned input_buffer_size = InputFrameBufferSize();
  DCHECK_GT(input_buffer_count, 0);
  DCHECK_GT(input_buffer_size, 0);

  portDef.nBufferCountActual = input_buffer_count;
  portDef.nBufferCountMin = input_buffer_count;
  portDef.nPortIndex = kOmxInputPortIndex;
  portDef.nBufferSize = input_buffer_size;
  portDef.format.video.nFrameWidth = 0;
  portDef.format.video.nFrameHeight = 0;
  portDef.format.video.xFramerate = 0;
  portDef.format.video.eCompressionFormat = VideoCodecToOmxCodingType(
      media::VideoCodecProfileToVideoCodec(video_profile_));

  if (portDef.format.video.eCompressionFormat == OMX_VIDEO_CodingMax) {
    LOG(ERROR) << "Cannot get OMX coding type for video profile:"
               << media::GetProfileName(video_profile_);
    return false;
  }

  err = OMX_SetParameter(component_, OMX_IndexParamPortDefinition, &portDef);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed to OMX_SetParameter for input port err="
               << OmxErrStr(err);
    return false;
  }

  base::AutoLock lock(input_ctx_lock_);
  InputContext& ctx = input_ctx_;

  input_frames_.resize(input_buffer_count);
  for (size_t i = 0; i < input_frames_.size(); ++i) {
    InputFrameBuffer& buf = input_frames_[i];
    OMX_BUFFERHEADERTYPE* buf_hdr = nullptr;

    err = OMX_AllocateBuffer(component_, &buf_hdr, kOmxInputPortIndex,
                             static_cast<void*>(this), input_buffer_size);
    if (err != OMX_ErrorNone || buf_hdr == nullptr) {
      LOG(ERROR) << "Failed to allocate omx input buffer err="
                 << OmxErrStr(err);
      return false;
    }

    buf_hdr->pAppPrivate = reinterpret_cast<OMX_PTR>(static_cast<uintptr_t>(i));
    buf.length = input_buffer_size;
    buf.buf_hdr = buf_hdr;
    buf.bytes_used = 0;
    buf.at_component = false;
    buf.is_free = true;

    ctx.input_frames_free.push(i);
  }

  return true;
}

bool TizenOmxVideoDecodeAccelerator::CreateOutputBuffer() {
  OMX_ERRORTYPE err;
  OMX_PARAM_PORTDEFINITIONTYPE portDef;
  portDef.nVersion = component_ver_;
  portDef.nSize = sizeof(portDef);
  portDef.nPortIndex = kOmxOutputPortIndex;

  err = OMX_GetParameter(component_, OMX_IndexParamPortDefinition, &portDef);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed on OMX_GetParameter for output port err="
               << OmxErrStr(err);
    return false;
  }

  OMX_BUFFERHEADERTYPE* buf = nullptr;
  err = OMX_AllocateBuffer(component_, &buf, kOmxOutputPortIndex,
                           static_cast<void*>(this), portDef.nBufferSize);
  if (err != OMX_ErrorNone || buf == nullptr) {
    LOG(ERROR) << "Failed to allocate initial output buffer err="
               << OmxErrStr(err);
    return false;
  }

  base::AutoLock lock(output_ctx_lock_);
  OmxOutputBuffer& buffer = output_ctx_.output_buffer;

  if (buffer.init_done) {
    LOG(ERROR) << "Omx buf header is already initialized!";
    return false;
  }

  buf->pAppPrivate = reinterpret_cast<OMX_PTR>(kOutputBufferPrivate);
  buffer.buf_hdr = buf;
  buffer.init_done = true;

  return true;
}

bool TizenOmxVideoDecodeAccelerator::DestroyOutputBuffer() {
  output_ctx_lock_.AssertAcquired();

  OmxOutputBuffer& omx_buf = output_ctx_.output_buffer;
  if (omx_buf.buf_hdr == nullptr)
    return true;

  if (omx_buf.at_component)
    LOG(WARNING) << "Destroying output buffer while in component";

  OMX_ERRORTYPE err =
      OMX_FreeBuffer(component_, kOmxOutputPortIndex, omx_buf.buf_hdr);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed on OMX_FreeBuffer for output buffer err="
               << OmxErrStr(err);
    return false;
  }

  omx_buf.buf_hdr = nullptr;
  omx_buf.at_component = false;
  omx_buf.init_done = false;

  LOG(INFO) << "Destroy of output buffers completed successfully.";
  return true;
}

bool TizenOmxVideoDecodeAccelerator::DestroyInputBuffers() {
  input_ctx_lock_.AssertAcquired();
  bool no_error = true;

  if (input_frames_.size() == 0)
    return true;

  for (size_t i = 0; i < input_frames_.size(); ++i) {
    InputFrameBuffer& buf = input_frames_[i];

    if (buf.buf_hdr == nullptr)
      continue;

    if (buf.at_component)
      LOG(WARNING) << "Destroying input buffer that is at component i=" << i;

    OMX_ERRORTYPE err =
        OMX_FreeBuffer(component_, kOmxInputPortIndex, buf.buf_hdr);
    if (err != OMX_ErrorNone) {
      LOG(ERROR) << "Failed on OMX_FreeBuffer for input port i=" << i
                 << " err=" << OmxErrStr(err);
      no_error = false;
      continue;
    }
    buf.buf_hdr = nullptr;
    buf.length = 0;
    buf.is_free = false;
  }

  if (no_error) {
    input_frames_.resize(0);
    LOG(INFO) << "Destroy of input buffers completed successfully.";
  }
  return no_error;
}

bool TizenOmxVideoDecodeAccelerator::DestroyOmxComponent() {
  if (component_ == nullptr)
    return true;
  OMX_ERRORTYPE err = OMX_FreeHandle(component_);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed on OMX_FreeHandle err=" << OmxErrStr(err);
    return false;
  }
  component_state_ = OMX_StateMax;
  component_req_state_ = OMX_StateMax;
  component_ = nullptr;
  return true;
}

bool TizenOmxVideoDecodeAccelerator::InitializeOmxComponentBasic() {
  OMX_HANDLETYPE component;
  OMX_VERSIONTYPE ver, verSpec;
  OMX_UUIDTYPE uuid;
  char compName[128];
  char dstName[128] = {0};

  if (omx_component_name_.size() == 0) {
    LOG(ERROR) << "Have no omx component name set. Was it requested via "
                  "TvResourceManager?";
    return false;
  } else {
    strncpy(compName, omx_component_name_.c_str(), 128);
  }

  OMX_ERRORTYPE err = OMX_GetHandle(&component, compName,
                                    static_cast<void*>(this), &callbacks_);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed on OMX_GetHandle err=" << OmxErrStr(err);
    return false;
  } else {
    component_ = component;
  }

  err = OMX_GetComponentVersion(component, (OMX_STRING)dstName, &ver, &verSpec,
                                &uuid);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed on OMX_GetComponentVersion err=" << OmxErrStr(err);
    return false;
  }

  LOG(INFO) << "Success to initialize OMX component: "
            << " name= " << dstName
            << " ver=" << static_cast<int>(ver.s.nVersionMajor) << "."
            << static_cast<int>(ver.s.nVersionMinor);

  component_ver_ = ver;

  OMX_GetState(component_, &component_state_);
  // Sanity check
  if (component_state_ != OMX_StateLoaded) {
    LOG(ERROR) << "Abnormal component state after init:"
               << OmxStateStr(component_state_);
    return false;
  }

  return true;
}

bool TizenOmxVideoDecodeAccelerator::InitializeOmxComponent() {
  OMX_Init();
  omx_core_init_done_ = true;

  if (!InitializeOmxComponentBasic())
    return false;
  DCHECK(component_ != nullptr);

  // Set input parameters
  if (!SetOmxDecoderInputType())
    return false;

  // Create input buffer
  if (!CreateInputBuffers())
    return false;

  if (!CreateOutputBuffer())
    return false;

  LOG(INFO) << "OMX component initialize done";
  return true;
}

void TizenOmxVideoDecodeAccelerator::DoFillThisBuffer() {
  output_ctx_lock_.AssertAcquired();
  OutputContext& ctx = output_ctx_;

  // Safety checks
  if (ctx.is_error.load()) {
    LOG(ERROR) << "Early out: output ctx is error";
    return;
  }
  if (!ctx.output_buffer.init_done) {
    LOG(ERROR) << "Omx output buffer not inited. Erroring";
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return;
  }
  if (ctx.output_buffer.buf_hdr == nullptr) {
    LOG(ERROR) << "buf_hdr is null!";
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return;
  }

  if (ctx.output_buffer.at_component) {
    LOG(ERROR) << "buffer already at component";
    return;
  }

  OMX_ERRORTYPE err = OMX_FillThisBuffer(component_, ctx.output_buffer.buf_hdr);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed to OMX_FillThisBuffer err=" << OmxErrStr(err);
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return;
  }

  ctx.output_buffer.at_component = true;
}

void TizenOmxVideoDecodeAccelerator::DoEmptyBufferDone(
    OMX_BUFFERHEADERTYPE* pBuffer) {
  base::AutoLock lock(input_ctx_lock_);
  InputContext& ctx = input_ctx_;
  // Safety checks
  if (ctx.is_error.load()) {
    LOG(ERROR) << "Early out: input ctx is error";
    return;
  }
  uintptr_t idx = reinterpret_cast<uintptr_t>(pBuffer->pAppPrivate);
  InputFrameBuffer& buf = input_frames_[idx];

  if (!buf.at_component) {
    LOG(ERROR) << "Emptied buffer that wasn't at component idx=" << idx;
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return;
  }

  if (pBuffer->nFlags & OMX_BUFFERFLAG_EOS)
    pBuffer->nFlags &= ~OMX_BUFFERFLAG_EOS;

  int free_buffers = ctx.input_frames_free.size();
  buf.at_component = false;
  ctx.input_frames_free.push(idx);
  buf.is_free = true;
  buf.bytes_used = 0;

  if (free_buffers == 0) {
    // We have freed new buffer so schedule decode task if needed
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(base::IgnoreResult(&TizenOmxVideoDecodeAccelerator::
                                          ScheduleDecodeBufferTaskIfNeeded),
                   base::Unretained(this)));
  }

  if (ctx.input_frames_free.size() == input_frames_.size()) {
    // All input buffers are returned, finish flush if needed
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE, base::Bind(base::IgnoreResult(
                                  &TizenOmxVideoDecodeAccelerator::FinishFlush),
                              base::Unretained(this)));
  }
}

bool TizenOmxVideoDecodeAccelerator::DoEmptyThisBuffer(int idx) {
  input_ctx_lock_.AssertAcquired();
  InputContext& ctx = input_ctx_;
  if (ctx.is_error.load()) {
    LOG(ERROR) << "Early out: input ctx is error";
    return false;
  }
  if (idx == -1) {
    LOG(ERROR) << "Fatal: idx=-1";
    return false;
  }
  InputFrameBuffer& buf = input_frames_[idx];
  if (buf.at_component) {
    LOG(ERROR) << "Called with buffer already at component";
    return false;
  }
  buf.buf_hdr->nOffset = 0;
  buf.buf_hdr->nFilledLen = buf.bytes_used;
  if (buf.input_id == kFlushBufferId) {
    DCHECK_EQ(static_cast<int>(buf.bytes_used), 0);
    buf.buf_hdr->nFlags |= OMX_BUFFERFLAG_EOS;
    // kSamsungPtsWorkAroundMagic is added to not trigger
    // EOS detection mechanism in OMX's SDP_MfcDec_OutputBufferProcess()
    buf.buf_hdr->nTimeStamp = ctx.last_pts + kSamsungPtsWorkAroundMagic;
  } else {
    ctx.last_pts = buf.buf_hdr->nTimeStamp = buf.input_id;
  }

  OMX_ERRORTYPE err = OMX_EmptyThisBuffer(component_, buf.buf_hdr);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed to OMX_EmptyThisBuffer idx=" << idx;
    return false;
  }

  buf.at_component = true;
  buf.is_free = false;

  return true;
}

bool TizenOmxVideoDecodeAccelerator::AppendToInputFrameBuffer(const void* data,
                                                              size_t size) {
  if (size == 0)
    return true;
  if (decoder_current_input_buffer_ == -1) {
    LOG(ERROR) << "Fatal: decoder_current_input_buffer_=-1";
    return false;
  }
  InputFrameBuffer& buf = input_frames_[decoder_current_input_buffer_];
  if (size > buf.length - buf.bytes_used) {
    LOG(ERROR) << "over-size frame, erroring";
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return false;
  }
  memcpy(reinterpret_cast<uint8_t*>(buf.buf_hdr->pBuffer) + buf.bytes_used,
         data, size);
  buf.bytes_used += size;
  return true;
}

bool TizenOmxVideoDecodeAccelerator::AppendAndFlushInputFrame(const void* data,
                                                              size_t size) {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());
  DCHECK_NE(decoder_state_, kUninitialized);
  DCHECK_NE(decoder_state_, kResetting);
  DCHECK_NE(decoder_state_, kError);
  // This routine can handle data == NULL and size == 0, which occurs when
  // we queue an empty buffer for the purposes of flushing the pipe.

  if (decoder_current_input_buffer_ != -1) {
    LOG(ERROR) << "Previous buffer wasn't sent to component properly";
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return false;
  }

  // Try to get an available input buffer
  base::AutoLock lock(input_ctx_lock_);
  InputContext& ctx = input_ctx_;
  if (ctx.input_frames_free.empty())
    // Stalled for input buffers
    return false;

  decoder_current_input_buffer_ = ctx.input_frames_free.front();
  ctx.input_frames_free.pop();
  InputFrameBuffer& buf = input_frames_[decoder_current_input_buffer_];
  DCHECK_EQ((int)buf.bytes_used, 0);
  DCHECK(decoder_current_bitstream_buffer_ != NULL);
  buf.input_id = decoder_current_bitstream_buffer_->input_id;

  if ((!size || AppendToInputFrameBuffer(data, size)) &&
      DoEmptyThisBuffer(decoder_current_input_buffer_)) {
    decoder_current_input_buffer_ = -1;
    return true;
  }

  return false;
}

void TizenOmxVideoDecodeAccelerator::DecodeBufferTask() {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());
  DCHECK_GE(decoder_decode_buffer_tasks_scheduled_, 1);
  decoder_decode_buffer_tasks_scheduled_--;

  if (decoder_state_ == kResetting) {
    LOG(ERROR) << "early out: kResetting state";
    return;
  } else if (decoder_state_ == kError) {
    LOG(ERROR) << "early out: kError state";
    return;
  } else if (decoder_state_ == kDestroying) {
    LOG(INFO) << "early out: kDestroying state";
    return;
  }

  if (decoder_current_bitstream_buffer_ == NULL) {
    if (decoder_input_queue_.empty())
      return;

    linked_ptr<BitstreamBufferRef>& buffer_ref = decoder_input_queue_.front();

    if (decoder_delay_bitstream_buffer_id_ == buffer_ref->input_id) {
      // We're asked to delay decoding on this and subsequent buffers.
      return;
    }

    // Setup to use the next buffer.
    decoder_current_bitstream_buffer_.reset(buffer_ref.release());
    decoder_input_queue_.pop();
  }
  bool schedule_task = true;
  const size_t size = decoder_current_bitstream_buffer_->size;
  if (size) {
    const uint8_t* const data = reinterpret_cast<const uint8_t*>(
        decoder_current_bitstream_buffer_->shm->memory());
    schedule_task = AppendAndFlushInputFrame(data, size);
  } else if (decoder_current_bitstream_buffer_->input_id != kFlushBufferId) {
    // Got an empty buffer from client and we won't send it to HW
    schedule_task = true;
  } else {
    // We've got flush buffer
    DCHECK_EQ(decoder_current_bitstream_buffer_->shm.get(),
              static_cast<base::SharedMemory*>(NULL));
    schedule_task = AppendAndFlushInputFrame(NULL, size);
  }

  if (decoder_state_ == kError) {
    LOG(ERROR) << "Error during AppendAndFlushInputFrame. Bailing.";
    return;
  }

  if (schedule_task) {
    // first input buffer was sent to HW, change our state to decoding
    if (decoder_state_ == kInitialized || decoder_state_ == kAfterReset)
      SetDecoderState(kDecoding);

    // Our current bitstream buffer is done; return it.
    // BitstreamBufferRef destructor calls NotifyEndOfBitstreamBuffer().
    decoder_current_bitstream_buffer_.reset();

    ScheduleDecodeBufferTaskIfNeeded();
  }
  // If we are not scheduling another DecodeBufferTask it means that
  // AppendToInputFrame couldn't find any free buffers on
  // input_ctx.input_frames_free_
  // and decoder_current_bitstream_buffer_ was not consumed.
  // When EmptyBufferDone will free a buffer it will also call
  // ScheduleDecodeBufferTaskIfNeeded()
  // to continue consuming decoder_current_bitstream_buffer_;
}

void TizenOmxVideoDecodeAccelerator::ScheduleDecodeBufferTaskIfNeeded() {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());

  // If we're behind on tasks, schedule another one.
  int buffers_to_decode = decoder_input_queue_.size();
  if (decoder_current_bitstream_buffer_ != NULL)
    buffers_to_decode++;
  if (decoder_decode_buffer_tasks_scheduled_ < buffers_to_decode) {
    decoder_decode_buffer_tasks_scheduled_++;
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::DecodeBufferTask,
                              base::Unretained(this)));
  }
}

void TizenOmxVideoDecodeAccelerator::DecodeTask(
    const media::BitstreamBuffer& bitstream_buffer) {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());

  std::unique_ptr<BitstreamBufferRef> bitstream_record(new BitstreamBufferRef(
      io_client_, io_task_runner_,
      new base::SharedMemory(bitstream_buffer.handle(), true),
      bitstream_buffer.size(), bitstream_buffer.id()));
  if (!bitstream_record->shm->Map(bitstream_buffer.size())) {
    LOG(ERROR) << "Decode(): could not map bitstream_buffer";
    SetErrorState(FROM_HERE, UNREADABLE_INPUT);
    return;
  }

  if (decoder_state_ == kResetting || decoder_flushing_) {
    // In the case that we're resetting or flushing, we need to delay decoding
    // the BitstreamBuffers that come after the Reset() or Flush() call.  When
    // we're here, we know that this DecodeTask() was scheduled by a Decode()
    // call that came after (in the client thread) the Reset() or Flush() call;
    // thus set up the delay if necessary.
    // TODO(i.gorniak): check if it's possible to
    // decoder_delay_bitstream_buffer_id_ == kFlushBufferId
    if (decoder_delay_bitstream_buffer_id_ == -1)
      decoder_delay_bitstream_buffer_id_ = bitstream_record->input_id;
  } else if (decoder_state_ == kError) {
    LOG(ERROR) << "Early out: kError state";
    return;
  } else if (decoder_state_ == kDestroying) {
    LOG(ERROR) << "Early out: kDestroying state";
    return;
  } else if (decoder_state_ == kInitializing) {
    LOG(ERROR) << "Early out: kInitializing state";
    return;
  }

  decoder_input_queue_.push(
      linked_ptr<BitstreamBufferRef>(bitstream_record.release()));
  decoder_decode_buffer_tasks_scheduled_++;
  DecodeBufferTask();
}

void TizenOmxVideoDecodeAccelerator::Decode(
    const media::BitstreamBuffer& bitstream_buffer) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  decoder_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::DecodeTask,
                            base::Unretained(this), bitstream_buffer));
}

void TizenOmxVideoDecodeAccelerator::DoFillBufferDone(
    OMX_BUFFERHEADERTYPE* pBuffer) {
  uintptr_t buf_hdr_id = reinterpret_cast<uintptr_t>(pBuffer->pAppPrivate);
  if (output_ctx_.is_error.load()) {
    LOG(ERROR) << "Early out: output ctx is error";
    return;
  } else if (buf_hdr_id != kOutputBufferPrivate) {
    LOG(ERROR) << "Got unknown omx output buffer. Erroring";
    SetErrorState(FROM_HERE, INVALID_ARGUMENT);
    return;
  }

  OutputContext& ctx = output_ctx_;
  base::AutoLock lock(output_ctx_lock_);
  ctx.output_buffer.at_component = false;

  DecodedVideoFrame prev_frame(ctx.last_frame);
  PictureBufferDesc prev_buf_desc(&prev_frame);

  ctx.last_frame = DecodedVideoFrame(pBuffer);
  DecodedVideoFrame& cur_frame = ctx.last_frame;
  PictureBufferDesc cur_buf_desc(&cur_frame);

  if (!ctx.no_decoded_frames++) {
    if (!cur_frame.contains_picture_info) {
      if (ctx.decoder_state != kResetting && ctx.decoder_state != kDestroying)
        SetErrorState(FROM_HERE, INVALID_ARGUMENT);
      return;
    }
    // For first frame we unconditionally schedule grow of new output records
    ScheduleGrowRecords(cur_buf_desc);
    return;
  }

  if (cur_frame.contains_picture_info) {
    if (prev_buf_desc.IsValid() && cur_buf_desc.IsValid() &&
        prev_buf_desc != cur_buf_desc) {
      LOG(INFO) << "Picture buffer layout has changed from "
                << prev_buf_desc.ToString() << " to "
                << cur_buf_desc.ToString();
      ScheduleGrowRecords(cur_buf_desc);

      // Schedule deletion of records with previous layout
      MarkRecordsForDeletion(&prev_buf_desc);

      // Processing of frame will be restarted in GrowOutputRecordsTask()
      return;
    }
  }

  // Just another decoded frame
  ProcessLastFrame();
}

void TizenOmxVideoDecodeAccelerator::ProcessLastFrame() {
  output_ctx_lock_.AssertAcquired();
  OutputContext& ctx = output_ctx_;
  DecodedVideoFrame& frame = ctx.last_frame;

  if (!frame.show_frame || !frame.contains_picture_info ||
      ctx.decoder_state == kDestroying) {
    // In this case we don't want to client get our frame so
    // fake that it's sent already
    frame.is_sent = true;
  } else {
    // Send frame to client
    if (!DoPictureReady())
      return;
    if (!frame.is_sent) {
      // There was no free output records
      return;
    }
  }

  // Frame is now sent to client
  if (ctx.decoder_state == kResetting || ctx.decoder_state == kDestroying) {
    // Do nothing
  } else if (frame.is_eos) {
    if (!ctx.decoder_is_flushing) {
      LOG(ERROR) << "Got EOS frame without flushing started.";
      SetErrorState(FROM_HERE, PLATFORM_FAILURE);
      return;
    }
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE, base::Bind(base::IgnoreResult(
                                  &TizenOmxVideoDecodeAccelerator::FinishFlush),
                              base::Unretained(this)));
  } else {
    // Resume video output sink
    DoFillThisBuffer();
  }
}

void TizenOmxVideoDecodeAccelerator::ReusePictureBufferTask(
    int32_t picture_buffer_id) {
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

  OutputRecord* output_record = output_records_[index].get();
  if (!output_record->at_client) {
    LOG(ERROR) << "ReusePictureBufferTask(): picture_buffer_id not reusable"
               << " index=" << index
               << " at_client=" << output_record->at_client
               << " is_mapped=" << output_record->is_mapped;
    SetErrorState(FROM_HERE, INVALID_ARGUMENT);
    return;
  }

  output_record->at_client = false;
  ctx.output_records_free.push_back(index);

  if (ctx.decoder_state == kResetting) {
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(
            base::IgnoreResult(&TizenOmxVideoDecodeAccelerator::ResetDoneTask),
            base::Unretained(this)));
  } else if (output_record->should_destroy) {
    LOG(ERROR) << "Scheduling destroy of output record " << index;
    ctx.output_records_free.pop_back();
    child_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&TizenOmxVideoDecodeAccelerator::PruneOutputRecordsTask,
                   base::Unretained(this)));
  } else if (!ctx.last_frame.is_sent &&
             output_record->buffer_desc == PictureBufferDesc(&ctx.last_frame)) {
    // there was decoded frame posted via DoFillBufferDone
    // but not sent to client due to lack of free output buffers.
    // Process it now.
    ProcessLastFrame();
  }
}

void TizenOmxVideoDecodeAccelerator::ReusePictureBuffer(
    int32_t picture_buffer_id) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  decoder_output_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&TizenOmxVideoDecodeAccelerator::ReusePictureBufferTask,
                 base::Unretained(this), picture_buffer_id));
}

bool TizenOmxVideoDecodeAccelerator::ProcessOutputPicture(
    media::Picture* picture,
    DecodedVideoFrame* last_frame,
    OutputRecord* free_record) {
  return true;
}

bool TizenOmxVideoDecodeAccelerator::DoPictureReady() {
  output_ctx_lock_.AssertAcquired();
  OutputContext& ctx = output_ctx_;
  DecodedVideoFrame& frame = ctx.last_frame;

  if (!frame.IsValid()) {
    LOG(ERROR) << "DoPictureReady called with invalid frame.";
    SetErrorState(FROM_HERE, INVALID_ARGUMENT);
    return false;
  }

  if (frame.is_sent) {
    LOG(WARNING) << "early out: frame already sent";
    return true;
  }

  if (ctx.output_records_free.empty())
    // No free output records
    return true;

  int rec_idx = ctx.output_records_free.front();
  ctx.output_records_free.pop_front();
  OutputRecord* rec = output_records_[rec_idx].get();

  if (rec->at_client || !rec->is_mapped) {
    LOG(ERROR) << "Invalid record on free list idx=" << rec_idx
               << " at_client=" << rec->at_client
               << " is_mapped=" << rec->is_mapped;
    SetErrorState(FROM_HERE, INVALID_ARGUMENT);
    return false;
  }

  media::Picture picture(rec->picture_id, frame.timestamp, gfx::Rect(frame.res),
                         ctx.color_space, false);
  if (!ProcessOutputPicture(&picture, &ctx.last_frame, rec)) {
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return false;
  }

  child_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Client::PictureReady, client_, picture));

  frame.is_sent = true;
  rec->at_client = true;

  return true;
}

void TizenOmxVideoDecodeAccelerator::PruneOutputRecordsTask() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  base::AutoLock lock(output_ctx_lock_);

  std::vector<std::unique_ptr<OutputRecord>> new_records;
  std::list<int> new_records_free;

  size_t size = output_records_.size();
  for (size_t i = 0; i < size; ++i) {
    std::unique_ptr<OutputRecord>& rec = output_records_[i];
    if (rec->should_destroy && !rec->at_client) {
      DestroyOutputRecord(i);
    } else {
      new_records.push_back(std::move(rec));
      if (!new_records.back()->at_client)
        new_records_free.push_back(new_records.size() - 1);
    }
  }

  output_records_ = std::move(new_records);
  output_ctx_.output_records_free = std::move(new_records_free);
}

void TizenOmxVideoDecodeAccelerator::MarkRecordsForDeletion(
    PictureBufferDesc* with_desc) {
  output_ctx_lock_.AssertAcquired();
  OutputContext& ctx = output_ctx_;
  size_t count = output_records_.size();
  bool schedule_prune = false;
  LOG(ERROR) << "Marking for deletion records with " << with_desc->ToString();

  for (unsigned i = 0; i < count; ++i) {
    OutputRecord* rec = output_records_[i].get();
    if (!rec->is_mapped || rec->buffer_desc != *with_desc)
      continue;
    rec->should_destroy = true;

    if (!rec->at_client) {
      ctx.output_records_free.remove(i);
      schedule_prune = true;
    }
    // If record is at client, it will be deleted in ReusePictureBufferTask
    // after it's returned
    LOG(INFO) << "Scheduling deletion of output record idx=" << i;
  }

  if (schedule_prune)
    child_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&TizenOmxVideoDecodeAccelerator::PruneOutputRecordsTask,
                   base::Unretained(this)));
}

bool TizenOmxVideoDecodeAccelerator::ScheduleGrowRecords(
    const PictureBufferDesc& desc) {
  output_ctx_lock_.AssertAcquired();
  OutputContext& ctx = output_ctx_;
  if (ctx.output_buffers_requested_count) {
    LOG(ERROR) << "Grow of output records already requested.";
    return false;
  }
  ctx.output_buffers_requested_count = OutputRecordsCount();
  ctx.output_buffers_requested_desc = desc;

  decoder_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(base::IgnoreResult(
                     &TizenOmxVideoDecodeAccelerator::GrowOutputRecordsTask),
                 base::Unretained(this)));

  return true;
}

void TizenOmxVideoDecodeAccelerator::GrowOutputRecordsTask() {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());

  if (decoder_state_ == kError) {
    LOG(ERROR) << "Early out: kError state";
    return;
  } else if (decoder_state_ == kDestroying) {
    LOG(ERROR) << "Early out: kDestroying state";
    return;
  }

  base::AutoLock lock(output_ctx_lock_);
  DCHECK_NE(output_ctx_.output_buffers_requested_count, 0);
  DCHECK(output_ctx_.output_buffers_requested_desc.IsValid());

  if (!GrowOutputRecords()) {
    LOG(ERROR) << "Failed on GrowOutputRecords. Erroring";
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return;
  }

  output_ctx_.output_buffers_requested_desc = PictureBufferDesc();

  // We have been called due to lack of free record for new frame.
  // So sent pictures to client as we have output records now
  if (!output_ctx_.last_frame.is_sent)
    ProcessLastFrame();

  // ResetTask() could've been delayed because of this task running
  if (decoder_state_ == kResetting) {
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::ResetDoneTask,
                              base::Unretained(this)));
    return;
  }
}

bool TizenOmxVideoDecodeAccelerator::GrowOutputRecords() {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());
  output_ctx_lock_.AssertAcquired();
  OutputContext& ctx = output_ctx_;
  int grow_by = ctx.output_buffers_requested_count;

  child_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&Client::ProvidePictureBuffers, client_, grow_by,
                 VideoPixelFormat(), TexturesPerPictureBuffer(),
                 ctx.output_buffers_requested_desc.size, TextureTarget()));

  output_ctx_lock_.Release();
  pictures_assigned_.Wait();

  if (decoder_state_ == kError) {
    LOG(ERROR) << "Early out: kError state";
    return false;
  }

  output_ctx_lock_.Acquire();

  size_t size = output_records_.size();
  for (size_t i = size - grow_by; i < size; ++i) {
    OutputRecord* rec = output_records_[i].get();
    if (!rec->is_mapped) {
      LOG(ERROR) << "Newly created record is not mapped idx=" << i;
      SetErrorState(FROM_HERE, PLATFORM_FAILURE);
      return false;
    }
    rec->at_client = false;
    ctx.output_records_free.push_back(i);
  }

  return true;
}

void TizenOmxVideoDecodeAccelerator::AssignPictureBuffers(
    const std::vector<media::PictureBuffer>& buffers) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  if (output_ctx_.is_error.load()) {
    LOG(ERROR) << "Early out: output ctx is error";
    pictures_assigned_.Signal();
    return;
  }

  // we can touch output_records_ from child thread as decoder_thread_
  // is now waiting on pictures_assigned_
  unsigned old_record_count = output_records_.size();
  unsigned new_count = buffers.size();

  base::AutoLock lock(output_ctx_lock_);
  OutputContext& ctx = output_ctx_;
  PictureBufferDesc& buffer_desc = ctx.output_buffers_requested_desc;

  if (buffers.size() < (unsigned)ctx.output_buffers_requested_count)
    LOG(WARNING) << "Got less output picture buffers than requested";

  output_records_.resize(old_record_count + new_count);
  int cur_new = 0;
  for (size_t i = old_record_count; i < output_records_.size();
       ++i, ++cur_new) {
    OutputRecord* rec = AllocateOutputRecord();
    if (!rec) {
      LOG(ERROR) << "Failed to allocated output record";
      SetErrorState(FROM_HERE, PLATFORM_FAILURE);
      break;
    }

    rec->picture_id = buffers[cur_new].id();
    rec->texture_id = !buffers[cur_new].service_texture_ids().empty()
                          ? buffers[cur_new].service_texture_ids()[0]
                          : 0;
    rec->buffer_desc = buffer_desc;

    if (!InitOutputRecord(buffers[cur_new], rec)) {
      LOG(ERROR) << "Failed to init input record idx=" << i
                 << " with buffer"
                    " description: "
                 << buffer_desc.ToString();
      SetErrorState(FROM_HERE, PLATFORM_FAILURE);
      break;
    }

    LOG(INFO) << "Creating output record idx=" << i << " with "
              << buffer_desc.ToString();

    rec->is_mapped = true;
    output_records_[i].reset(rec);
    ctx.output_buffers_requested_count--;
  }

  pictures_assigned_.Signal();
}

bool TizenOmxVideoDecodeAccelerator::DestroyOutputRecord(int idx) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  output_ctx_lock_.AssertAcquired();
  OutputRecord* rec = output_records_[idx].get();

  // Do nothing if not mapped
  if (!rec->is_mapped)
    return true;

  rec->is_mapped = false;
  child_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&Client::DismissPictureBuffer, client_, rec->picture_id));

  LOG(INFO) << "Output record " << idx << " released";

  return true;
}

void TizenOmxVideoDecodeAccelerator::DestroyOutputRecords() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  bool success = true;

  output_ctx_lock_.AssertAcquired();

  size_t size = output_records_.size();
  for (size_t i = 0; i < size; ++i) {
    if (!DestroyOutputRecord(i))
      success = false;
  }

  output_ctx_.output_records_free.clear();
  output_records_.clear();

  if (success)
    LOG(INFO) << "Destroy of output records completed successfully";
  else
    LOG(ERROR) << "Destroy of output records completed with errors";
}

void TizenOmxVideoDecodeAccelerator::FinishFlush() {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());
  if (!decoder_flushing_ || component_flush_requested_ > 0)
    return;

  if (!decoder_input_queue_.empty()) {
    if (decoder_input_queue_.front()->input_id !=
        decoder_delay_bitstream_buffer_id_)
      return;
  }
  if (decoder_current_input_buffer_ != -1)
    return;

  {
    base::AutoLock in_lock(input_ctx_lock_);
    InputContext& in_ctx = input_ctx_;
    if (in_ctx.input_frames_free.size() != input_frames_.size())
      return;
  }

  {
    base::AutoLock out_lock(output_ctx_lock_);
    OutputContext& out_ctx = output_ctx_;
    if (out_ctx.output_buffer.at_component || !out_ctx.last_frame.is_sent)
      return;

    // during flush, after EOS frame was received, omx output buffer was not
    // sent to component in ProcessLastFrame() so do it now
    DoFillThisBuffer();
  }

  decoder_delay_bitstream_buffer_id_ = -1;
  decoder_flushing_ = output_ctx_.decoder_is_flushing = false;

  LOG(INFO) << "Flush has been finished.";
  child_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&Client::NotifyFlushDone, client_));
  // While we were flushing, we early-outed DecodeBufferTask()s.
  ScheduleDecodeBufferTaskIfNeeded();
}

void TizenOmxVideoDecodeAccelerator::FlushTask() {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());

  // Flush outstanding buffers.
  if (decoder_state_ == kInitialized || decoder_state_ == kAfterReset) {
    // There's nothing in the pipe, so return done immediately.
    child_task_runner_->PostTask(FROM_HERE,
                                 base::Bind(&Client::NotifyFlushDone, client_));
    return;
  } else if (decoder_state_ == kError) {
    LOG(ERROR) << "FlushTask(): early out: kError state";
    return;
  } else if (decoder_state_ == kDestroying) {
    LOG(ERROR) << "FlushTask(): early out: kDestroying state";
    return;
  } else if (decoder_state_ == kInitializing) {
    LOG(ERROR) << "FlushTask(): early out: kInitializing state";
    return;
  }

  // We don't support stacked flushing.
  if (decoder_flushing_)
    LOG(ERROR) << "Flush called without finishing previous one!";
  LOG(INFO) << "Flush has been started";

  // Queue up an empty buffer -- this triggers the flush.
  decoder_input_queue_.push(
      linked_ptr<BitstreamBufferRef>(new BitstreamBufferRef(
          io_client_, io_task_runner_, NULL, 0, kFlushBufferId)));
  decoder_flushing_ = true;

  {
    base::AutoLock lock(output_ctx_lock_);
    output_ctx_.decoder_is_flushing = true;
  }

  ScheduleDecodeBufferTaskIfNeeded();
}

void TizenOmxVideoDecodeAccelerator::Flush() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  decoder_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::FlushTask,
                            base::Unretained(this)));
}

void TizenOmxVideoDecodeAccelerator::OnFlushComplete(OMX_U32 port_index) {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());

  if (decoder_state_ == kError) {
    LOG(ERROR) << "Early out: kError state";
    return;
  } else if (decoder_state_ == kDestroying) {
    LOG(ERROR) << "Early out: kDestroying state";
    return;
  }

  if (decoder_state_ == kResetting) {
    // Flush for reset purposes
    component_flush_requested_--;
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::ResetDoneTask,
                              base::Unretained(this)));
    return;
  } else {
    LOG(ERROR) << "Port flush completed in not expected decoder state="
               << decoder_state_;
    SetErrorState(FROM_HERE, INVALID_ARGUMENT);
  }
}

void TizenOmxVideoDecodeAccelerator::ResetTask() {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());

  if (decoder_state_ == kError) {
    LOG(ERROR) << "ResetTask(): early out: kError state";
    return;
  } else if (decoder_state_ == kDestroying) {
    LOG(ERROR) << "Early out: kDestroying state";
    return;
  } else if (decoder_state_ == kInitializing) {
    LOG(ERROR) << "Early out: kInitializing state";
    return;
  }

  if (decoder_flushing_) {
    // Naughty reset call without previous flush completed.
    // To not overcomplicate things let it finish
    // and continue with reset afterwards.
    decoder_thread_.message_loop()->task_runner()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&TizenOmxVideoDecodeAccelerator::ResetTask,
                   base::Unretained(this)),
        base::TimeDelta::FromMilliseconds(1));
    return;
  }
  LOG(INFO) << "Reset has been started";

  decoder_current_bitstream_buffer_.reset();
  while (!decoder_input_queue_.empty())
    decoder_input_queue_.pop();

  decoder_current_input_buffer_ = -1;

  // Mark that we're resetting, then enqueue a ResetDoneTask().  All intervening
  // jobs will early-out in the kResetting state
  SetDecoderState(kResetting);

  // perform HW flush, ResetDoneTask won't continue till it ends
  OMX_ERRORTYPE err =
      OMX_SendCommand(component_, OMX_CommandFlush, OMX_ALL, NULL);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed on OMX_SendCommand(OMX_CommandFlush) err="
               << OmxErrStr(err);
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
    return;
  }
  component_flush_requested_ = 2;
}

void TizenOmxVideoDecodeAccelerator::ResetDoneTask() {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());

  if (decoder_state_ == kError) {
    LOG(ERROR) << "Early out: kError state";
    return;
  } else if (decoder_state_ == kDestroying) {
    LOG(ERROR) << "Early out: kDestroying state";
    return;
  }

  // Wait for HW flush to complete
  if (component_flush_requested_ > 0)
    return;

  {
    base::AutoLock lock(output_ctx_lock_);
    OutputContext& ctx = output_ctx_;

    // Wait until getting new picture buffers ends
    if (ctx.output_buffers_requested_count)
      return;

    // All conditions for reset to end are met
    // Reset output context
    ctx.color_space = gfx::ColorSpace();

    // Resume getting pictures from OMX
    DoFillThisBuffer();
  }

  // Jobs drained, we're finished resetting.
  DCHECK_EQ(decoder_state_, kResetting);

  // Reset input context
  {
    base::AutoLock lock(input_ctx_lock_);
    input_ctx_.last_pts = 0;
    if (input_ctx_.input_frames_free.size() != input_frames_.size())
      LOG(WARNING) << "Not all input buffers are returned";
  }

  // Reset decoder stuff
  decoder_delay_bitstream_buffer_id_ = -1;
  SetDecoderState(kAfterReset);

  LOG(INFO) << "Reset has been finished.";

  child_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&Client::NotifyResetDone, client_));

  // While we were resetting, we early-outed DecodeBufferTask()s.
  ScheduleDecodeBufferTaskIfNeeded();
}

void TizenOmxVideoDecodeAccelerator::Reset() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  decoder_thread_.message_loop()->task_runner()->PostTask(
      FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::ResetTask,
                            base::Unretained(this)));
}

void TizenOmxVideoDecodeAccelerator::DestroyTaskFinish() {
  if (decoder_output_thread_.IsRunning())
    decoder_output_thread_.Stop();

  decoder_current_bitstream_buffer_.reset();
  decoder_current_input_buffer_ = -1;
  decoder_decode_buffer_tasks_scheduled_ = 0;
  while (!decoder_input_queue_.empty())
    decoder_input_queue_.pop();

  // Set our state to kError.  Just in case.
  SetDecoderState(kError);
  output_ctx_.is_error.store(true);
  input_ctx_.is_error.store(true);
  LOG(INFO) << "DestroyTask finished successfully";
}

void TizenOmxVideoDecodeAccelerator::DestroyTask() {
  bool finished = false;
  if (decoder_state_ != kDestroying)
    SetDecoderState(kDestroying);

  if (decoder_state_ == kUninitialized) {
    // DoInit() failed early so simple synchronous destroy is possible
    {
      base::AutoLock out_lock(output_ctx_lock_);
      DestroyOutputBuffer();
    }
    {
      base::AutoLock in_lock(input_ctx_lock_);
      DestroyInputBuffers();
    }
    DestroyOmxComponent();
    ReleaseDecoderResource();
    DestroyTaskFinish();
    return;
  }

  if (component_state_ == OMX_StateExecuting) {
    if (component_req_state_ != OMX_StateIdle)
      SwitchComponentState(OMX_StateIdle);
  } else if (component_state_ == OMX_StateIdle) {
    if (component_req_state_ != OMX_StateLoaded)
      // Switch to loaded state, after that component will
      // wait for depopulating of both ports
      SwitchComponentState(OMX_StateLoaded);

    base::AutoLock in_lock(input_ctx_lock_);
    base::AutoLock out_lock(output_ctx_lock_);
    OmxPortState& in_state = input_ctx_.input_port_state;
    OmxPortState& out_state = output_ctx_.output_port_state;

    // Disable and depopulate ports if needed
    if (out_state == kPortEnabled && out_state != kPortIsDisabling)
      SwitchPortState(kPortDisable, kOmxOutputPortIndex);
    if (out_state == kPortIsDisabling)
      DestroyOutputBuffer();

    if (in_state == kPortEnabled && in_state != kPortIsDisabling)
      SwitchPortState(kPortDisable, kOmxInputPortIndex);
    if (in_state == kPortIsDisabling)
      DestroyInputBuffers();

    // Now decoder should change its state to OMX_StateLoaded
    // And disable both ports
  } else if (component_state_ == OMX_StateLoaded) {
    base::AutoLock in_lock(input_ctx_lock_);
    base::AutoLock out_lock(output_ctx_lock_);
    OmxPortState& in_state = input_ctx_.input_port_state;
    OmxPortState& out_state = output_ctx_.output_port_state;
    if (in_state == kPortDisabled && out_state == kPortDisabled) {
      // We still have buffers only if InitializeOmxComponentAsync() failed
      // early while component was in OMX_StateLoaded
      DestroyOutputBuffer();
      DestroyInputBuffers();

      DestroyOmxComponent();
      LOG(INFO) << "Component destroy completed.";
    }
  } else if (component_state_ == OMX_StateMax) {
    ReleaseDecoderResource();
    finished = true;
  }

  // We must post ourselves until all destroy steps are finished
  // to keep Destroy() waiting thus decoder_thread_ alive and able
  // to receive callbacks (i.e. handle OMX event callbacks)
  if (!finished) {
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::DestroyTask,
                              base::Unretained(this)));
  } else {
    DestroyTaskFinish();
  }
}

void TizenOmxVideoDecodeAccelerator::Destroy() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  LOG(INFO) << "Destroy started";

  // We're destroying; cancel all callbacks.
  client_ptr_factory_.reset();
  weak_this_factory_.InvalidateWeakPtrs();

  // If the decoder thread is running, destroy using posted task.
  if (decoder_thread_.IsRunning()) {
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE, base::Bind(&TizenOmxVideoDecodeAccelerator::DestroyTask,
                              base::Unretained(this)));
    // Signal all conditionals as decoder_thread_ could started wait
    pictures_assigned_.Signal();
    // DestroyTask() will keep decoder_thread_ alive until destroy completes
    decoder_thread_.Stop();
  }
  // If decoder thread is not running it means that Initialize() failed early
  // and all cleanup can be done in this thread

  DestroyOutputRecords();

  if (omx_core_init_done_)
    OMX_Deinit();

  LOG(INFO) << "Destroy finished";
  delete this;
}

void TizenOmxVideoDecodeAccelerator::OnStateSet(OMX_STATETYPE new_state) {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());
  if (component_req_state_ != OMX_StateMax &&
      new_state != component_req_state_) {
    LOG(WARNING) << "Component changed to not expected state="
                 << OmxStateStr(new_state)
                 << " expected=" << OmxStateStr(component_req_state_);
    SetErrorState(FROM_HERE, PLATFORM_FAILURE);
  } else {
    component_req_state_ = OMX_StateMax;
  }

  LOG(INFO) << "Component changed state to: " << OmxStateStr(new_state);
  component_state_ = new_state;

  ContinueInitIfNeeded();
}

bool TizenOmxVideoDecodeAccelerator::SwitchPortState(PortEnableChange change,
                                                     OMX_U32 port_index) {
  if (port_index != kOmxInputPortIndex && port_index != kOmxOutputPortIndex) {
    LOG(ERROR) << "Got event with unknown port index. Erroring";
    SetErrorState(FROM_HERE, INVALID_ARGUMENT);
    return false;
  }

  base::Lock& ctx_lock =
      (port_index == kOmxInputPortIndex) ? input_ctx_lock_ : output_ctx_lock_;
  ctx_lock.AssertAcquired();

  OmxPortState& state = (port_index == kOmxInputPortIndex)
                            ? input_ctx_.input_port_state
                            : output_ctx_.output_port_state;

  // Return true if port is already in desired state or is changing to it
  if ((change == kPortEnable && state == kPortIsEnabling) ||
      (change == kPortDisable && state == kPortIsDisabling)) {
    LOG(WARNING)
        << "Port state change requested while same previous one was ongoing"
        << "port_index=" << port_index << " change=" << change;
    return true;
  }

  if ((change == kPortEnable && state == kPortEnabled) ||
      (change == kPortDisable && state == kPortDisabled)) {
    LOG(WARNING)
        << "Port state change requested while port already in desired state"
        << "port_index=" << port_index << " change=" << change;
    return true;
  }

  // Return false if there is ongoing, opposite change
  if ((change == kPortEnable && state == kPortIsDisabling) ||
      (change == kPortDisable && state == kPortIsEnabling)) {
    LOG(ERROR)
        << "Port state change requested while opposite change was ongoing"
        << "port_index=" << port_index << " change=" << change;
    return false;
  }

  // Normal case
  OMX_COMMANDTYPE cmd =
      (change == kPortEnable) ? OMX_CommandPortEnable : OMX_CommandPortDisable;
  OMX_ERRORTYPE err = OMX_SendCommand(component_, cmd, port_index, 0);
  if (err != OMX_ErrorNone) {
    LOG(ERROR) << "Failed on OMX_SendCommand. change=" << change
               << " port_index=" << port_index;
    return false;
  }

  state = (change == kPortEnable) ? kPortIsEnabling : kPortIsDisabling;
  return true;
}

void TizenOmxVideoDecodeAccelerator::OnPortEnableChanged(
    PortEnableChange change,
    OMX_U32 port_index) {
  if (port_index != kOmxInputPortIndex && port_index != kOmxOutputPortIndex) {
    LOG(ERROR) << "Got event with unknown port index. Erroring";
    SetErrorState(FROM_HERE, INVALID_ARGUMENT);
    return;
  }

  base::Lock& ctx_lock =
      (port_index == kOmxInputPortIndex) ? input_ctx_lock_ : output_ctx_lock_;
  base::AutoLock lock(ctx_lock);

  OmxPortState& state = (port_index == kOmxInputPortIndex)
                            ? input_ctx_.input_port_state
                            : output_ctx_.output_port_state;

  if ((change == kPortEnable && state != kPortIsEnabling) ||
      (change == kPortDisable && state != kPortIsDisabling))
    LOG(WARNING) << "Unexpected OMX port state change port_index=" << port_index
                 << " change=" << change;

  state = (change == kPortEnable) ? kPortEnabled : kPortDisabled;
}

void TizenOmxVideoDecodeAccelerator::OnEventCmdComplete(OMX_COMMANDTYPE cmd,
                                                        OMX_U32 nData2) {
  switch (cmd) {
    case OMX_CommandStateSet:
      OnStateSet((OMX_STATETYPE)nData2);
      break;
    case OMX_CommandFlush:
      OnFlushComplete(nData2);
      break;
    case OMX_CommandPortDisable:
      OnPortEnableChanged(kPortDisable, nData2);
      break;
    case OMX_CommandPortEnable:
      OnPortEnableChanged(kPortEnable, nData2);
      break;
    case OMX_CommandMarkBuffer:
    case OMX_CommandKhronosExtensions:
    case OMX_CommandVendorStartUnused:
    case OMX_CommandComponentDeInit:
    case OMX_CommandEmptyBuffer:
    case OMX_CommandFillBuffer:
    case OMX_CommandFakeBuffer:
      LOG(ERROR) << "Unhandled OnEventCmdComplete " << OmxCmdStr(cmd);
      SetErrorState(FROM_HERE, PLATFORM_FAILURE);
      break;
    default:
      LOG(ERROR) << "Unknown OnEventCmdComplete " << OmxCmdStr(cmd);
      SetErrorState(FROM_HERE, PLATFORM_FAILURE);
      break;
  }
}

void TizenOmxVideoDecodeAccelerator::OnOmxError(OMX_ERRORTYPE err) {
  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());
  LOG(ERROR) << "OMX component generated error: " << OmxErrStr(err);

  SetErrorState(FROM_HERE, PLATFORM_FAILURE);
  ContinueInitIfNeeded();
}

void TizenOmxVideoDecodeAccelerator::OnColourDescription(
    OMX_VIDEO_COLOUR_DESCRIPTION* color_desc) {
  LOG(WARNING) << "This video have color description present."
                  " colour_primaries="
               << static_cast<int>(color_desc->colour_primaries)
               << " transfer_characteristics="
               << static_cast<int>(color_desc->transfer_characteristics)
               << " matrix_coefficient="
               << static_cast<int>(color_desc->matrix_coeffs);
  base::AutoLock lock(output_ctx_lock_);
  media::VideoColorSpace video_color_space = media::VideoColorSpace(
      static_cast<int>(color_desc->colour_primaries),
      static_cast<int>(color_desc->transfer_characteristics),
      static_cast<int>(color_desc->matrix_coeffs),
      gfx::ColorSpace::RangeID::DERIVED);
  output_ctx_.color_space = video_color_space.ToGfxColorSpace();
}

void TizenOmxVideoDecodeAccelerator::OnOmxEvent(OMX_HANDLETYPE hComponent,
                                                OMX_EVENTTYPE eEvent,
                                                OMX_U32 nData1,
                                                OMX_U32 nData2,
                                                OMX_PTR pEventData) {
  if (decoder_thread_.message_loop() != NULL &&
      decoder_thread_.message_loop() != base::MessageLoop::current()) {
    decoder_thread_.message_loop()->task_runner()->PostTask(
        FROM_HERE, base::Bind(base::IgnoreResult(
                                  &TizenOmxVideoDecodeAccelerator::OnOmxEvent),
                              base::Unretained(this), hComponent, eEvent,
                              nData1, nData2, pEventData));
    return;
  }

  DCHECK_EQ(decoder_thread_.message_loop(), base::MessageLoop::current());
  switch (eEvent) {
    case OMX_EventCmdComplete:
      OnEventCmdComplete(static_cast<OMX_COMMANDTYPE>(nData1), nData2);
      break;
    case OMX_EventPortSettingsChanged:
    case OMX_EventBufferFlag:
    case OMX_EventSdpVbiData:
    case OMX_EventSdpMasteringDisplayColourVolume:
      // do nothing
      break;
    case OMX_EventError:
      OnOmxError(static_cast<OMX_ERRORTYPE>(nData1));
      break;
    case OMX_EventSdpColourDescription:
      OnColourDescription(
          static_cast<OMX_VIDEO_COLOUR_DESCRIPTION*>(pEventData));
      break;
    case OMX_EventMark:
    case OMX_EventResourcesAcquired:
    case OMX_EventComponentResumed:
    case OMX_EventDynamicResourcesAvailable:
    case OMX_EventPortFormatDetected:
    case OMX_EventKhronosExtensions:
    case OMX_EventVendorStartUnused:
    case OMX_EventSdpVideoInfo:
    case OMX_EventSdpUserdataEtc:
    case OMX_EventSdphdrcompatibilityinfo:
      LOG(ERROR) << "Unhandled event " << OmxEventStr(eEvent);
      return;
    default:
      LOG(ERROR) << "Got unknown event=" << static_cast<unsigned int>(eEvent);
      return;
  }
}

}  // namespace content
