// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_host.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "cc/blink/web_layer_impl.h"
#include "content/browser/renderer_host/pepper/browser_host_callback_helpers.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_data_source_host.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_data_source_private.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_private.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_media_player_private_factory.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_url_helper.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "media/base/bind_to_current_loop.h"
#include "media/blink/video_frame_compositor.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/proxy/proxy_config.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/error_conversion.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/proxy/dispatch_reply_message.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/file_type_conversion.h"
#include "ppapi/shared_impl/ppb_view_shared.h"

namespace content {

namespace {

PP_Rect IntRectIntersection(PP_Rect first, PP_Rect second) {
  PP_Rect intersection;
  intersection.point.x = std::max(first.point.x, second.point.x);
  intersection.point.y = std::max(first.point.y, second.point.y);
  auto x_end = std::min(first.point.x + first.size.width,
                        second.point.x + second.size.width);
  auto y_end = std::min(first.point.y + first.size.height,
                        second.point.y + second.size.height);
  if (x_end < intersection.point.x || y_end < intersection.point.y)
    return PP_MakeRectFromXYWH(0, 0, 0, 0);
  intersection.size.width = x_end - intersection.point.x;
  intersection.size.height = y_end - intersection.point.y;
  return intersection;
}

float VisibleCropRatio(int32_t video_dimension, int32_t visible_dimension) {
  if (video_dimension <= visible_dimension || video_dimension == 0)
    return 1.0f;
  float normalized =
      static_cast<float>(video_dimension - visible_dimension) / video_dimension;
  return 1.0f - normalized;
}

class MediaEventsListenerImpl : public MediaEventsListenerPrivate {
 public:
  static std::unique_ptr<MediaEventsListenerImpl> Create(
      base::WeakPtr<PepperMediaPlayerHost>);
  ~MediaEventsListenerImpl() override;

  void OnTimeUpdate(PP_TimeTicks) override;
  void OnEnded() override;
  void OnError(PP_MediaPlayerError) override;

 private:
  explicit MediaEventsListenerImpl(base::WeakPtr<PepperMediaPlayerHost>);
  base::WeakPtr<PepperMediaPlayerHost> host_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  DISALLOW_COPY_AND_ASSIGN(MediaEventsListenerImpl);
};

class SubtitleListenerImpl : public SubtitleListenerPrivate {
 public:
  static std::unique_ptr<SubtitleListenerImpl> Create(
      base::WeakPtr<PepperMediaPlayerHost>);
  ~SubtitleListenerImpl() override;

  void OnShowSubtitle(PP_TimeDelta, const std::string&) override;

 private:
  explicit SubtitleListenerImpl(base::WeakPtr<PepperMediaPlayerHost>);
  base::WeakPtr<PepperMediaPlayerHost> host_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  DISALLOW_COPY_AND_ASSIGN(SubtitleListenerImpl);
};

class BufferingListenerImpl : public BufferingListenerPrivate {
 public:
  static std::unique_ptr<BufferingListenerImpl> Create(
      base::WeakPtr<PepperMediaPlayerHost>);
  ~BufferingListenerImpl() override;

  void OnBufferingStart() override;
  void OnBufferingProgress(int percent) override;
  void OnBufferingComplete() override;

 private:
  explicit BufferingListenerImpl(base::WeakPtr<PepperMediaPlayerHost>);
  base::WeakPtr<PepperMediaPlayerHost> host_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  DISALLOW_COPY_AND_ASSIGN(BufferingListenerImpl);
};

class DRMListenerImpl : public DRMListenerPrivate {
 public:
  static std::unique_ptr<DRMListenerImpl> Create(
      base::WeakPtr<PepperMediaPlayerHost>);
  ~DRMListenerImpl() override;

  void OnInitDataLoaded(PP_MediaPlayerDRMType,
                        const std::vector<uint8_t>& data) override;
  void OnLicenseRequest(const std::vector<uint8_t>& request) override;

