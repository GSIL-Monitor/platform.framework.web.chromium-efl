// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/tizen/ieme_drm_bridge.h"

#include <drmdecrypt_api.h>
#include <algorithm>
#include <limits>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/decoder_buffer.h"
#include "tizen_src/ewk/efl_integration/common/application_type.h"
#include "url/gurl.h"

namespace media {

namespace {

static const base::TimeDelta kEmeDecryptRetryTime =
    base::TimeDelta::FromMilliseconds(1000);

// Helper class to invoke free in smart pointers
struct libc_free {
  template <class Raw>
  void operator()(Raw* ptr) {
    ::free(ptr);
  }
};

// Convert |session_type| to IEME SessionType used in session_create
eme::SessionType ConvertSessionType(CdmSessionType session_type) {
  switch (session_type) {
    case CdmSessionType::TEMPORARY_SESSION:
      return eme::kTemporary;
    case CdmSessionType::PERSISTENT_LICENSE_SESSION:
      return eme::kPersistentLicense;
    case CdmSessionType::PERSISTENT_RELEASE_MESSAGE_SESSION:
      return eme::kPersistentUsageRecord;
    default:
      LOG(ERROR) << "Not supported session type "
                 << static_cast<int>(session_type) << ".";
      return eme::kTemporary;
  }
}

// Convert |message_type| from IEME message callback
media::CdmMessageType ConvertMessageType(eme::MessageType message_type) {
  switch (message_type) {
    case eme::kLicenseRequest:
      return CdmMessageType::LICENSE_REQUEST;
    case eme::kLicenseRenewal:
      return CdmMessageType::LICENSE_RENEWAL;
    case eme::kLicenseRelease:
      return CdmMessageType::LICENSE_RELEASE;
    case eme::kIndividualizationRequest:
      return CdmMessageType::LICENSE_REQUEST;
    default:
      LOG(ERROR) << "Not supported message type " << message_type << ".";
      return CdmMessageType::LICENSE_REQUEST;
  }
}

// Convert |init_data_type| to IEME InitDataType used in session_generateRequest
eme::InitDataType ConvertInitDataType(media::EmeInitDataType init_data_type) {
  switch (init_data_type) {
    case media::EmeInitDataType::WEBM:
      return eme::kWebM;
    case media::EmeInitDataType::CENC:
      return eme::kCenc;
    case media::EmeInitDataType::KEYIDS:
      return eme::kKeyIds;
    default:
      LOG(ERROR)
          << "Not supported message init date type "
          << static_cast<std::underlying_type<media::EmeInitDataType>::type>(
                 init_data_type)
          << ".";
      // TODO(m.debski): Let's hope IEME checks this and returns reasonable
      // error, is it ok to assume that? We can reject promise ourselves.
      return std::numeric_limits<eme::InitDataType>::max();
  }
}

CdmKeyInformation::KeyStatus ConvertKeyStatus(eme::KeyStatus key_status) {
  switch (key_status) {
    case eme::kUsable:
      return CdmKeyInformation::USABLE;
    case eme::kExpired:
      return CdmKeyInformation::EXPIRED;
    case eme::kOutputRestricted:
      return CdmKeyInformation::OUTPUT_RESTRICTED;
    case eme::kStatusPending:
      // TODO(xhwang): This should probably be renamed to "PENDING".
      return CdmKeyInformation::KEY_STATUS_PENDING;
    case eme::kInternalError:
      return CdmKeyInformation::INTERNAL_ERROR;
    case eme::kReleased:
      return CdmKeyInformation::RELEASED;
    default:
      LOG(ERROR) << "Not supported message key status " << key_status << ".";
      return CdmKeyInformation::INTERNAL_ERROR;
  }
}

// It should be used only to convert erros. Do not use it
// for kSuccess status because CdmPromise::UNKNOWN_ERROR
// will be returned.
CdmPromise::Exception ConvertEmeError(eme::Status status) {
  DCHECK(status != eme::kSuccess);
  switch (status) {
    case eme::kInvalidAccess:
      return CdmPromise::Exception::TYPE_ERROR;
    case eme::kNotSupported:
      return CdmPromise::Exception::NOT_SUPPORTED_ERROR;
    case eme::kInvalidState:
      return CdmPromise::Exception::INVALID_STATE_ERROR;
    case eme::kQuotaExceeded:
      return CdmPromise::Exception::QUOTA_EXCEEDED_ERROR;
    default:
      return CdmPromise::Exception::TYPE_ERROR;
  }
}

std::string ConvertEmeErrorToString(eme::Status status) {
  DCHECK(status != eme::kSuccess);
  switch (status) {
    case eme::kInvalidAccess:
      return "Invalid access error";
    case eme::kNotSupported:
      return "Operation not supported";
    case eme::kInvalidState:
      return "Invalid state";
    case eme::kQuotaExceeded:
      return "Quota was exceed";
    default:
      return "";
  }
}

void RejectCdmPromise(CdmPromise& promise,
                      eme::Status error,
                      const std::string& message) {
  DCHECK(error != eme::kSuccess);
  std::stringstream ss;
  ss << message << ". " << ConvertEmeErrorToString(error) << ".";
  promise.reject(ConvertEmeError(error), error, ss.str());
}

std::string CreateStringFromBinaryData(const std::vector<uint8_t>& data) {
  // This way string can contain null characters
  return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

std::vector<uint8_t> CreateBinaryDataFromString(const std::string& data) {
  const auto data_ptr = reinterpret_cast<const uint8_t*>(data.data());
  const auto data_size = sizeof(*data.data()) * data.size();
  CHECK(data_size >= 0);
  return std::vector<uint8_t>(data_ptr, data_ptr + data_size);
}

}  // namespace

// static
bool IEMEDrmBridge::IsKeySystemSupported(const std::string& key_system) {
  CHECK(!key_system.empty());
  return eme::IEME::isKeySystemSupported(key_system) == eme::kSupported;
}

IEMEDrmBridge::IEMEDrmBridge(
    const std::string& key_system,
    const SessionMessageCB& session_message_cb,
    const SessionClosedCB& session_closed_cb,
    const SessionKeysChangeCB& session_keys_change_cb,
    const SessionExpirationUpdateCB& session_expiration_update_cb)
    : key_system_(key_system),
      session_message_cb_(session_message_cb),
      session_closed_cb_(session_closed_cb),
      session_keys_change_cb_(session_keys_change_cb),
      session_expiration_update_cb_(session_expiration_update_cb),
      cdm_promise_adapter_(new CdmPromiseAdapter()),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {
  LOG(INFO) << "Constructing DRM bridge " << this << " for key system "
            << key_system;
}

IEMEDrmBridge::~IEMEDrmBridge() {
  LOG(INFO) << "Destroying IEMEDrmBridge";

  // Destroy our CDM so no callbacks are called from this point onwards.
  cdm_.reset();
  cdm_promise_adapter_.reset();
}

// static
ScopedIEMEDrmBridgePtr IEMEDrmBridge::Create(
    const std::string& key_system,
    const SessionMessageCB& session_message_cb,
    const SessionClosedCB& session_closed_cb,
    const SessionKeysChangeCB& session_keys_change_cb,
    const SessionExpirationUpdateCB& session_expiration_update_cb) {
  LOG(INFO) << "Creating DRM bridge for key system: " << key_system;
  ScopedIEMEDrmBridgePtr ieme_drm_bridge;

  if (IsKeySystemSupported(key_system)) {
    ieme_drm_bridge.reset(new IEMEDrmBridge(
        key_system, session_message_cb, session_closed_cb,
        session_keys_change_cb, session_expiration_update_cb));
  }

  return ieme_drm_bridge;
}

// static
ScopedIEMEDrmBridgePtr IEMEDrmBridge::CreateWithoutSessionSupport(
    const std::string& key_system) {
  return IEMEDrmBridge::Create(key_system, SessionMessageCB(),
                               SessionClosedCB(), SessionKeysChangeCB(),
                               SessionExpirationUpdateCB());
}

bool IEMEDrmBridge::VerifyStandaloneEmePreconditions(
    CdmPromise& promise,
    const char* method_name) const {
  if (GetPlayerType() == MEDIA_PLAYER_TYPE_NONE) {
    std::string error_str("Unable to call ");
    error_str += method_name;
    error_str +=
        " until MediaKeys object is associated with a media element and the "
        "media element has source set";
    LOG(WARNING) << error_str;
    promise.reject(CdmPromise::Exception::INVALID_STATE_ERROR, 0, error_str);
    return false;
  } else
    return true;
}

void IEMEDrmBridge::CreateIEME() {
  // Up to now IEME instance was created in IEMEDrmBridge's constructor.
  // This is now delayed to additionally support EME + DASH scenario in HbbTV.
  // Previously IEMEDrmBridge was used only for EME + MSE case. It requires
  // IEME instanced in the default model (local) as the decryption calls are
  // invoked in Chromium.
  // EME + DASH requires IEME in the remote model as the decryption is performed
  // in CAPI Player process.
  // However HbbTV still can use EME + MSE. Delaying IEME creation gives more
  // wiggle room to detect which EME use case HbbTV application attempts.
  // This is currently based on the source type the related media element was
  // set with. If MediaSource was set we need default model. If URL was set we
  // need remote model.
  // Without the delay we would need to know the source type when MediaKeys
  // is created (impossible as to know the source type MediaKeys needs to
  // be already assigned to media element). With the delay we require it much
  // later - during MediaKeySession's generateRequest() call.

  if (cdm_) {
    return;
  }

  // remote model for standalone EME in HbbTV
  bool is_url_player =
      (GetPlayerType() == MEDIA_PLAYER_TYPE_URL) ||
      (GetPlayerType() == MEDIA_PLAYER_TYPE_URL_WITH_VIDEO_HOLE);

  auto ieme_model = (content::IsHbbTV() && is_url_player)
                        ? eme::CDM_MODEL::E_CDM_MODEL_REMOTE
                        : eme::CDM_MODEL::E_CDM_MODEL_DEFAULT;

  LOG(INFO) << "Creating IEME, key system: " << key_system_
            << ", player type: " << GetPlayerType()
            << ", IEME model: " << ieme_model;

  cdm_.reset(eme::IEME::create(this, key_system_, false, ieme_model));
  CHECK(cdm_.get());
}

void IEMEDrmBridge::SetServerCertificate(
    const std::vector<uint8_t>& certificate,
    std::unique_ptr<media::SimpleCdmPromise> promise) {
  LOG(INFO) << "Setting server certificate";
  CHECK(!certificate.empty());

  if (content::IsHbbTV() && !VerifyStandaloneEmePreconditions(
                                *promise.get(), "setServerCeritficate")) {
    return;
  }

  CreateIEME();

  const auto result_status =
      cdm_->setServerCertificate(CreateStringFromBinaryData(certificate));
  if (eme::kSuccess == result_status) {
    promise->resolve();
  } else {
    LOG(ERROR) << "Setting server certificate failed with status: "
               << result_status;
    promise->reject(CdmPromise::Exception::TYPE_ERROR, 0,
                    "Set server certificate failed.");
  }
}

void IEMEDrmBridge::CreateSessionAndGenerateRequest(
    CdmSessionType session_type,
    media::EmeInitDataType init_data_type,
    const std::vector<uint8_t>& init_data,
    std::unique_ptr<media::NewSessionCdmPromise> promise) {
  LOG(INFO) << "Creating session session_type: "
            << static_cast<int>(session_type) << " init_data_type: "
            << static_cast<std::underlying_type<media::EmeInitDataType>::type>(
                   init_data_type);

  if (content::IsHbbTV() &&
      !VerifyStandaloneEmePreconditions(*promise.get(), "generateRequest")) {
    return;
  }

  CreateIEME();

  std::string session_id;
  const auto create_result_status =
      cdm_->session_create(ConvertSessionType(session_type), &session_id);
  if (eme::kSuccess != create_result_status) {
    LOG(ERROR) << "Creating session failed with status: "
               << create_result_status;
    RejectCdmPromise(*promise, create_result_status, "Session creation failed");
    return;
  } else if (session_id.empty()) {
    LOG(ERROR) << "Session empty";
    promise->reject(CdmPromise::Exception::TYPE_ERROR, create_result_status,
                    "Returned session Id is empty.");
    return;
  }

  LOG(INFO) << "success!" << session_id;
  const auto generate_result_status = cdm_->session_generateRequest(
      session_id, ConvertInitDataType(init_data_type),
      CreateStringFromBinaryData(init_data));
  if (eme::kSuccess != generate_result_status) {
    LOG(ERROR) << "IEME::session_generateRequest failed with status: "
               << generate_result_status;
    RejectCdmPromise(*promise, generate_result_status,
                     "Request generation failed");
    return;
  }

  LOG(INFO) << "request generated!" << session_id;
  promise->resolve(session_id);
}

void IEMEDrmBridge::LoadSession(
    CdmSessionType session_type,
    const std::string& session_id,
    std::unique_ptr<media::NewSessionCdmPromise> promise) {
  LOG(INFO) << "Loading session id: " << session_id;

  CHECK(session_type != CdmSessionType::TEMPORARY_SESSION);

  if (content::IsHbbTV() &&
      !VerifyStandaloneEmePreconditions(*promise.get(), "load")) {
    return;
  }
  CreateIEME();

  const auto load_result_status = cdm_->session_load(session_id);
  if (eme::kSuccess != load_result_status) {
    LOG(ERROR) << "Loading session failed with status: " << load_result_status;
    RejectCdmPromise(*promise, load_result_status, "Loading session failed");
    return;
  }

  promise->resolve(session_id);
  onKeyStatusesChange(session_id);
}

void IEMEDrmBridge::UpdateSession(
    const std::string& session_id,
    const std::vector<uint8_t>& response,
    std::unique_ptr<media::SimpleCdmPromise> promise) {
  LOG(INFO) << "Updating session id: " << session_id;

  const auto update_result_status =
      cdm_->session_update(session_id, CreateStringFromBinaryData(response));
  if (eme::kSuccess != update_result_status) {
    LOG(ERROR) << "Updating session failed with status: "
               << update_result_status;
    RejectCdmPromise(*promise, update_result_status, "Updating session failed");
    return;
  }

  promise->resolve();
  onKeyStatusesChange(session_id);
}

void IEMEDrmBridge::CloseSession(
    const std::string& session_id,
    std::unique_ptr<media::SimpleCdmPromise> promise) {
  LOG(INFO) << "Closing session id: " << session_id;

  const auto close_result_status = cdm_->session_close(session_id);
  if (eme::kSuccess != close_result_status) {
    LOG(ERROR) << "Closing failed with status: " << close_result_status;
    RejectCdmPromise(*promise, close_result_status, "Closing session failed.");
    return;
  }

  session_key_statuses_map_.erase(session_id);
  last_expiration_times_map_.erase(session_id);

  promise->resolve();
  task_runner_->PostTask(FROM_HERE, base::Bind(session_closed_cb_, session_id));
  onKeyStatusesChange(session_id);
}

void IEMEDrmBridge::RemoveSession(
    const std::string& session_id,
    std::unique_ptr<media::SimpleCdmPromise> promise) {
  LOG(INFO) << "Removing session id: " << session_id;

  const auto remove_result_status = cdm_->session_remove(session_id);
  if (eme::kSuccess != remove_result_status) {
    LOG(ERROR) << "Removing session id: " << session_id
               << " failed with status: " << remove_result_status;
    RejectCdmPromise(*promise, remove_result_status,
                     "Removing session failed.");
    return;
  }

  CHECK(session_promise_map_.find(session_id) == session_promise_map_.end());
  session_promise_map_[session_id] =
      cdm_promise_adapter_->SavePromise(std::move(promise));
  onKeyStatusesChange(session_id);
}

CdmContext* IEMEDrmBridge::GetCdmContext() {
  return this;
}

eme::eme_decryptor_t IEMEDrmBridge::GetDecryptorHandle() {
  return cdm_->getDecryptor("");
}

const std::string& IEMEDrmBridge::GetKeySystem() const {
  return key_system_;
}

Decryptor* IEMEDrmBridge::GetDecryptor() {
  return this;
}

int IEMEDrmBridge::GetCdmId() const {
  // we want to use Decryptor API
  return CdmContext::kInvalidCdmId;
}

void IEMEDrmBridge::RegisterNewKeyCB(StreamType stream_type,
                                     const NewKeyCB& new_key_cb) {
  switch (stream_type) {
    case kAudio:
      new_audio_key_cb_ = new_key_cb;
      break;
    case kVideo:
      new_video_key_cb_ = new_key_cb;
      break;
    default:
      LOG(ERROR) << "Unsupported stream type " << stream_type;
  }
}

void IEMEDrmBridge::Decrypt(StreamType stream_type,
                            const scoped_refptr<DecoderBuffer>& encrypted,
                            const DecryptCB& decrypt_cb) {
  auto decrypt_conf = encrypted->decrypt_config();
  scoped_refptr<DecoderBuffer> decrypted = encrypted;
  Decryptor::Status status = Decryptor::kError;

  if (!cdm_) {
    decrypt_cb.Run(Decryptor::kNoKey, decrypted);
    return;
  }

  if (decrypt_conf && decrypt_conf->iv().size() > 0) {
    std::vector<MSD_SUBSAMPLE_INFO> subsamples;
    subsamples.reserve(decrypt_conf->subsamples().size());
    for (const auto& subsample : decrypt_conf->subsamples()) {
      subsamples.emplace_back(
          MSD_SUBSAMPLE_INFO{subsample.clear_bytes, subsample.cypher_bytes});
    }

    MSD_FMP4_DATA subsamples_info{
        subsamples.size(), subsamples.data(),
    };

    // Encryption pattern support
    const media::EncryptionMode enc_mode = decrypt_conf->encryption_mode();
    const bool use_enc_pattern =
        static_cast<bool>(decrypt_conf->encryption_pattern());
    const unsigned int crypt_byte_block =
        use_enc_pattern ? decrypt_conf->encryption_pattern()->crypt_byte_block()
                        : 0u;
    const unsigned int skip_byte_block =
        use_enc_pattern ? decrypt_conf->encryption_pattern()->skip_byte_block()
                        : 0u;

    msd_cipher_param_s params{
        // below params were requested by MM to be hardcoded to those values
        // (see docs)
        enc_mode == media::EncryptionMode::kCbcs ? MSD_AES128_CBC
                                                 : MSD_AES128_CTR,  // algorithm
        MSD_FORMAT_FMP4,                                            // format
        MSD_PHASE_NONE,                                             // phase

        false,  // buseoutbuf "false when it must be protected with
                // TZ(TrustZone)"

        reinterpret_cast<unsigned char*>(
            const_cast<char*>(decrypt_conf->key_id().data())),  // pkid
        decrypt_conf->key_id().size(),                          // ukidlen

        reinterpret_cast<unsigned char*>(encrypted->writable_data()),  // pdata
        encrypted->data_size(),  // udatalen

        nullptr,  // poutbuf
        0,        // uoutbuflen

        reinterpret_cast<unsigned char*>(
            const_cast<char*>(decrypt_conf->iv().data())),  // piv
        decrypt_conf->iv().size(),                          // uivlen

        &subsamples_info,  // psubdata (I assume it is MSD_FMP4_DATA struct)
        nullptr,  // psplitoffsets "Nullable. If used, -1 terminated int array.
                  // (max offsets = 15)"

        use_enc_pattern,   // When using 'cbcs' pattern scheme, need to set '1'.
                           // If not, '0'
        crypt_byte_block,  // In case that busepattern is '1', count of the
                           // encrypted Blocks in the protection pattern
        skip_byte_block,   // In case that busepattern is '1', count of the
                           // unencrypted Blocks in the protection pattern
    };
    msd_cipher_param_s* params_ptr = &params;
    // This one was allocated with malloc so should be deallocated with free
    handle_and_size_s* decrypted_params_ptr = nullptr;

    auto ret = eme_decryptarray(GetDecryptorHandle(), &params_ptr, 1, nullptr,
                                0, &decrypted_params_ptr);
    std::unique_ptr<handle_and_size_s, libc_free> scoped_decrypted_params_ptr{
        decrypted_params_ptr};
    if (ret == E_SUCCESS) {
      decrypted = DecoderBuffer::CreateTzHandleBuffer(
          decrypted_params_ptr->handle, decrypted_params_ptr->size);
      decrypted->set_timestamp(encrypted->timestamp());
      decrypted->set_duration(encrypted->duration());
      status = Decryptor::kSuccess;
    } else if (ret == E_NEED_KEY) {
      LOG(WARNING) << "eme_decryptarray() failed no decryption key";
      status = Decryptor::kNoKey;
    } else if (ret == E_DECRYPT_BUFFER_FULL) {
      LOG(WARNING) << "eme_decryptarray() TZ out of memory";
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&IEMEDrmBridge::Decrypt, weak_factory_.GetWeakPtr(),
                     stream_type, encrypted, decrypt_cb),
          kEmeDecryptRetryTime);
      return;
    } else {
      LOG(ERROR) << "eme_decryptarray() failed with err code: " << ret;
      status = Decryptor::kError;
    }
  } else {
    status = Decryptor::kSuccess;
  }
  decrypt_cb.Run(status, decrypted);
}

