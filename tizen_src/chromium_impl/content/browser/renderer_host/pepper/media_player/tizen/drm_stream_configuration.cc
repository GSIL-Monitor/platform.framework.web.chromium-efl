// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/media_player/tizen/drm_stream_configuration.h"

#include <climits>
#include <string>
#include <type_traits>

#include "base/logging.h"
#include "ppapi/c/pp_errors.h"

namespace content {

namespace {

const char* kCencPsshDrmInitData = "cenc:pssh";
const char* kPlayreadyProDrmInitData = "mspr:pro";

const uint8_t kPlayreadySystemId[] = {
    // "9a04f079-9840-4286-ab92-e65be0885f95";
    0x9a, 0x04, 0xf0, 0x79, 0x98, 0x40, 0x42, 0x86,
    0xab, 0x92, 0xe6, 0x5b, 0xe0, 0x88, 0x5f, 0x95,
};

const uint8_t kWidevineSystemId[] = {
    // "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
    0xed, 0xef, 0x8b, 0xa9, 0x79, 0xd6, 0x4a, 0xce,
    0xa3, 0xc8, 0x27, 0xdc, 0xd5, 0x1d, 0x21, 0xed,
};

enum PlayReadyRecordTypes {
  kRightsManagementHeader = 0x1,
  kReserved = 0x2,
  kEmbeddedLicenseStore = 0x3
};

template <size_t N, size_t M>
bool SystemIdEqual(const uint8_t (&s0)[N], const uint8_t (&s1)[M]) {
  static_assert(N == 16 && M == 16, "Wrong SystemId length");
  for (size_t i = 0; i < N; ++i) {
    if (s0[i] != s1[i])
      return false;
  }

  return true;
}

}  // namespace

// TODO(a.bujalski): Check if this class isn't already present in Chromium
class ByteStreamReader {
 public:
  ByteStreamReader(const uint8_t* data, size_t);

  template <typename T>
  bool CheckPreconditions(size_t bytes);

  template <typename T>
  bool ReadBigEndian(T* out);

  template <typename T>
  bool ReadBytesBigEndian(T* out, size_t bytes);

  bool SkipBytes(size_t bytes);

  template <typename T>
  bool ReadLittleEndian(T* out);

  template <typename T>
  bool ReadBytesLittleEndian(T* out, size_t bytes);

  template <size_t N>
  bool ReadArray(uint8_t (&arr)[N]);

  bool AppendRest(std::vector<uint8_t>* data);

 private:
  const uint8_t* data_;
  size_t size_;

