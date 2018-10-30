// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MEDIA_TIZEN_OMX_VIDEO_DECODER_H_
#define CONTENT_COMMON_MEDIA_TIZEN_OMX_VIDEO_DECODER_H_

#include <atomic>
#include <memory>
#include <queue>
#include <vector>

#include "OMX_Core.h"
#include "OMX_Types.h"
#include "OMX_Video.h"

#include "base/location.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "content/common/content_export.h"
#include "content/common/tizen_resource_manager.h"
#include "media/video/picture.h"
#include "media/video/video_decode_accelerator.h"
#include "ui/gfx/color_space.h"
#include "ui/gfx/geometry/size_f.h"

namespace content {

class CONTENT_EXPORT TizenOmxVideoDecodeAccelerator
    : public media::VideoDecodeAccelerator,
      public content::TizenResourceManager::ResourceClient {
 public:
  TizenOmxVideoDecodeAccelerator();

  // VideoDecodeAccelerator implementation
  bool Initialize(const Config& config, Client* client) override;
  void Decode(const media::BitstreamBuffer& bitstream_buffer) override;
  virtual void AssignPictureBuffers(
      const std::vector<media::PictureBuffer>& buffers);
  virtual void ReusePictureBuffer(int32_t picture_buffer_id);
  void Flush() override;
  void Reset() override;
  void Destroy() override;
  bool TryToSetupDecodeOnSeparateThread(
      const base::WeakPtr<Client>& decode_client,
      const scoped_refptr<base::SingleThreadTaskRunner>& decode_task_runner)
      override;

  // Callbacks from OMX library called on private pthread
  void OnOmxEvent(OMX_HANDLETYPE hComponent,
                  OMX_EVENTTYPE eEvent,
                  OMX_U32 nData1,
                  OMX_U32 nData2,
                  OMX_PTR pEventData);
  void DoEmptyBufferDone(OMX_BUFFERHEADERTYPE* pBuffer);
  void DoFillBufferDone(OMX_BUFFERHEADERTYPE* pBuffer);

 protected:
  // Auto-destruction reference for BitstreamBuffer, for message-passing from
  // Decode() to DecodeTask().
  struct BitstreamBufferRef;
  // Record for decoded pictures that can be sent to PictureReady.
  struct PictureRecord;

  // Describes decoded frame
  struct DecodedVideoFrame {
    // Video frame info
    int32_t timestamp;
    gfx::Size res;
    gfx::Size effective_res;
    gfx::Size next_res;
    OMX_U32 framerate;
    bool is_eos;
    bool show_frame;
    // Buffer info
    uintptr_t y_virtaddr;
    uintptr_t u_virtaddr;
    uintptr_t v_virtaddr;
    unsigned y_linesize;
    unsigned u_linesize;
    unsigned v_linesize;
    // Whether has been sent to client already
    bool is_sent;
    bool contains_picture_info;
    DecodedVideoFrame();
    DecodedVideoFrame(const DecodedVideoFrame&);
    explicit DecodedVideoFrame(const OMX_BUFFERHEADERTYPE*);
    bool IsChangingResolution(void);
    bool IsValid();

   private:
    // Cntains valid info?
    bool is_valid;
  };

  struct PictureBufferDesc {
    explicit PictureBufferDesc(DecodedVideoFrame* frame);
    PictureBufferDesc(void);
    bool IsValid(void) { return is_valid; }
    std::string ToString();
    bool operator!=(const PictureBufferDesc& other);
    bool operator==(const PictureBufferDesc& other);
    gfx::Size size;
    unsigned y_linesize;
    unsigned u_linesize;
    unsigned v_linesize;

   private:
    bool is_valid;
  };

  // Record for output buffers.
  struct OutputRecord {
    OutputRecord();
    virtual ~OutputRecord();
    bool at_client;      // held by client.
    bool is_mapped;      // have texture assigned
    int32_t picture_id;  // picture buffer id as returned to PictureReady().
    int32_t texture_id;
    PictureBufferDesc buffer_desc;
    bool should_destroy;
  };

  // Input buffer
  struct InputFrameBuffer {
    InputFrameBuffer()
        : buf_hdr(nullptr),
          is_free(true),
          at_component(false),
          length(0),
          bytes_used(0),
          address(nullptr),
          input_id(-1) {}
    OMX_BUFFERHEADERTYPE* buf_hdr;
    bool is_free;
    bool at_component;
    size_t length;
    off_t bytes_used;
    void* address;
    int input_id;
  };

  enum OmxPortState {
    kPortEnabled = 0,
    kPortIsDisabling,
    kPortDisabled,
    kPortIsEnabling,
    kPortStateMax,
  };

  enum PortEnableChange {
    kPortDisable,
    kPortEnable,
  };

  // Internal state of the decoder.
  enum State {
    kUninitialized,  // Initialize() not yet called.
    kInitializing,   // Initialize() did all synchronous init steps.
                     // OMX component is initializing asynchronously.
    kInitialized,    // Initialize() returned true; ready to start decoding.
    kDecoding,       // DecodeBufferInitial() successful; decoding frames.
    kResetting,      // Presently resetting.
    kAfterReset,     // After Reset(), ready to start decoding again.
    kChangingResolution,  // Performing resolution change, all remaining
                          // pre-change frames decoded and processed.
    kDestroying,
    kError,  // Error in kDecoding state.
  };

  struct InputContext {
    InputContext() : last_pts(0), input_port_state(kPortDisabled) {
      is_error.store(false);
    }
    int last_pts;
    std::queue<int> input_frames_free;
    OmxPortState input_port_state;
    // can be changed without input_ctx_lock_ held
    std::atomic<bool> is_error;
  };

  struct OmxOutputBuffer {
    OmxOutputBuffer()
        : buf_hdr(nullptr), at_component(false), init_done(false) {}
    OMX_BUFFERHEADERTYPE* buf_hdr;
    bool at_component;
    bool init_done;
  };

  struct OutputContext {
    OutputContext()
        : decoder_state(kUninitialized),
          decoder_is_flushing(false),
          no_decoded_frames(0),
          output_buffers_requested_count(0),
          output_port_state(kPortDisabled) {
      is_error.store(false);
    }
    std::list<int> output_records_free;
    OmxOutputBuffer output_buffer;
    DecodedVideoFrame last_frame;
    State decoder_state;
    bool decoder_is_flushing;
    int no_decoded_frames;
    // nonzero value means GrowOutputRecordsTask() was started
    int output_buffers_requested_count;
    PictureBufferDesc output_buffers_requested_desc;
    OmxPortState output_port_state;
    gfx::ColorSpace color_space;
    // can be changed without output_ctx_lock_ held
    std::atomic<bool> is_error;
  };

  enum InitResult {
    kInitPending,
    kInitOk,
    kInitError,
  };

  // Input related functions
  void DecodeBufferTask();
  void DecodeTask(const media::BitstreamBuffer& bitstream_buffer);
  void ScheduleDecodeBufferTaskIfNeeded();
  bool AppendAndFlushInputFrame(const void* data, size_t size);
  bool AppendToInputFrameBuffer(const void* data, size_t size);
  // Must be called with input_ctx_lock_ held */
  bool DoEmptyThisBuffer(int idx);

  // Output related functions
  bool DoPictureReady();
  void ReusePictureBufferTask(int32_t picture_buffer_id);
  void DoFillThisBuffer();
  void ProcessLastFrame();
  bool GrowOutputRecords();
  void GrowOutputRecordsTask();
  bool ScheduleGrowRecords(const PictureBufferDesc& desc);
  void MarkRecordsForDeletion(PictureBufferDesc* with_desc);
  virtual unsigned TexturesPerPictureBuffer() const = 0;
  virtual media::VideoPixelFormat VideoPixelFormat() const = 0;
  virtual uint32_t TextureTarget() const = 0;
  virtual bool ProcessOutputPicture(media::Picture* picture,
                                    DecodedVideoFrame* last_frame,
                                    OutputRecord* free_record);

  // OMX related functions
  bool InitializeOmxComponent();
  bool InitializeOmxComponentBasic();
  InitResult InitializeOmxComponentAsync();
  bool DestroyOmxComponent();
  bool SetOmxDecoderInputType();
  bool SwitchComponentState(OMX_STATETYPE new_state);
  bool SwitchPortState(PortEnableChange change, OMX_U32 port_index);
  // Event handlers
  void OnEventCmdComplete(OMX_COMMANDTYPE nData1, OMX_U32 nData2);
  void OnStateSet(OMX_STATETYPE new_state);
  void OnFlushComplete(OMX_U32 port_index);
  void OnOmxError(OMX_ERRORTYPE err);
  void OnPortEnableChanged(PortEnableChange change, OMX_U32 port_index);
  void OnColourDescription(OMX_VIDEO_COLOUR_DESCRIPTION* color_desc);

  // TizenResourceManager related functions
  virtual bool FillRequestedResources(
      std::vector<TizenRequestedResource>* devices) = 0;
  bool RequestDecoderResource(void);
  void ReleaseDecoderResource(void);
  bool RMResourceCallback(base::WaitableEvent* wait,
                          TizenResourceConflictType type,
                          const std::vector<int>& ids);

  // Buffer stuff
  virtual unsigned OutputRecordsCount() const = 0;
  virtual unsigned InputFrameBufferSize() const = 0;
  virtual unsigned InputFrameBuffersCount() const = 0;
  bool CreateInputBuffers();
  bool CreateOutputBuffer();
  bool DestroyOutputBuffer();
  bool DestroyInputBuffers();
  virtual void DestroyOutputRecords();
  virtual bool DestroyOutputRecord(int idx);
  void PruneOutputRecordsTask();
  virtual OutputRecord* AllocateOutputRecord() = 0;
  virtual bool InitOutputRecord(const media::PictureBuffer& picture_buffer,
                                OutputRecord* new_record) = 0;

  // Other functions
  void SetErrorState(const base::Location& from, Error error);
  void NotifyError(Error error);
  void DoSetErrorState(Error error);
  void FlushTask();
  void FinishFlush();
  void ResetTask();
  void ResetDoneTask();
  void DestroyTaskFinish();
  void DestroyTask();
  void SetDecoderState(State new_state);
  void InitTask();
  InitResult DoInit();
  void ContinueInitIfNeeded();
  bool IsProfileSupported(media::VideoCodecProfile profile) const;
  virtual bool EarlyInit();
  virtual bool PostInit();
  virtual std::vector<media::VideoCodecProfile> SupportedProfiles() const = 0;
  // Decoder variables

  // The message loop that created the class. Used for all callbacks. This
  // class expects all calls to this class to be on this message loop (not
  // checked).
  scoped_refptr<base::SingleThreadTaskRunner> child_task_runner_;
  base::WeakPtrFactory<TizenOmxVideoDecodeAccelerator> weak_this_factory_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;
  // Callbacks to |io_client_| must be executed on |io_task_runner_|.
  base::WeakPtr<Client> io_client_;
  base::Thread decoder_thread_;
  base::Thread decoder_output_thread_;
  std::unique_ptr<base::WeakPtrFactory<Client>> client_ptr_factory_;
  base::WeakPtr<Client> client_;
  base::WaitableEvent init_done_;
  bool init_success_;
  State decoder_state_;
  bool decoder_flushing_;

  // Read only variables set once during init
  // Those can be accessed from other threads without
  // any locking.
  media::VideoCodecProfile video_profile_;

#if defined(TIZEN_PEPPER_EXTENSIONS)
  // Mode used to configure latency mode of decoder.
  // In low latency mode only content with 'I' and 'P' frames is supported,
  // but it is possible to call GetPicture after each performed frame decode.
  using VideoLatencyMode =
      media::VideoDecodeAccelerator::Config::VideoLatencyMode;
  VideoLatencyMode latency_mode_;
#endif

  // Input related variables
  // input_ctx_ is protected by input_ctx_lock_;
  // can be accessed from private OMX's threads
  InputContext input_ctx_;
  base::Lock input_ctx_lock_;
  // All below fields are accessed only from decoder_thread_
  std::unique_ptr<BitstreamBufferRef> decoder_current_bitstream_buffer_;
  int decoder_delay_bitstream_buffer_id_;
  std::queue<linked_ptr<BitstreamBufferRef>> decoder_input_queue_;
  std::vector<InputFrameBuffer> input_frames_;
  int decoder_current_input_buffer_;
  int decoder_decode_buffer_tasks_scheduled_;

  // Output related variables
  // output_ctx_ is protected by output_ctx_lock_;
  // can be accessed from private OMX's threads
  OutputContext output_ctx_;
  base::Lock output_ctx_lock_;
  std::vector<std::unique_ptr<OutputRecord>> output_records_;
  base::WaitableEvent pictures_assigned_;

  // TvResourceManager related variables
  base::Lock resource_lock_;
  TizenResourceList allocated_resources_;
  base::WaitableEvent* resource_released_event_;
  std::string omx_component_name_;

  // OMX related variables
  OMX_CALLBACKTYPE callbacks_;
  OMX_HANDLETYPE component_;
  OMX_VERSIONTYPE component_ver_;
  OMX_STATETYPE component_state_;
  // state we currently changing to; OMX_StateMax means no info
  OMX_STATETYPE component_req_state_;
  int component_flush_requested_;
  bool omx_core_init_done_;

  ~TizenOmxVideoDecodeAccelerator() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TizenOmxVideoDecodeAccelerator);
};

}  // namespace content

#endif  // CONTENT_COMMON_MEDIA_TIZEN_OMX_VIDEO_DECODER_H_