void IEMEDrmBridge::CancelDecrypt(StreamType) {
  // Decrypt() calls the DecryptCB synchronously so there's nothing to cancel.
}

// We problably won't use that
void IEMEDrmBridge::InitializeAudioDecoder(const AudioDecoderConfig& config,
                                           const DecoderInitCB& init_cb) {
  // IEMEDrmBridge does not support audio decoding.
  init_cb.Run(false);
}

void IEMEDrmBridge::InitializeVideoDecoder(const VideoDecoderConfig& config,
                                           const DecoderInitCB& init_cb) {
  // IEMEDrmBridge does not support video decoding.
  init_cb.Run(false);
}

void IEMEDrmBridge::DecryptAndDecodeAudio(
    const scoped_refptr<DecoderBuffer>& encrypted,
    const AudioDecodeCB& audio_decode_cb) {
  NOTREACHED() << "IEMEDrmBridge does not support audio decoding";
}

void IEMEDrmBridge::DecryptAndDecodeVideo(
    const scoped_refptr<DecoderBuffer>& encrypted,
    const VideoDecodeCB& video_decode_cb) {
  NOTREACHED() << "IEMEDrmBridge does not support video decoding";
}

void IEMEDrmBridge::ResetDecoder(StreamType stream_type) {
  NOTREACHED() << "IEMEDrmBridge does not support audio/video decoding";
}