 private:
  explicit DRMListenerImpl(base::WeakPtr<PepperMediaPlayerHost>);
  base::WeakPtr<PepperMediaPlayerHost> host_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  DISALLOW_COPY_AND_ASSIGN(DRMListenerImpl);
};

class UrlSubtitlesDownloader
    : public PepperMediaPlayerHost::SubtitlesDownloader,
      public net::URLRequest::Delegate {
 public:
  UrlSubtitlesDownloader(
      const GURL& url,
      const base::Callback<base::ScopedTempDir*(void)>& temp_dir_callback,
      const base::Callback<void(int32_t, const base::FilePath&)>& callback);
  ~UrlSubtitlesDownloader() override;

  void OnResponseStarted(net::URLRequest* request) override;
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override;

 private:
  void StartDownload(const GURL& url);
  void ReadData();

  std::unique_ptr<net::URLRequestContext> request_context_;
  std::unique_ptr<net::URLRequest> url_request_;
  scoped_refptr<net::IOBufferWithSize> read_buffer_;

  base::File file_;
  base::FilePath file_path_;

  base::WeakPtrFactory<UrlSubtitlesDownloader> factory_;
};

class FileSubtitlesDownloader
    : public PepperMediaPlayerHost::SubtitlesDownloader {
 public:
  FileSubtitlesDownloader(
      const GURL& url,
      const base::Callback<void(int32_t, const base::FilePath&)>& callback);
  ~FileSubtitlesDownloader() override;
};

template <typename MsgClass, typename... Args>
void SendEventMessage(base::WeakPtr<PepperMediaPlayerHost> host,
                      const Args&... args) {
  // This check is safe bacause this method and host deletion
  // are both performed on main thread
  if (!host.get())
    return;

  host.get()->host()->SendUnsolicitedReply(host.get()->pp_resource(),
                                           MsgClass(args...));
}

int32_t HttpErrorToPepperError(int err) {
  switch (err) {
    case 401:
    case 403:
      LOG(ERROR) << "Insufficient permission";
      return PP_ERROR_NOACCESS;
    case 404:
    case 410:
      LOG(ERROR) << "File not found, error = " << err;
      return PP_ERROR_FILENOTFOUND;
    default:
      LOG(ERROR) << "Unresolved HTTP error code = " << err;
      return PP_ERROR_FAILED;
  }
}

std::unique_ptr<MediaEventsListenerImpl> MediaEventsListenerImpl::Create(
    base::WeakPtr<PepperMediaPlayerHost> host) {
  return std::unique_ptr<MediaEventsListenerImpl>{
      new MediaEventsListenerImpl(host)};
}

MediaEventsListenerImpl::~MediaEventsListenerImpl() = default;

void MediaEventsListenerImpl::OnTimeUpdate(PP_TimeTicks time) {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(
          &SendEventMessage<PpapiPluginMsg_MediaEventsListener_OnTimeUpdate,
                            PP_TimeTicks>,
          host_, time));
}

void MediaEventsListenerImpl::OnEnded() {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SendEventMessage<PpapiPluginMsg_MediaEventsListener_OnEnded>,
                 host_));
}

void MediaEventsListenerImpl::OnError(PP_MediaPlayerError error) {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SendEventMessage<PpapiPluginMsg_MediaEventsListener_OnError,
                                   PP_MediaPlayerError>,
                 host_, error));
}

MediaEventsListenerImpl::MediaEventsListenerImpl(
    base::WeakPtr<PepperMediaPlayerHost> host)
    : host_(host), task_runner_(base::MessageLoop::current()->task_runner()) {}

std::unique_ptr<SubtitleListenerImpl> SubtitleListenerImpl::Create(
    base::WeakPtr<PepperMediaPlayerHost> host) {
  return std::unique_ptr<SubtitleListenerImpl>{new SubtitleListenerImpl(host)};
}

SubtitleListenerImpl::~SubtitleListenerImpl() = default;

void SubtitleListenerImpl::OnShowSubtitle(PP_TimeDelta duration,
                                          const std::string& text) {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(
          &SendEventMessage<PpapiPluginMsg_SubtitleListener_OnShowSubtitle,
                            PP_TimeDelta, std::string>,
          host_, duration, text));
}

SubtitleListenerImpl::SubtitleListenerImpl(
    base::WeakPtr<PepperMediaPlayerHost> host)
    : host_(host), task_runner_(base::MessageLoop::current()->task_runner()) {}

std::unique_ptr<BufferingListenerImpl> BufferingListenerImpl::Create(
    base::WeakPtr<PepperMediaPlayerHost> host) {
  return std::unique_ptr<BufferingListenerImpl>{
      new BufferingListenerImpl(host)};
}

BufferingListenerImpl::~BufferingListenerImpl() = default;

void BufferingListenerImpl::OnBufferingStart() {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(
          &SendEventMessage<PpapiPluginMsg_BufferingListener_OnBufferingStart>,
          host_));
}

void BufferingListenerImpl::OnBufferingProgress(int percent) {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SendEventMessage<
                     PpapiPluginMsg_BufferingListener_OnBufferingProgress, int>,
                 host_, percent));
}

