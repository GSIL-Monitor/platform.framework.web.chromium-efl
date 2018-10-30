// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_ELEMENTARY_STREAM_PRIVATE_TIZEN_H_
#define CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_ELEMENTARY_STREAM_PRIVATE_TIZEN_H_

#include <cstdint>
#include <vector>

#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "content/browser/renderer_host/pepper/media_player/pepper_es_data_source_private.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/drm_stream_configuration.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_player_dispatcher.h"
#include "content/browser/renderer_host/pepper/media_player/tizen/platform_context_tizen.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/shared_impl/media_player/es_packet.h"

namespace content {

class PepperESDataSourcePrivate;

// This class provides common functionality for ES streams of various types.
// StreamTraits must provide following traits for the given ElementaryStreamT:
// * CodecConfig - ppapi codec configuration type
// * StreamInfo - platform player stream_info structure type
// * SetStreamInfo() - function that calls player and sets StreamInfo
template <typename ElementaryStreamT, typename StreamTraits>
class PepperElementaryStreamPrivateTizen : public ElementaryStreamT {
 public:
  typedef typename StreamTraits::CodecConfig CodecConfig;
  typedef typename StreamTraits::StreamInfo StreamInfo;

  // Called on player thread
  int32_t SourceAttached(PepperPlayerAdapterInterface* player);

  // Called on IO thread
  void SourceDetached(PepperPlayerAdapterInterface* player);
  // Called on player thread
  int32_t SetEndOfStream();

  void ClearOwner() { owner_ = nullptr; }

  // Reset any DRM session information associated with this stream.
  void ClearDrmSession() { drm_session_ = {}; }

  void AppendPacket(std::unique_ptr<ppapi::ESPacket> packet,
                    const base::Callback<void(int32_t)>& callback) override;

  void AppendEncryptedPacket(
      std::unique_ptr<ppapi::EncryptedESPacket> packet,
      const base::Callback<void(int32_t)>& callback) override;

  void AppendTrustZonePacket(
      std::unique_ptr<ppapi::ESPacket> packet,
      const PP_TrustZoneReference& handle,
      const base::Callback<void(int32_t)>& callback) override;

  void Flush(const base::Callback<void(int32_t)>& callback) override;

  void SetDRMInitData(const std::vector<uint8_t>& type,
                      const std::vector<uint8_t>& init_data,
                      const base::Callback<void(int32_t)>& callback) override;

  void InitializeDone(PP_StreamInitializationMode mode,
                      const CodecConfig& config,
                      const base::Callback<void(int32_t)>& callback) override;

  void UpdateStreamConfigDrmType(StreamInfo* stream_config);

  int32_t ConfigureDrm();

  int32_t InitializeDoneOnPlayerThread(PepperPlayerAdapterInterface* player);

  void SetListener(
      base::WeakPtr<ElementaryStreamListenerPrivate> listener) override;

  void RemoveListener(ElementaryStreamListenerPrivate* listener) override;

  bool IsValid() { return codec_config_.IsValid(); }

 protected:
  explicit PepperElementaryStreamPrivateTizen(PepperESDataSourcePrivate* owner)
      : drm_configuration_{DRMStreamConfiguration::Create()},
        owner_{owner},
        listener_{nullptr},
        mode_{PP_STREAMINITIALIZATIONMODE_FULL} {}

  ~PepperElementaryStreamPrivateTizen() override = default;

  int32_t AppendPacketOnAppendThread(ppapi::ESPacket* packet,
                                     PepperPlayerAdapterInterface* player);

  int32_t AppendEncryptedPacketOnAppendThread(
      ppapi::EncryptedESPacket* packet,
      PepperPlayerAdapterInterface* player);

  int32_t AppendTrustZonePacketOnAppendThread(
      ppapi::ESPacket* packet,
      const PP_TrustZoneReference& handle,
      PepperPlayerAdapterInterface* player);

  int32_t FlushOnPlayerThread(PepperPlayerAdapterInterface* player) {
    LOG(ERROR) << "Not supported";
    return PP_ERROR_NOTSUPPORTED;
  }

  PepperTizenPlayerDispatcher* dispatcher() {
    return owner_ ? owner_->GetContext().dispatcher() : nullptr;
  }

  PepperTizenDRMManager* drm_manager() {
    return owner_ ? owner_->GetContext().drm_manager() : nullptr;
  }

  DRMStreamConfiguration* drm_configuration() {
    return drm_configuration_.get();
  }

  PepperTizenDRMSession* drm_session() { return drm_session_.get(); }

  bool InitializeDrmSession();

 private:
  std::unique_ptr<DRMStreamConfiguration> drm_configuration_;

  scoped_refptr<PepperTizenDRMSession> drm_session_;