void IEMEDrmBridge::DeinitializeDecoder(StreamType stream_type) {
  // IEMEDrmBridge does not support audio/video decoding, but since this can be
  // called any time after InitializeAudioDecoder/InitializeVideoDecoder,
  // nothing to be done here.
}

void IEMEDrmBridge::IEMEDrmBridge::onMessage(const std::string& session_id,
                                             eme::MessageType message_type,
                                             const std::string& message) {
  LOG(INFO) << "session: " << session_id << " type: " << message_type
            << " message: " << message;
  if (eme::kLicenseAlreadyDone == message_type)
    return;

  task_runner_->PostTask(FROM_HERE,
                         base::Bind(session_message_cb_, session_id,
                                    ConvertMessageType(message_type),
                                    CreateBinaryDataFromString(message)));
}

void IEMEDrmBridge::onKeyStatusesChange(const std::string& session_id) {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&IEMEDrmBridge::ChangeKeyStatuses,
                                    weak_factory_.GetWeakPtr(), session_id));
}

void IEMEDrmBridge::ChangeKeyStatuses(const std::string& session_id) {
  LOG(INFO) << "session: " << session_id;
  eme::KeyStatusMap key_statuses;
  const auto key_status_result_status =
      cdm_->session_getKeyStatuses(session_id, &key_statuses);
  if (key_status_result_status != eme::kSuccess) {
    LOG(ERROR) << "Could not retrieve key statuses for session id: "
               << session_id << " status: " << key_status_result_status;
    return;
  }
  UpdateKeyStatuses(session_id, key_statuses);
  UpdateExpirationTime(session_id);
}