  const uint8_t* curr_byte_;
  size_t left_bytes_;
};

ByteStreamReader::ByteStreamReader(const uint8_t* data, size_t size)
    : data_(data), size_(size), curr_byte_(data), left_bytes_(size) {
  DCHECK(data_);
}

template <typename T>
bool ByteStreamReader::CheckPreconditions(size_t bytes) {
  static_assert(std::is_integral<T>::value, "Integer required.");

  DCHECK(bytes <= sizeof(T));
  if (bytes > sizeof(T)) {
    LOG(FATAL) << "Trying to read more bytes than sizeof(T)";
    return false;
  }

  DCHECK(left_bytes_ >= bytes);
  if (left_bytes_ < bytes)
    return false;

  return true;
}

template <typename T>
bool ByteStreamReader::ReadBigEndian(T* out) {
  return ReadBytesBigEndian(out, sizeof(T));
}

template <typename T>
bool ByteStreamReader::ReadBytesBigEndian(T* out, size_t bytes) {
  if (!CheckPreconditions<T>(bytes))
    return false;

  DCHECK(out);
  *out = 0;
  for (size_t i = 0; i < bytes; ++i) {
    *out <<= 8;
    *out |= *curr_byte_;
    ++curr_byte_;
  }

  left_bytes_ -= bytes;
  return true;
}

template <>
bool ByteStreamReader::ReadBytesBigEndian<uint8_t>(uint8_t* out, size_t bytes) {
  if (!CheckPreconditions<uint8_t>(bytes))
    return false;

  DCHECK(out);
  if (bytes == 1) {
    *out = *curr_byte_;
    ++curr_byte_;
    --left_bytes_;
  }
  return true;
}

bool ByteStreamReader::SkipBytes(size_t bytes) {
  if (left_bytes_ < bytes)
    return false;

  curr_byte_ += bytes;
  left_bytes_ -= bytes;
  return true;
}

template <typename T>
bool ByteStreamReader::ReadLittleEndian(T* out) {
  return ReadBytesLittleEndian(out, sizeof(T));
}

template <typename T>
bool ByteStreamReader::ReadBytesLittleEndian(T* out, size_t bytes) {
  if (!CheckPreconditions<T>(bytes))
    return false;

  DCHECK(out);
  *out = 0;
  for (size_t i = 0; i < bytes; ++i) {
    *out |= static_cast<T>(*curr_byte_) << (i * 8);
    ++curr_byte_;
  }

  left_bytes_ -= bytes;
  return true;
}

template <>
bool ByteStreamReader::ReadBytesLittleEndian<uint8_t>(uint8_t* out,
                                                      size_t bytes) {
  return ReadBytesBigEndian(out, bytes);
}

template <size_t N>
bool ByteStreamReader::ReadArray(uint8_t (&arr)[N]) {
  if (left_bytes_ < N)
    return false;

  for (size_t i = 0; i < N; ++i)
    ReadBigEndian(&arr[i]);

  return true;
}

bool ByteStreamReader::AppendRest(std::vector<uint8_t>* data) {
  if (!left_bytes_)
    return false;

  data->clear();
  data->insert(data->end(), curr_byte_, curr_byte_ + left_bytes_);
  left_bytes_ = 0;
  return true;
}

int32_t DRMStreamConfiguration::ParseDRMInitData(
    const std::vector<uint8_t>& type,
    const std::vector<uint8_t>& init_data) {
  int32_t error_code = PP_ERROR_NOTSUPPORTED;
  std::string type_str{reinterpret_cast<const char*>(type.data()), type.size()};
  if (type_str == kCencPsshDrmInitData)
    error_code = ParsePSSHBox(init_data);
  else if (type_str == kPlayreadyProDrmInitData)
    error_code = ParsePROHeader(init_data);
  return error_code;
}

int32_t DRMStreamConfiguration::ParsePSSHBox(
    const std::vector<uint8_t>& pssh_box) {
  ByteStreamReader reader(pssh_box.data(), pssh_box.size());
  // Taking advantage of the fact that && operator doesn't evaluates its 2nd
  // argument when it's not necessary.
  bool read_ok = true;

  // Box fields
  uint64_t size = 0;
  uint32_t box_type = 0;

  read_ok = read_ok && reader.ReadBytesBigEndian(&size, sizeof(uint32_t));
  read_ok = read_ok && reader.ReadBigEndian(&box_type);
  if (!read_ok) {
    LOG(ERROR) << "Cannot read box type or size";
    return PP_ERROR_BADARGUMENT;
  }

  if (size == 1)
    read_ok = read_ok && reader.ReadBigEndian(&size);

  // FullBox fields
  uint8_t version = 0;
  read_ok = read_ok && reader.ReadBigEndian(&version);

  static const size_t flags_field_bits = 24;
  static const size_t flags_field_bytes = flags_field_bits / 8;
  uint32_t flags = 0;
  read_ok = read_ok && reader.ReadBytesBigEndian(&flags, flags_field_bytes);
  if (!read_ok) {
    LOG(ERROR) << "Malformed FullBox";
    return PP_ERROR_BADARGUMENT;
  }

  // PSSH fields
  uint8_t system_id[16];
  read_ok = read_ok && reader.ReadArray(system_id);
  if (!read_ok) {
    LOG(ERROR) << "Malformed PSSH box (cannot read system ID)";
    return PP_ERROR_BADARGUMENT;
  }

  if (version > 0) {
    uint32_t kid_count = 0;
    read_ok = read_ok && reader.ReadBigEndian(&kid_count);
    for (uint32_t i = 0; read_ok && i < kid_count; ++i) {
      uint8_t kid[16];
      read_ok = read_ok && reader.ReadArray(kid);
    }

    if (!read_ok) {
      LOG(ERROR) << "Malformed PSSH box (cannot read version)";
      return PP_ERROR_BADARGUMENT;
    }
  }

  uint32_t data_size = 0;
  std::vector<uint8_t> drm_specific_data;
  read_ok = read_ok && reader.ReadBigEndian(&data_size);
  read_ok = read_ok && reader.AppendRest(&drm_specific_data);
  if (!read_ok) {
    LOG(ERROR) << "Malformed PSSH box (cannot read DRM specific data)";
    return PP_ERROR_BADARGUMENT;
  }

  DCHECK(drm_specific_data.size() == data_size);
  if (drm_specific_data.size() != data_size) {
    LOG(ERROR) << "Malformed PSSH box (cannot read DRM data size)";
    return PP_ERROR_BADARGUMENT;
  }

  if (SystemIdEqual(system_id, kPlayreadySystemId))
    return ParsePROHeader(drm_specific_data);

  if (SystemIdEqual(system_id, kWidevineSystemId)) {
    drm_type_ = PP_MEDIAPLAYERDRMTYPE_WIDEVINE_MODULAR;
    drm_specific_data_ = std::string{
        reinterpret_cast<const char*>(pssh_box.data()), pssh_box.size()};
    return PP_OK;
  }

  LOG(WARNING) << "Unknown DRM SystemId";
  return PP_ERROR_BADARGUMENT;
}

int32_t DRMStreamConfiguration::ParsePROHeader(
    const std::vector<uint8_t>& pro_header) {
  ByteStreamReader reader(pro_header.data(), pro_header.size());
  // Taking advantage of the fact that && operator doesn't evaluates its 2nd
  // argument when it's not necessary.
  bool read_ok = true;

  uint32_t length = 0;
  uint16_t records_count = 0;

  read_ok = read_ok && reader.ReadLittleEndian(&length);
  read_ok = read_ok && reader.ReadLittleEndian(&records_count);
  if (!read_ok) {
    LOG(ERROR) << "Malformed PRO header";
    return PP_ERROR_BADARGUMENT;
  }

  for (uint16_t i = 0; i < records_count; ++i) {
    uint16_t record_type = 0;
    uint16_t record_length = 0;

    read_ok = read_ok && reader.ReadLittleEndian(&record_type);
    read_ok = read_ok && reader.ReadLittleEndian(&record_length);
    if (!read_ok) {
      LOG(ERROR) << "Malformed PRO header";
      return PP_ERROR_BADARGUMENT;
    }

    int32_t error_code = PP_OK;
    switch (record_type) {
      case kRightsManagementHeader:
        error_code = ReadPlayReadyRightsManagementHeader(reader, record_length);
        break;
      case kReserved:
        LOG(ERROR) << "PRO: Got reserved header";
        error_code =
            (reader.SkipBytes(record_length) ? PP_OK : PP_ERROR_BADARGUMENT);
        break;
      case kEmbeddedLicenseStore:
        error_code = ReadPlayReadyEmbeddedLicense(reader, record_length);
        break;
      default:
        LOG(ERROR) << "PRO: Got unknown header";
        error_code =
            (reader.SkipBytes(record_length) ? PP_OK : PP_ERROR_BADARGUMENT);
        break;
    }

    if (error_code != PP_OK) {
      LOG(WARNING) << "Got error when parsing PRO record of type : "
                   << record_type << " errorCode: " << error_code;
      return error_code;
    }
  }

  drm_type_ = PP_MEDIAPLAYERDRMTYPE_PLAYREADY;
  drm_specific_data_ = std::string{
      reinterpret_cast<const char*>(pro_header.data()), pro_header.size()};
  return PP_OK;
}

int32_t DRMStreamConfiguration::ReadPlayReadyRightsManagementHeader(
    ByteStreamReader& /* reader */,
    uint16_t /* length */) {
  // TODO(a.bujalski) Update XML parsing
  //  String str;
  //  for (uint16_t i = 0; i < length; i += 2) {
  //    UChar ch = 0;
  //    if (!reader.readLittleEndian(ch)) return PP_ERROR_BADARGUMENT;

  //    str.append(ch);
  //  }

  //  RefPtr<WebCore::Document> document =
  //      WebCore::Document::createXHTML(0, WebCore::KURL());
  //  RefPtr<WebCore::DocumentFragment> fragment =
  //      WebCore::DocumentFragment::create(document.get());
  //  if (!fragment->parseXML(str, 0)) {
  //    LOG(WARNING) << "Parsing pro header failed";
  //    return PP_ERROR_BADARGUMENT;
  //  }

  //  const char* rmh_mandatory_tags[] = {"WRMHEADER"};
  //  for (auto mandatory_tag : rmh_mandatory_tags) {
  //    RefPtr<WebCore::NodeList> nodes =
  //        fragment->getElementsByTagName(mandatory_tag);
  //    if (!nodes->length()) {
  //      LOG(WARNING) << "Mandatory tag: " << mandatory_tag
  //                   << " not found in RMH document";
  //      return PP_ERROR_BADARGUMENT;
  //    }
  //  }

  return PP_OK;
}

int32_t DRMStreamConfiguration::ReadPlayReadyEmbeddedLicense(
    ByteStreamReader& reader,
    uint16_t length) {
  // FIXME currently skipped.
  //
  // According to MS PlayReady document for DASH content protection:
  //     It is NOT RECOMMENDED to include an ELS unless it is needed
  //     as part of a DRM Domain or a License Chain scheme.
  //
  // see:
  // http://www.microsoft.com/playready/documents/
  //     "DASH Content Protection using Microsoft PlayReady"
  return (reader.SkipBytes(length) ? PP_OK : PP_ERROR_BADARGUMENT);
}

}  // namespace content
