// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_drm_session.h"

#include <algorithm>

#include "content/browser/renderer_host/pepper/media_player/tizen/pepper_tizen_drm_manager.h"

namespace {

// Privacy mode for IEME. See their header for details. Summary:
// true - require server certificate
// false - do not require server certificate
const bool kIemePrivacyMode = false;

const char* GetKeySystemName(PP_MediaPlayerDRMType drm_type) {
  switch (drm_type) {
    case PP_MEDIAPLAYERDRMTYPE_PLAYREADY:
      return "com.microsoft.playready";
    case PP_MEDIAPLAYERDRMTYPE_WIDEVINE_MODULAR:
      return "com.widevine.alpha";
    default:
      return nullptr;
  }
}

eme::InitDataType GetInitDataType(PP_MediaPlayerDRMType drm_type) {
  switch (drm_type) {
    default:  // PP_MEDIAPLAYERDRMTYPE_PLAYREADY
      return eme::kCenc;
  }
}

std::string EmeStatusToString(eme::Status status) {
  std::ostringstream result;
  switch (status) {
    case eme::kSuccess:
      result << "Success";
      break;
    case eme::kNeedsDeviceCertificate:
      result << "Device Certificate is missing";
      break;
    case eme::kSessionNotFound:
      result << "Given session id is invalid";
      break;
    case eme::kDecryptError:
      result << "Decryption failed";
      break;
    case eme::kNoKey:
      result << "No Key present in the given session";
      break;
    case eme::kTypeError:
      result << "Data type error";
      break;
    case eme::kNotSupported:
      result << "Not supported";
      break;
    case eme::kInvalidState:
      result << "Invalid state";
      break;
    case eme::kQuotaExceeded:
      result << "Quota exceed";
      break;
    case eme::kInvalidHandle:
      result << "Invalid handle";
      break;
    case eme::kRangeError:
      result << "Range error";
      break;
    case eme::kSupported:
      // IEME.h defines this as "Is Supported used to check for a given KS";
      // it's hardly an error, but it's included here for completness.
      result << "Supported";
      break;
    case eme::kUnexpectedError:
      result << "Unexpected error";
      break;
    default:
      result << "Unknown status code";
  }
  result << " (" << static_cast<int32_t>(status) << ")";
  return result.str();
}

}  // anonymous namespace