void IEMEDrmBridge::UpdateKeyStatuses(const std::string& session_id,
                                      const eme::KeyStatusMap& key_statuses) {
  bool has_additional_usable_key = false;
  bool key_status_change = false;

  CdmKeysInfo cdm_keys_info;
  KayStatusesMap old_key_statuses_map;
  auto& current_key_statuses_map = session_key_statuses_map_[session_id];
  std::swap(old_key_statuses_map, current_key_statuses_map);

  for (const auto& key : key_statuses) {
    const auto key_id = CreateBinaryDataFromString(key.first);
    CHECK(!key_id.empty());

    CdmKeyInformation::KeyStatus key_status = ConvertKeyStatus(key.second);

    const auto it = old_key_statuses_map.find(key.first);
    if ((it == old_key_statuses_map.end() || it->second != key_status) &&
        // EME spec claims that onKeyStatusChange event should be fired for
        // pending key status, but it is suppressed due to problems repoted
        // by major content providers.
        key_status != CdmKeyInformation::KEY_STATUS_PENDING) {
      key_status_change = true;
      if (key_status == CdmKeyInformation::USABLE) {
        has_additional_usable_key = true;
      }
    }
    if (it != old_key_statuses_map.end()) {
      old_key_statuses_map.erase(it);
    }

    current_key_statuses_map[key.first] = key_status;

    LOG(INFO) << "Key status: " << base::HexEncode(key_id.data(), key_id.size())
              << ", " << key_status;
    cdm_keys_info.push_back(base::MakeUnique<CdmKeyInformation>(
        key_id, key_status, 0 /* there is no key system code */));
  }

  NotifyNewKey();

  // Session keys change callback will be fired when:
  // 1. New keys were added
  // 2. Keys were removed
  // 3. Keys statuses were changed
  if (key_status_change || !old_key_statuses_map.empty()) {
    session_keys_change_cb_.Run(session_id, has_additional_usable_key,
                                std::move(cdm_keys_info));
  }
}