  // non-owning pointer
  PepperESDataSourcePrivate* owner_;

  base::WeakPtr<ElementaryStreamListenerPrivate> listener_;

  // Configuration cache, used to store configuration before an attach.
  CodecConfig codec_config_;

  PP_StreamInitializationMode mode_;

  // Platform player configuration cache.
  StreamInfo player_stream_info_;
};  // class PepperElementaryStreamPrivateTizen

template <typename ElementaryStreamT, typename StreamTraits>
int32_t PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    SourceAttached(PepperPlayerAdapterInterface* player) {
  DCHECK(player != nullptr);

  int32_t pp_error = InitializeDoneOnPlayerThread(player);
  if (pp_error != PP_OK)
    return pp_error;

  if (listener_)
    player->SetListener(StreamTraits::Type, listener_);

  return PP_OK;
}

template <typename ElementaryStreamT, typename StreamTraits>
void PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    SourceDetached(PepperPlayerAdapterInterface* player) {
  DCHECK(player != nullptr);

  if (listener_)
    player->RemoveListener(StreamTraits::Type, listener_.get());
}

template <typename ElementaryStreamT, typename StreamTraits>
int32_t PepperElementaryStreamPrivateTizen<ElementaryStreamT,
                                           StreamTraits>::SetEndOfStream() {
  if (!dispatcher() || !dispatcher()->player()) {
    LOG(ERROR) << "Pepper Media Player is not initialized";
    return PP_ERROR_FAILED;
  }

  return dispatcher()->player()->SubmitEOSPacket(StreamTraits::Type);
}

template <typename ElementaryStreamT, typename StreamTraits>
void PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    AppendPacket(std::unique_ptr<ppapi::ESPacket> packet,
                 const base::Callback<void(int32_t)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperationOnAppendThread(
      dispatcher(), FROM_HERE,
      base::Bind(
          &PepperElementaryStreamPrivateTizen::AppendPacketOnAppendThread, this,
          base::Owned(packet.release())),
      callback);
}

template <typename ElementaryStreamT, typename StreamTraits>
void PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    AppendEncryptedPacket(std::unique_ptr<ppapi::EncryptedESPacket> packet,
                          const base::Callback<void(int32_t)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperationOnAppendThread(
      dispatcher(), FROM_HERE,
      base::Bind(&PepperElementaryStreamPrivateTizen::
                     AppendEncryptedPacketOnAppendThread,
                 this, base::Owned(packet.release())),
      callback);
}

template <typename ElementaryStreamT, typename StreamTraits>
void PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    AppendTrustZonePacket(std::unique_ptr<ppapi::ESPacket> packet,
                          const PP_TrustZoneReference& handle,
                          const base::Callback<void(int32_t)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperationOnAppendThread(
      dispatcher(), FROM_HERE,
      base::Bind(&PepperElementaryStreamPrivateTizen::
                     AppendTrustZonePacketOnAppendThread,
                 this, base::Owned(packet.release()), handle),
      callback);
}

template <typename ElementaryStreamT, typename StreamTraits>
void PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::Flush(
    const base::Callback<void(int32_t)>& callback) {
  PepperTizenPlayerDispatcher::DispatchOperation(
      dispatcher(), FROM_HERE,
      base::Bind(&PepperElementaryStreamPrivateTizen::FlushOnPlayerThread,
                 this),
      callback);
}

template <typename ElementaryStreamT, typename StreamTraits>
void PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    SetDRMInitData(const std::vector<uint8_t>& type,
                   const std::vector<uint8_t>& init_data,
                   const base::Callback<void(int32_t)>& callback) {
  base::MessageLoop::current()->task_runner()->PostTask(
      FROM_HERE, base::Bind(callback, drm_configuration()->ParseDRMInitData(
                                          type, init_data)));
}

template <typename ElementaryStreamT, typename StreamTraits>
void PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    InitializeDone(PP_StreamInitializationMode mode,
                   const CodecConfig& config,
                   const base::Callback<void(int32_t)>& callback) {
  mode_ = mode;
  codec_config_ = config;
  // This source hasn't been attached yet, so only store configuration for
  // the time being. Configuration will be continued by ES Data Source upon
  // attach.
  if (!dispatcher()) {
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(PP_OK)));
    return;
  }

  int32_t pp_error = ConfigureDrm();
  if (pp_error == PP_OK) {
    PepperTizenPlayerDispatcher::DispatchOperation(
        dispatcher(), FROM_HERE,
        base::Bind(
            &PepperElementaryStreamPrivateTizen<
                ElementaryStreamT, StreamTraits>::InitializeDoneOnPlayerThread,
            this),
        callback);
  } else {
    ClearDrmSession();
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(callback, static_cast<int32_t>(pp_error)));
  }
}