namespace content {

void HandleAndSizeDeleter::operator()(handle_and_size_s* ptr) const {
  if (!ptr)
    return;

  if (ptr->handle != 0 && release_handle(ptr) != 0)
    LOG(WARNING) << "Failed to release TrustZone handle.";

  // handle_and_size_s struct is allocated using malloc, so free() it
  free(ptr);
}

// static
scoped_refptr<PepperTizenDRMSession> PepperTizenDRMSession::Create(
    PP_MediaPlayerDRMType drm_type,
    eme::IEventListener* listener) {
  auto keysystem_name = GetKeySystemName(drm_type);
  if (!keysystem_name) {
    LOG(ERROR) << "Cannot get keysystem name for drm_type == " << drm_type;
    return nullptr;
  }

  std::string keysystem_string = keysystem_name;
  if (!eme::IEME::isKeySystemSupported(keysystem_string)) {
    LOG(ERROR) << "Keysystem " << keysystem_string << " is not supported.";
    return nullptr;
  }

  auto session =
      new PepperTizenDRMSession(drm_type, keysystem_string, listener);
  if (!session->InitializeSession()) {
    delete session;
    return nullptr;
  }
  return session;
}

PepperTizenDRMSession::PepperTizenDRMSession(PP_MediaPlayerDRMType drm_type,
                                             const std::string& keysystem_name,
                                             eme::IEventListener* listener)
    : drm_type_(drm_type),
      ieme_(eme::IEME::create(listener, keysystem_name, kIemePrivacyMode),
            eme::IEME::destroy) {
  if (!ieme_) {
    LOG(ERROR) << "Cannot create a supported keysystem (" << keysystem_name
               << ")!";
    NOTREACHED();
  }
}

PepperTizenDRMSession::~PepperTizenDRMSession() {
  if (!session_id_.empty()) {
    ieme_->session_close(session_id_);
  }
}

bool PepperTizenDRMSession::InitializeSession() {
  if (is_valid()) {
    LOG(WARNING) << "Trying to reinitialize a session.";
    return false;
  }

  eme::Status status = ieme_->session_create(eme::kTemporary, &session_id_);
  if (status != eme::kSuccess) {
    LOG(ERROR) << "Cannot create DRM session: " << EmeStatusToString(status);
    session_id_.clear();
    return false;
  }
  return true;
}

bool PepperTizenDRMSession::Initialize(const std::string& init_data) {
  eme::Status status = ieme_->session_generateRequest(
      session_id_, GetInitDataType(drm_type_), init_data);
  if (status != eme::kSuccess) {
    LOG(ERROR) << "Cannot generate request: " << EmeStatusToString(status);
    return false;
  }

  return true;
}

ScopedHandleAndSize PepperTizenDRMSession::Decrypt(
    const ppapi::EncryptedESPacket& packet) {
  if (!is_valid()) {
    LOG(ERROR) << "Cannot decrypt data without a session.";
    return {};
  }

  std::vector<MSD_SUBSAMPLE_INFO> subsamples;
  subsamples.reserve(packet.Subsamples().size());
  for (const auto& subsample : packet.Subsamples()) {
    subsamples.emplace_back(
        MSD_SUBSAMPLE_INFO{subsample.clear_bytes, subsample.cipher_bytes});
  }

  MSD_FMP4_DATA subsamples_info{
      subsamples.size(), subsamples.data(),
  };

  // CENC assumes that IV with size 8 is actually IV with size 16 with last 8
  // bytes set to 0.
  std::vector<uint8_t> iv_data(std::max(packet.IvSize(), 16u), 0);
  memcpy(iv_data.data(), packet.IvData(), packet.IvSize());

  // see ieme_drm_bridge.cc
  msd_cipher_param_s params{
      MSD_AES128_CTR,   // algorithm
      MSD_FORMAT_FMP4,  // format
      MSD_PHASE_NONE,   // phase
      false,            // false -> protected
                        // with TrustZone
      reinterpret_cast<unsigned char*>(
          const_cast<void*>(packet.KeyIdData())),  // pkid
      packet.KeyIdSize(),                          // ukidlen
      reinterpret_cast<unsigned char*>(
          const_cast<void*>(packet.Data())),  // pdata
      packet.DataSize(),                      // udatalen
      nullptr,                                // poutbuf
      0,                                      // uoutbuflen
      iv_data.data(),                         // piv
      iv_data.size(),                         // uivlen
      &subsamples_info,                       // psubdata
      nullptr,                                // psplitoffsets
  };
  msd_cipher_param_s* params_ptr = &params;
  // Will be malloc-ked by eme_decryptarray
  handle_and_size_s* trust_zone_handle_ptr = nullptr;
  auto decrypt_result = eme_decryptarray(ieme_->getDecryptor(), &params_ptr, 1,
                                         nullptr, 0, &trust_zone_handle_ptr);
  auto handle = AdoptHandle(trust_zone_handle_ptr);

  if (decrypt_result == E_NEED_KEY) {
    LOG(ERROR)
        << "Cannot decrypt data without a key. Please install a license first.";
    return {};
  } else if (decrypt_result != E_SUCCESS) {
    LOG(ERROR) << "Decryption error: " << decrypt_result;
    return {};
  }
  return std::move(handle);
}

bool PepperTizenDRMSession::InstallLicense(const std::string& response) {
  if (!is_valid()) {
    LOG(ERROR) << "Cannot install a license without a session.";
    return false;
  }
  auto update_result = ieme_->session_update(session_id_, response);
  if (update_result != eme::kSuccess) {
    LOG(ERROR) << "Cannot install license: "
               << EmeStatusToString(update_result);
    return false;
  }
  return true;
}

ScopedHandleAndSize AdoptHandle(handle_and_size_s* handle_and_size) {
  return ScopedHandleAndSize{handle_and_size};
}

void ClearHandle(handle_and_size_s* handle_and_size) {
  handle_and_size->handle = 0;
  handle_and_size->size = 0;
}

}  // namespace content