void IEMEDrmBridge::UpdateExpirationTime(const std::string& session_id) {
  LOG(INFO) << "Updating expiration time session: " << session_id;

  int64_t expiration_time;
  const auto get_expiration_result_status =
      cdm_->session_getExpiration(session_id, &expiration_time);
  if (eme::kSuccess != get_expiration_result_status) {
    LOG(ERROR) << "Updating expiration time for session id: " << session_id
               << " failed with status: " << get_expiration_result_status;
    return;
  }

  LOG(INFO) << "Expiration time: " << expiration_time << " ms";

  const auto last_expiration_it = last_expiration_times_map_.find(session_id);

  if (last_expiration_times_map_.end() == last_expiration_it ||
      expiration_time != last_expiration_it->second) {
    last_expiration_times_map_[session_id] = expiration_time;
    NotifySessionExpirationUpdate(session_id, expiration_time);
  }
}

void IEMEDrmBridge::onRemoveComplete(const std::string& session_id) {
  LOG(INFO) << "session: " << session_id;
  const auto promise_it = session_promise_map_.find(session_id);
  CHECK(promise_it != session_promise_map_.end());

  if (promise_it != session_promise_map_.end()) {
    IEMEDrmBridge::ResolvePromise(promise_it->second);
  }
}

void IEMEDrmBridge::ResolvePromise(uint32_t promise_id) {
  cdm_promise_adapter_->ResolvePromise(promise_id);
}