template <typename ElementaryStreamT, typename StreamTraits>
void PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    UpdateStreamConfigDrmType(StreamInfo* stream_config) {
  stream_config->drm_type = drm_configuration()->drm_type();
}

template <typename ElementaryStreamT, typename StreamTraits>
int32_t PepperElementaryStreamPrivateTizen<ElementaryStreamT,
                                           StreamTraits>::ConfigureDrm() {
  StreamInfo player_stream_info;
  player_stream_info.config = codec_config_;
  player_stream_info.initialization_mode = mode_;

  UpdateStreamConfigDrmType(&player_stream_info);
  if (player_stream_info.drm_type != PP_MEDIAPLAYERDRMTYPE_UNKNOWN &&
      !InitializeDrmSession()) {
    LOG(ERROR) << "Cannot initialize DRM session.";
    return PP_ERROR_FAILED;
  } else if (player_stream_info.drm_type == PP_MEDIAPLAYERDRMTYPE_UNKNOWN &&
             drm_session()) {
    ClearDrmSession();
  }

  player_stream_info_ = player_stream_info;
  return PP_OK;
}

template <typename ElementaryStreamT, typename StreamTraits>
int32_t PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    InitializeDoneOnPlayerThread(PepperPlayerAdapterInterface* player) {
  return StreamTraits::SetStreamInfo(player, player_stream_info_);
}

template <typename ElementaryStreamT, typename StreamTraits>
void PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    SetListener(base::WeakPtr<ElementaryStreamListenerPrivate> listener) {
  if (dispatcher() && dispatcher()->player())
    dispatcher()->player()->SetListener(StreamTraits::Type, listener);

  listener_ = listener;
}

template <typename ElementaryStreamT, typename StreamTraits>
void PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    RemoveListener(ElementaryStreamListenerPrivate* listener) {
  if (dispatcher() && dispatcher()->player())
    dispatcher()->player()->RemoveListener(StreamTraits::Type, listener);

  listener_ = nullptr;
}

template <typename ElementaryStreamT, typename StreamTraits>
int32_t PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    AppendPacketOnAppendThread(ppapi::ESPacket* packet,
                               PepperPlayerAdapterInterface* player) {
  return player->SubmitPacket(packet, StreamTraits::Type);
}

template <typename ElementaryStreamT, typename StreamTraits>
int32_t PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    AppendEncryptedPacketOnAppendThread(ppapi::EncryptedESPacket* packet,
                                        PepperPlayerAdapterInterface* player) {
  if (!drm_session_) {
    LOG(ERROR) << "DRM session is not set";
    return PP_ERROR_FAILED;
  }

  auto managed_handle = drm_session_->Decrypt(*packet);
  if (!managed_handle) {
    LOG(ERROR) << "Packet decryption has failed";
    return PP_ERROR_FAILED;
  }

  return player->SubmitEncryptedPacket(packet, std::move(managed_handle),
                                       StreamTraits::Type);
}

template <typename ElementaryStreamT, typename StreamTraits>
int32_t PepperElementaryStreamPrivateTizen<ElementaryStreamT, StreamTraits>::
    AppendTrustZonePacketOnAppendThread(ppapi::ESPacket* packet,
                                        const PP_TrustZoneReference& handle,
                                        PepperPlayerAdapterInterface* player) {
  // Need to use malloc to be compilant with handles returned by iEME
  handle_and_size_s* handle_and_size =
      static_cast<handle_and_size_s*>(malloc(sizeof(handle_and_size_s)));
  if (!handle_and_size)
    return PP_ERROR_NOMEMORY;

  handle_and_size->handle = handle.handle;
  handle_and_size->size = handle.size;
  handle_and_size->tz_enabled = TZ_ENABLED_YES;
  auto managed_handle = AdoptHandle(handle_and_size);

  return player->SubmitEncryptedPacket(packet, std::move(managed_handle),
                                       StreamTraits::Type);
}

template <typename ElementaryStreamT, typename StreamTraits>
bool PepperElementaryStreamPrivateTizen<ElementaryStreamT,
                                        StreamTraits>::InitializeDrmSession() {
  DCHECK(drm_manager());
  auto pp_drm_type = drm_configuration_->drm_type();
  if (pp_drm_type == PP_MEDIAPLAYERDRMTYPE_UNKNOWN) {
    LOG(WARNING) << "Unknown DRM type or no DRM.";
    return false;
  }
  drm_session_ = drm_manager()->GetSession(
      pp_drm_type, drm_configuration()->drm_specific_data());
  return !!drm_session_;
}

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_PEPPER_MEDIA_PLAYER_TIZEN_PEPPER_ELEMENTARY_STREAM_PRIVATE_TIZEN_H_