void BufferingListenerImpl::OnBufferingComplete() {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SendEventMessage<
                     PpapiPluginMsg_BufferingListener_OnBufferingComplete>,
                 host_));
}

BufferingListenerImpl::BufferingListenerImpl(
    base::WeakPtr<PepperMediaPlayerHost> host)
    : host_(host), task_runner_(base::MessageLoop::current()->task_runner()) {}

std::unique_ptr<DRMListenerImpl> DRMListenerImpl::Create(
    base::WeakPtr<PepperMediaPlayerHost> host) {
  return std::unique_ptr<DRMListenerImpl>{new DRMListenerImpl(host)};
}

DRMListenerImpl::~DRMListenerImpl() = default;

void DRMListenerImpl::OnInitDataLoaded(PP_MediaPlayerDRMType type,
                                       const std::vector<uint8_t>& data) {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SendEventMessage<PpapiPluginMsg_DRMListener_OnInitDataLoaded,
                                   PP_MediaPlayerDRMType, std::vector<uint8_t>>,
                 host_, type, data));
}

void DRMListenerImpl::OnLicenseRequest(const std::vector<uint8_t>& request) {
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&SendEventMessage<PpapiPluginMsg_DRMListener_OnLicenseRequest,
                                   std::vector<uint8_t>>,
                 host_, request));
}

DRMListenerImpl::DRMListenerImpl(base::WeakPtr<PepperMediaPlayerHost> host)
    : host_(host), task_runner_(base::MessageLoop::current()->task_runner()) {}

UrlSubtitlesDownloader::UrlSubtitlesDownloader(
    const GURL& url,
    const base::Callback<base::ScopedTempDir*(void)>& temp_dir_callback,
    const base::Callback<void(int32_t, const base::FilePath&)>& callback)
    : PepperMediaPlayerHost::SubtitlesDownloader(callback), factory_(this) {
  base::ScopedTempDir* temp_dir = temp_dir_callback.Run();
  if (!temp_dir) {
    LOG(WARNING) << "Failed to create temporary directory";
    PostDownloadCompleted(PP_ERROR_NOSPACE, base::FilePath());
    return;
  }

  if (!CreateTemporaryFileInDir(temp_dir->GetPath(), &file_path_)) {
    LOG(WARNING) << "Failed to create temporary file";
    PostDownloadCompleted(PP_ERROR_NOSPACE, base::FilePath());
    return;
  }

  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&UrlSubtitlesDownloader::StartDownload,
                                     factory_.GetWeakPtr(), url));
}

UrlSubtitlesDownloader::~UrlSubtitlesDownloader() = default;

void UrlSubtitlesDownloader::OnResponseStarted(net::URLRequest* request) {
  DCHECK(url_request_.get() == request);

  if (!request->status().is_success()) {
    PostDownloadCompleted(
        ppapi::host::NetErrorToPepperError(request->status().error()),
        base::FilePath());
    return;
  }

  // Check for failed HTTP request
  if (request->GetResponseCode() != -1 && request->GetResponseCode() != 200) {
    PostDownloadCompleted(HttpErrorToPepperError(request->GetResponseCode()),
                          base::FilePath());
    return;
  }

  ReadData();
}

void UrlSubtitlesDownloader::OnReadCompleted(net::URLRequest* request,
                                             int bytes_read) {
  DCHECK(url_request_.get() == request);
  if (bytes_read == 0) {
    PostDownloadCompleted(PP_OK, file_path_);
    return;
  }

  if (bytes_read == -1) {
    PostDownloadCompleted(
        ppapi::host::NetErrorToPepperError(request->status().error()),
        base::FilePath());
    return;
  }

  file_.WriteAtCurrentPos(read_buffer_->data(), bytes_read);

  ReadData();
}

void UrlSubtitlesDownloader::StartDownload(const GURL& url) {
  file_ = base::File(file_path_,
                     base::File::FLAG_OPEN_TRUNCATED | base::File::FLAG_WRITE);
  if (!file_.IsValid()) {
    LOG(WARNING) << "Failed to open temporary file";
    PostDownloadCompleted(PP_ERROR_FAILED, base::FilePath());
    return;
  }

  read_buffer_ = new net::IOBufferWithSize(1024 * 1024);
  if (!read_buffer_) {
    LOG(WARNING) << "Failed to create buffer";
    PostDownloadCompleted(PP_ERROR_NOMEMORY, base::FilePath());
    return;
  }

  net::URLRequestContextBuilder builder;
  builder.set_proxy_config_service(
      base::MakeUnique<net::ProxyConfigServiceFixed>(
          net::ProxyConfig::CreateDirect()));
  request_context_ = builder.Build();
  if (!request_context_) {
    LOG(WARNING) << "Failed to create request context";
    PostDownloadCompleted(PP_ERROR_FAILED, base::FilePath());
    return;
  }

  url_request_ =
      request_context_->CreateRequest(url, net::DEFAULT_PRIORITY, this);
  if (!url_request_) {
    LOG(WARNING) << "Failed to create request";
    PostDownloadCompleted(PP_ERROR_FAILED, base::FilePath());
    return;
  }

  url_request_->Start();
}