void IEMEDrmBridge::NotifyNewKey() {
  if (!new_audio_key_cb_.is_null())
    new_audio_key_cb_.Run();

  if (!new_video_key_cb_.is_null())
    new_video_key_cb_.Run();
}

// From IEME doc: |expiry_time_ms| is the time, in milliseconds since 1970 UTC,
// after which the key(s) in the session will no longer be usable to decrypt
// media data, or -1 if no such time exists.
void IEMEDrmBridge::NotifySessionExpirationUpdate(const std::string& session_id,
                                                  int64_t expiry_time_ms) {
  if (expiry_time_ms < 0) {
    expiry_time_ms = 0;
  }

  session_expiration_update_cb_.Run(
      session_id, base::Time::FromDoubleT(expiry_time_ms / 1000.0));
}

ContentDecryptionModule* CreateIEMEDrmBridge(
    const string& key_system,
    const SessionMessageCB& session_message_cb,
    const SessionClosedCB& session_closed_cb,
    const SessionKeysChangeCB& session_keys_change_cb,
    const SessionExpirationUpdateCB& session_expiration_update_cb) {
  return IEMEDrmBridge::Create(key_system, session_message_cb,
                               session_closed_cb, session_keys_change_cb,
                               session_expiration_update_cb)
      .release();
}

}  // namespace media
