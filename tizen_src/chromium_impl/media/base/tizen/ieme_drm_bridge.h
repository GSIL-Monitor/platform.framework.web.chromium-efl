// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_ANDROID_IEME_DRM_BRIDGE_H_
#define MEDIA_BASE_ANDROID_IEME_DRM_BRIDGE_H_

#include <emeCDM/IEME.h>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/hash_tables.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "media/base/cdm_context.h"
#include "media/base/cdm_key_information.h"
#include "media/base/cdm_promise_adapter.h"
#include "media/base/content_decryption_module.h"
#include "media/base/decryptor.h"
#include "media/base/media_export.h"
#include "media/cdm/player_tracker_impl.h"

class GURL;

namespace media {

class IEMEDrmBridge;
class MediaPlayerManager;

using ScopedIEMEDrmBridgePtr = std::unique_ptr<IEMEDrmBridge>;

// This class provides DRM services for tizen capi EME implementation.
class MEDIA_EXPORT IEMEDrmBridge : public ContentDecryptionModule,
                                   public CdmContext,
                                   public Decryptor,
                                   public eme::IEventListener {
 public:
  ~IEMEDrmBridge() override;

  // Checks whether |key_system| is supported.
  static bool IsKeySystemSupported(const std::string& key_system);

  // Returns a MediaDrmBridge instance if |key_system| is supported, or a NULL
  // pointer otherwise.
  // TODO(xhwang): Is it okay not to update session expiration info?
  static ScopedIEMEDrmBridgePtr Create(
      const std::string& key_system,
      const SessionMessageCB& session_message_cb,
      const SessionClosedCB& session_closed_cb,
      const SessionKeysChangeCB& session_keys_change_cb,
      const SessionExpirationUpdateCB& session_expiration_update_cb);

  // Returns a MediaDrmBridge instance if |key_system| is supported, or a NULL
  // otherwise. No session callbacks are provided. This is used when we need to
  // use MediaDrmBridge without creating any sessions.
  static ScopedIEMEDrmBridgePtr CreateWithoutSessionSupport(
      const std::string& key_system);

  // MediaKeys (via BrowserCdm) implementation.
  void SetServerCertificate(
      const std::vector<uint8_t>& certificate,
      std::unique_ptr<media::SimpleCdmPromise> promise) override;
  void CreateSessionAndGenerateRequest(
      CdmSessionType session_type,
      media::EmeInitDataType init_data_type,
      const std::vector<uint8_t>& init_data,
      std::unique_ptr<media::NewSessionCdmPromise> promise) override;
  void LoadSession(
      CdmSessionType session_type,
      const std::string& session_id,
      std::unique_ptr<media::NewSessionCdmPromise> promise) override;
  void UpdateSession(const std::string& session_id,
                     const std::vector<uint8_t>& response,
                     std::unique_ptr<media::SimpleCdmPromise> promise) override;
  void CloseSession(const std::string& session_id,
                    std::unique_ptr<media::SimpleCdmPromise> promise) override;
  void RemoveSession(const std::string& session_id,
                     std::unique_ptr<media::SimpleCdmPromise> promise) override;
  CdmContext* GetCdmContext() override;

  // Get handle for this IEME instance
  eme::eme_decryptor_t GetDecryptorHandle() override;

  // Return key system which it was configured to
  const std::string& GetKeySystem() const;

  // CdmContext interface
  Decryptor* GetDecryptor() override;

  // TODO(m.debski): We could make it return the same type as
  // IEME::getDecryptor() and use that from Browser side MediaPlayer
  // to create drm_info.
  int GetCdmId() const override;

  // TODO(m.debski):
  // Decryptor interface
  void RegisterNewKeyCB(StreamType stream_type,
                        const NewKeyCB& new_key_cb) override;
  void Decrypt(StreamType stream_type,
               const scoped_refptr<DecoderBuffer>& encrypted,
               const DecryptCB& decrypt_cb) override;
  void CancelDecrypt(StreamType stream_type) override;
  // We problably won't use that
  void InitializeAudioDecoder(const AudioDecoderConfig& config,
                              const DecoderInitCB& init_cb) override;
  void InitializeVideoDecoder(const VideoDecoderConfig& config,
                              const DecoderInitCB& init_cb) override;
  void DecryptAndDecodeAudio(const scoped_refptr<DecoderBuffer>& encrypted,
                             const AudioDecodeCB& audio_decode_cb) override;
  void DecryptAndDecodeVideo(const scoped_refptr<DecoderBuffer>& encrypted,
                             const VideoDecodeCB& video_decode_cb) override;
  void ResetDecoder(StreamType stream_type) override;
  void DeinitializeDecoder(StreamType stream_type) override;

  // eme::IEventListener implementation.
  void onMessage(const std::string& session_id,
                 eme::MessageType message_type,
                 const std::string& message) override;
  void onKeyStatusesChange(const std::string& session_id) override;
  void onRemoveComplete(const std::string& session_id) override;

 private:
  IEMEDrmBridge(const std::string& key_system,
                const SessionMessageCB& session_message_cb,
                const SessionClosedCB& session_closed_cb,
                const SessionKeysChangeCB& session_keys_change_cb,
                const SessionExpirationUpdateCB& session_expiration_update_cb);

  bool VerifyStandaloneEmePreconditions(CdmPromise& promise,
                                        const char* method_name) const;
  void CreateIEME();
  void ResolvePromise(uint32_t promise_id);
  void ChangeKeyStatuses(const std::string& session_id);
  // Updates key statuses map based on given map. Notifies about new key and
  // calls session_keys_change_cb
  void UpdateKeyStatuses(const std::string& session_id,
                         const eme::KeyStatusMap&);
  // Checks session expiration time and if changed calls
  // NotifySessionExpirationUpdate.
  void UpdateExpirationTime(const std::string& session_id);

  void NotifyNewKey();
  void NotifySessionExpirationUpdate(const std::string& session_id,
                                     int64_t expiry_time_ms);
  // DRM API requires us to call destroy instead of the destructor
  struct IEMEDelete {
    inline void operator()(eme::IEME* cdm) const { eme::IEME::destroy(cdm); }
  };

  std::unique_ptr<eme::IEME, IEMEDelete> cdm_;

  // Key system name which was used to create |cdm_|
  const std::string key_system_;

  // Callbacks for firing session events.
  SessionMessageCB session_message_cb_;
  SessionClosedCB session_closed_cb_;
  SessionKeysChangeCB session_keys_change_cb_;
  SessionExpirationUpdateCB session_expiration_update_cb_;

  // For resolving remove session promise
  typedef base::hash_map<std::string, uint32_t> SessionPromiseMap;
  SessionPromiseMap session_promise_map_;
  std::unique_ptr<CdmPromiseAdapter> cdm_promise_adapter_;

  // This map is for keeping track of keys and detecting whether new key has
  // been added
  typedef base::hash_map<std::string, CdmKeyInformation::KeyStatus>
      KayStatusesMap;
  base::hash_map<std::string, KayStatusesMap> session_key_statuses_map_;

  // Expiration data after last check
  base::hash_map<std::string, int64_t> last_expiration_times_map_;

  // For DRM callbacks because they are not queuing their events on the event
  // loop
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  NewKeyCB new_audio_key_cb_;
  NewKeyCB new_video_key_cb_;

  base::WeakPtrFactory<IEMEDrmBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(IEMEDrmBridge);
};

ContentDecryptionModule* CreateIEMEDrmBridge(
    const std::string& key_system,
    const SessionMessageCB& session_message_cb,
    const SessionClosedCB& session_closed_cb,
    const SessionKeysChangeCB& session_keys_change_cb,
    const SessionExpirationUpdateCB& session_expiration_update_cb);

}  // namespace media

#endif  // MEDIA_BASE_ANDROID_IEME_DRM_BRIDGE_H_