void UrlSubtitlesDownloader::ReadData() {
  int bytes_read;
  while (url_request_->Read(read_buffer_.get(), read_buffer_->size(),
                            &bytes_read)) {
    if (bytes_read == 0) {
      PostDownloadCompleted(PP_OK, file_path_);
      return;
    }

    file_.WriteAtCurrentPos(read_buffer_->data(), bytes_read);
  }

  if (!url_request_->status().is_success()) {
    PostDownloadCompleted(PP_ERROR_FAILED, base::FilePath());
    return;
  }
}

FileSubtitlesDownloader::FileSubtitlesDownloader(
    const GURL& url,
    const base::Callback<void(int32_t, const base::FilePath&)>& callback)
    : PepperMediaPlayerHost::SubtitlesDownloader(callback) {
  auto file_path = base::FilePath(url.PathForRequest());
  auto file =
      base::File(file_path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  int32_t pp_error = PP_OK;
  if (!file.IsValid())
    pp_error = ppapi::FileErrorToPepperError(file.error_details());

  PostDownloadCompleted(pp_error, file_path);
}

FileSubtitlesDownloader::~FileSubtitlesDownloader() = default;

}  // namespace

PepperMediaPlayerHost::PepperMediaPlayerHost(BrowserPpapiHost* host,
                                             PP_Instance instance,
                                             PP_Resource resource,
                                             PP_MediaPlayerMode player_mode)
    : ResourceHost(host->GetPpapiHost(), instance, resource),
      video_position_(PP_MakeRectFromXYWH(0, 0, 0, 0)),
      clip_rect_(PP_MakeRectFromXYWH(0, 0, 0, 0)),
      plugin_rect_initialized_(false),
      browser_ppapi_host_(host),
      private_(PepperMediaPlayerPrivateFactory::GetInstance().CreateMediaPlayer(
          player_mode)),
      factory_(this) {}

PepperMediaPlayerHost::~PepperMediaPlayerHost() {
  if (display_rect_cb_) {
    display_rect_cb_.Run(PP_ERROR_ABORTED);
    display_rect_cb_.Reset();
  }

  private_->SetBufferingListener(nullptr);
  private_->SetDRMListener(nullptr);
  private_->SetMediaEventsListener(nullptr);
  private_->SetSubtitleListener(nullptr);
}

bool PepperMediaPlayerHost::IsMediaPlayerHost() {
  return true;
}

int32_t PepperMediaPlayerHost::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(PepperMediaPlayerHost, msg)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_MediaPlayer_SetMediaEventsListener, OnSetMediaEventsListener)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_MediaPlayer_SetSubtitleListener, OnSetSubtitleListener)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_MediaPlayer_SetBufferingListener, OnSetBufferingListener)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_SetDRMListener,
                                    OnSetDRMListener)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_AttachDataSource,
                                    OnAttachDataSource)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_MediaPlayer_Play, OnPlay)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_MediaPlayer_Pause, OnPause)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_MediaPlayer_Stop, OnStop)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_Seek, OnSeek)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_SetPlaybackRate,
                                    OnSetPlaybackRate)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_MediaPlayer_GetDuration,
                                      OnGetDuration)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_MediaPlayer_GetCurrentTime,
                                      OnGetCurrentTime)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(PpapiHostMsg_MediaPlayer_GetPlayerState,
                                      OnGetPlayerState)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
      PpapiHostMsg_MediaPlayer_GetCurrentVideoTrackInfo,
      OnGetCurrentVideoTrackInfo)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
      PpapiHostMsg_MediaPlayer_GetVideoTracksList, OnGetVideoTracksList)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
      PpapiHostMsg_MediaPlayer_GetCurrentAudioTrackInfo,
      OnGetCurrentAudioTrackInfo)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
      PpapiHostMsg_MediaPlayer_GetAudioTracksList, OnGetAudioTracksList)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
      PpapiHostMsg_MediaPlayer_GetCurrentTextTrackInfo,
      OnGetCurrentTextTrackInfo)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL_0(
      PpapiHostMsg_MediaPlayer_GetTextTracksList, OnGetTextTracksList)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_SelectTrack,
                                    OnSelectTrack)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(
      PpapiHostMsg_MediaPlayer_AddExternalSubtitles, OnAddExternalSubtitles)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_SetSubtitlesDelay,
                                    OnSetSubtitlesDelay)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_SetDisplayRect,
                                    OnSetDisplayRect)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_SetDisplayMode,
                                    OnSetDisplayMode)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_SetVr360Mode,
                                    OnSetVr360Mode)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_SetVr360Rotation,
                                    OnSetVr360Rotation)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_SetVr360ZoomLevel,
                                    OnSetVr360ZoomLevel)
  PPAPI_DISPATCH_HOST_RESOURCE_CALL(PpapiHostMsg_MediaPlayer_SetDRMSpecificData,
                                    OnSetDRMSpecificData)
  PPAPI_END_MESSAGE_MAP()
  LOG(ERROR) << "Resource message unresolved";
  return PP_ERROR_FAILED;
}

int32_t PepperMediaPlayerHost::OnSetMediaEventsListener(
    ppapi::host::HostMessageContext*,
    bool is_set) {
  private_->SetMediaEventsListener(
      is_set ? MediaEventsListenerImpl::Create(factory_.GetWeakPtr())
             : nullptr);
  return PP_OK;
}

int32_t PepperMediaPlayerHost::OnSetSubtitleListener(
    ppapi::host::HostMessageContext*,
    bool is_set) {
  private_->SetSubtitleListener(
      is_set ? SubtitleListenerImpl::Create(factory_.GetWeakPtr()) : nullptr);
  return PP_OK;
}

int32_t PepperMediaPlayerHost::OnSetBufferingListener(
    ppapi::host::HostMessageContext*,
    bool is_set) {
  private_->SetBufferingListener(
      is_set ? BufferingListenerImpl::Create(factory_.GetWeakPtr()) : nullptr);
  return PP_OK;
}

int32_t PepperMediaPlayerHost::OnSetDRMListener(
    ppapi::host::HostMessageContext*,
    bool is_set) {
  private_->SetDRMListener(
      is_set ? DRMListenerImpl::Create(factory_.GetWeakPtr()) : nullptr);
  return PP_OK;
}

int32_t PepperMediaPlayerHost::OnAttachDataSource(
    ppapi::host::HostMessageContext* context,
    PP_Resource data_source) {
  if (!data_source) {
    LOG(ERROR) << "Invalid data source";
    return PP_ERROR_BADARGUMENT;
  }

  ResourceHost* resource_host = host()->GetResourceHost(data_source);
  if (!resource_host || !resource_host->IsMediaDataSourceHost()) {
    LOG(ERROR) << "Invalid resource host";
    return PP_ERROR_BADARGUMENT;
  }

  PepperMediaDataSourceHost* data_source_host =
      static_cast<PepperMediaDataSourceHost*>(resource_host);
  PepperMediaDataSourcePrivate* media_data_source_private =
      data_source_host->MediaDataSourcePrivate();
  if (!media_data_source_private) {
    LOG(ERROR) << "MediaDataSource implementation is not set";
    return PP_ERROR_FAILED;
  }

  private_->AttachDataSource(
      media_data_source_private,
      base::Bind(
          &ReplyCallback<PpapiPluginMsg_MediaPlayer_AttachDataSourceReply>,
          pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnPlay(
    ppapi::host::HostMessageContext* context) {
  private_->Play(
      base::Bind(&ReplyCallback<PpapiPluginMsg_MediaPlayer_PlayReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnPause(
    ppapi::host::HostMessageContext* context) {
  private_->Pause(
      base::Bind(&ReplyCallback<PpapiPluginMsg_MediaPlayer_PauseReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnStop(
    ppapi::host::HostMessageContext* context) {
  private_->Stop(
      base::Bind(&ReplyCallback<PpapiPluginMsg_MediaPlayer_StopReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnSeek(ppapi::host::HostMessageContext* context,
                                      PP_TimeTicks time) {
  private_->Seek(
      time, base::Bind(&ReplyCallback<PpapiPluginMsg_MediaPlayer_SeekReply>,
                       pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnSetPlaybackRate(
    ppapi::host::HostMessageContext* context,
    double rate) {
  private_->SetPlaybackRate(
      rate, base::Bind(
                &ReplyCallback<PpapiPluginMsg_MediaPlayer_SetPlaybackRateReply>,
                pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnGetDuration(
    ppapi::host::HostMessageContext* context) {
  private_->GetDuration(base::Bind(
      &ReplyCallbackWithValueOutput<PpapiPluginMsg_MediaPlayer_GetDurationReply,
                                    PP_TimeDelta>,
      pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnGetCurrentTime(
    ppapi::host::HostMessageContext* context) {
  private_->GetCurrentTime(base::Bind(
      &ReplyCallbackWithValueOutput<
          PpapiPluginMsg_MediaPlayer_GetCurrentTimeReply, PP_TimeTicks>,
      pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnGetPlayerState(
    ppapi::host::HostMessageContext* context) {
  private_->GetPlayerState(base::Bind(
      &ReplyCallbackWithValueOutput<
          PpapiPluginMsg_MediaPlayer_GetPlayerStateReply, PP_MediaPlayerState>,
      pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnGetCurrentVideoTrackInfo(
    ppapi::host::HostMessageContext* context) {
  private_->GetCurrentVideoTrackInfo(
      base::Bind(&ReplyCallbackWithRefOutput<
                     PpapiPluginMsg_MediaPlayer_GetCurrentVideoTrackInfoReply,
                     PP_VideoTrackInfo>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnGetVideoTracksList(
    ppapi::host::HostMessageContext* context) {
  private_->GetVideoTracksList(
      base::Bind(&ReplyCallbackWithRefOutput<
                     PpapiPluginMsg_MediaPlayer_GetVideoTracksListReply,
                     std::vector<PP_VideoTrackInfo>>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnGetCurrentAudioTrackInfo(
    ppapi::host::HostMessageContext* context) {
  private_->GetCurrentAudioTrackInfo(
      base::Bind(&ReplyCallbackWithRefOutput<
                     PpapiPluginMsg_MediaPlayer_GetCurrentAudioTrackInfoReply,
                     PP_AudioTrackInfo>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnGetAudioTracksList(
    ppapi::host::HostMessageContext* context) {
  private_->GetAudioTracksList(
      base::Bind(&ReplyCallbackWithRefOutput<
                     PpapiPluginMsg_MediaPlayer_GetAudioTracksListReply,
                     std::vector<PP_AudioTrackInfo>>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnGetCurrentTextTrackInfo(
    ppapi::host::HostMessageContext* context) {
  private_->GetCurrentTextTrackInfo(
      base::Bind(&ReplyCallbackWithRefOutput<
                     PpapiPluginMsg_MediaPlayer_GetCurrentTextTrackInfoReply,
                     PP_TextTrackInfo>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnGetTextTracksList(
    ppapi::host::HostMessageContext* context) {
  private_->GetTextTracksList(
      base::Bind(&ReplyCallbackWithRefOutput<
                     PpapiPluginMsg_MediaPlayer_GetTextTracksListReply,
                     std::vector<PP_TextTrackInfo>>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnSelectTrack(
    ppapi::host::HostMessageContext* context,
    PP_ElementaryStream_Type_Samsung streamType,
    uint32_t trackIndex) {
  private_->SelectTrack(
      streamType, trackIndex,
      base::Bind(&ReplyCallback<PpapiPluginMsg_MediaPlayer_SelectTrackReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnAddExternalSubtitles(
    ppapi::host::HostMessageContext* context,
    const std::string& url,
    const std::string& encoding) {
  if (downloader_) {
    LOG(ERROR) << "OnAddExternalSubtitles is already in progress";
    return PP_ERROR_INPROGRESS;
  }

  GURL resolved_url =
      PepperUrlHelper::ResolveURL(browser_ppapi_host_, pp_instance(), url);
  if (!resolved_url.is_valid()) {
    LOG(ERROR) << url << " cannot be properly resolved";
    return PP_ERROR_FAILED;
  }

  downloader_ = SubtitlesDownloader::Create(
      resolved_url,
      base::Bind(&PepperMediaPlayerHost::GetTempDir, base::Unretained(this)),
      base::Bind(&PepperMediaPlayerHost::OnSubtitlesDownloaded,
                 factory_.GetWeakPtr(), context->MakeReplyMessageContext(),
                 encoding));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnSetSubtitlesDelay(
    ppapi::host::HostMessageContext* context,
    PP_TimeDelta delay) {
  private_->SetSubtitlesDelay(
      delay,
      base::Bind(
          &ReplyCallback<PpapiPluginMsg_MediaPlayer_SetSubtitlesDelayReply>,
          pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnSetDisplayRect(
    ppapi::host::HostMessageContext* context,
    const PP_Rect& rect) {
  video_position_ = rect;
  if (!UpdateVideoPosition(base::Bind(
          &ReplyCallback<PpapiPluginMsg_MediaPlayer_SetDisplayRectReply>,
          pp_instance(), context->MakeReplyMessageContext()))) {
    LOG(ERROR) << "UpdateVideoPosition failed";
    return PP_ERROR_FAILED;
  }
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnSetDisplayMode(
    ppapi::host::HostMessageContext* context,
    PP_MediaPlayerDisplayMode display_mode) {
  private_->SetDisplayMode(
      display_mode,
      base::Bind(&ReplyCallback<PpapiPluginMsg_MediaPlayer_SetDisplayModeReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnSetVr360Mode(
    ppapi::host::HostMessageContext* context,
    PP_MediaPlayerVr360Mode vr360_mode) {
  private_->SetVr360Mode(
      vr360_mode,
      base::Bind(&ReplyCallback<PpapiPluginMsg_MediaPlayer_SetVr360ModeReply>,
                 pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnSetVr360Rotation(
    ppapi::host::HostMessageContext* context,
    float horizontal_angle,
    float vertical_angle) {
  private_->SetVr360Rotation(
      horizontal_angle, vertical_angle,
      base::Bind(
          &ReplyCallback<PpapiPluginMsg_MediaPlayer_SetVr360RotationReply>,
          pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnSetVr360ZoomLevel(
    ppapi::host::HostMessageContext* context,
    uint32_t zoom_level) {
  private_->SetVr360ZoomLevel(
      zoom_level,
      base::Bind(
          &ReplyCallback<PpapiPluginMsg_MediaPlayer_SetVr360ZoomLevelReply>,
          pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

int32_t PepperMediaPlayerHost::OnSetDRMSpecificData(
    ppapi::host::HostMessageContext* context,
    PP_MediaPlayerDRMType type,
    PP_MediaPlayerDRMOperation operation,
    const std::string& data) {
  private_->SetDRMSpecificData(
      type, operation, data.data(), data.length(),
      base::Bind(
          &ReplyCallback<PpapiPluginMsg_MediaPlayer_SetDRMSpecificDataReply>,
          pp_instance(), context->MakeReplyMessageContext()));
  return PP_OK_COMPLETIONPENDING;
}

bool PepperMediaPlayerHost::UpdateVideoPosition(
    const base::Callback<void(int32_t)>& callback) {
  if (!private_)
    return false;

  if (display_rect_cb_ && callback) {
    // TODO(a.bujalski)
    // Update Host API to return PPAPI error code directly, instead of such
    // hack, to return true here and then schedule callback call with error.
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE,
        base::Bind(callback, static_cast<int32_t>(PP_ERROR_INPROGRESS)));
    return true;
  }

  if (!plugin_rect_initialized_ && callback) {
    display_rect_cb_ = callback;
    return true;
  }

  // visible_video_rect is computed relatively to plugin_view_.rect
  PP_Rect visible_video_rect;
  if (video_position_.size.height > 0 && video_position_.size.width > 0)
    visible_video_rect = IntRectIntersection(clip_rect_, video_position_);
  else
    visible_video_rect = clip_rect_;

  // we have something to show, so calculate relative rect
  if (visible_video_rect.size.width > 0 && visible_video_rect.size.height > 0) {
    // relative to the page
    visible_video_rect.point.x =
        plugin_rect_.point.x + visible_video_rect.point.x;
    visible_video_rect.point.y =
        plugin_rect_.point.y + visible_video_rect.point.y;

    // relative to the screen
    auto viewport_rect = GetViewportRect();
    visible_video_rect.point.x =
        viewport_rect.point.x + visible_video_rect.point.x;
    visible_video_rect.point.y =
        viewport_rect.point.y + visible_video_rect.point.y;
  }
  PP_FloatRect crop_rect;
  if ((visible_video_rect.size.width == video_position_.size.width &&
       visible_video_rect.size.height == video_position_.size.height) ||
      (visible_video_rect.size.width == 0 &&
       visible_video_rect.size.height == 0)) {
    crop_rect = PP_MakeFloatRectFromXYWH(0.0f, 0.0f, 1.0f, 1.0f);
  } else {
    auto crop_width = VisibleCropRatio(video_position_.size.width,
                                       visible_video_rect.size.width);
    auto crop_height = VisibleCropRatio(video_position_.size.height,
                                        visible_video_rect.size.height);
    crop_rect = PP_MakeFloatRectFromXYWH(1.0f - crop_width, 1.0f - crop_height,
                                         crop_width, crop_height);
  }

  DCHECK(!display_rect_cb_ || !callback);
  base::Callback<void(int32_t)> cb{std::move(display_rect_cb_)};
  display_rect_cb_.Reset();
  if (!cb)
    cb = callback;
  // Need to set not null callback if both |display_rect_cb_| and |callback|
  // are null.
  if (!cb)
    cb = base::Bind([](int32_t) {});

  private_->SetDisplayRect(visible_video_rect, crop_rect, cb);
  return true;
}

PP_Rect PepperMediaPlayerHost::GetViewportRect() {
  int render_process_id, render_frame_id;
  bool res = browser_ppapi_host_->GetRenderFrameIDsForInstance(
      pp_instance(), &render_process_id, &render_frame_id);
  if (!res) {
    LOG(WARNING) << "Failed to get render frame host";
    return PP_Rect();
  }
  auto frame_host = RenderFrameHost::FromID(render_process_id, render_frame_id);
  auto web_contents = WebContents::FromRenderFrameHost(frame_host);
  if (!web_contents) {
    LOG(WARNING) << "Failed to get web content";
    return PP_Rect();
  }
  auto widget_host_view = web_contents->GetRenderWidgetHostView();
  if (!widget_host_view) {
    LOG(WARNING) << "Failed to get widget host view";
    return PP_Rect();
  }
  auto rect = widget_host_view->GetViewBounds();
  return PP_MakeRectFromXYWH(rect.x(), rect.y(), rect.width(), rect.height());
}

void PepperMediaPlayerHost::OnSubtitlesDownloaded(
    ppapi::host::ReplyMessageContext reply_context,
    const std::string& encoding,
    int32_t download_result,
    const base::FilePath& path) {
  downloader_.reset();
  if (download_result != PP_OK) {
    reply_context.params.set_result(download_result);
    host()->SendReply(reply_context,
                      PpapiPluginMsg_MediaPlayer_AddExternalSubtitlesReply(
                          PP_TextTrackInfo()));

    return;
  }

  private_->AddExternalSubtitles(
      path.value(), encoding,
      base::Bind(&ReplyCallbackWithRefOutput<
                     PpapiPluginMsg_MediaPlayer_AddExternalSubtitlesReply,
                     PP_TextTrackInfo>,
                 pp_instance(), reply_context));
}

base::ScopedTempDir* PepperMediaPlayerHost::GetTempDir() {
  if (!temp_dir_) {
    temp_dir_.reset(new base::ScopedTempDir);
    if (!temp_dir_->CreateUniqueTempDir()) {
      LOG(WARNING) << "Failed to create temporary directory";
      temp_dir_.reset();
    }
  }

  return temp_dir_.get();
}

std::unique_ptr<PepperMediaPlayerHost::SubtitlesDownloader>
PepperMediaPlayerHost::SubtitlesDownloader::Create(
    const GURL& url,
    const base::Callback<base::ScopedTempDir*(void)>& temp_dir_callback,
    const base::Callback<void(int32_t, const base::FilePath&)>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (url.SchemeIsFile()) {
    return std::unique_ptr<PepperMediaPlayerHost::SubtitlesDownloader>{
        new FileSubtitlesDownloader(url, callback)};
  } else {
    return std::unique_ptr<PepperMediaPlayerHost::SubtitlesDownloader>{
        new UrlSubtitlesDownloader(url, temp_dir_callback, callback)};
  }
}

PepperMediaPlayerHost::SubtitlesDownloader::SubtitlesDownloader(
    const base::Callback<void(int32_t, const base::FilePath&)>& callback)
    : callback_(callback) {}

PepperMediaPlayerHost::SubtitlesDownloader::~SubtitlesDownloader() = default;

void PepperMediaPlayerHost::SubtitlesDownloader::PostDownloadCompleted(
    int32_t result,
    const base::FilePath& file_path) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&PepperMediaPlayerHost::SubtitlesDownloader::DownloadCompleted,
                 callback_, result, file_path));
}

// static
void PepperMediaPlayerHost::SubtitlesDownloader::DownloadCompleted(
    const base::Callback<void(int32_t, const base::FilePath&)>& callback,
    int32_t result,
    const base::FilePath& file_path) {
  callback.Run(result, file_path);
}

void PepperMediaPlayerHost::UpdatePluginView(const PP_Rect& plugin_rect,
                                             const PP_Rect& clip_rect) {
  plugin_rect_ = plugin_rect;
  if (clip_rect.size.width > 0 && clip_rect.size.height > 0)
    clip_rect_ = clip_rect;

  plugin_rect_initialized_ = true;
  UpdateVideoPosition(base::Callback<void(int32_t)>());
}

}  // namespace content
