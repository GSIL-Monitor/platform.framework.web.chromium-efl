// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crx_file/crx_verifier.h"

#include <cstring>
#include <iterator>
#include <memory>
#include <set>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "components/crx_file/crx2_file.h"
#include "components/crx_file/crx3.pb.h"
#include "components/crx_file/id_util.h"
#include "crypto/secure_hash.h"
#include "crypto/secure_util.h"
#include "crypto/sha2.h"
#include "crypto/signature_verifier.h"

namespace crx_file {

namespace {

// The maximum size the Crx2 parser will tolerate for a public key.
constexpr uint32_t kMaxPublicKeySize = 1 << 16;

// The maximum size the Crx2 parser will tolerate for a signature.
constexpr uint32_t kMaxSignatureSize = 1 << 16;

// The maximum size the Crx3 parser will tolerate for a header.
constexpr uint32_t kMaxHeaderSize = 1 << 18;

// The context for Crx3 signing, encoded in UTF8.
constexpr unsigned char kSignatureContext[] = u8"CRX3 SignedData";

// The SHA256 hash of the "ecdsa_2017_public" Crx3 key.
constexpr uint8_t kPublisherKeyHash[] = {
    0x61, 0xf7, 0xf2, 0xa6, 0xbf, 0xcf, 0x74, 0xcd, 0x0b, 0xc1, 0xfe,
    0x24, 0x97, 0xcc, 0x9b, 0x04, 0x25, 0x4c, 0x65, 0x8f, 0x79, 0xf2,
    0x14, 0x53, 0x92, 0x86, 0x7e, 0xa8, 0x36, 0x63, 0x67, 0xcf};

using VerifierCollection =
    std::vector<std::unique_ptr<crypto::SignatureVerifier>>;
using RepeatedProof = google::protobuf::RepeatedPtrField<AsymmetricKeyProof>;

int ReadAndHashBuffer(uint8_t* buffer,
                      int length,
                      base::File* file,
                      crypto::SecureHash* hash) {
  static_assert(sizeof(char) == sizeof(uint8_t), "Unsupported char size.");
  int read = file->ReadAtCurrentPos(reinterpret_cast<char*>(buffer), length);
  hash->Update(buffer, read);
  return read;
}

// Returns UINT32_MAX in the case of an unexpected EOF or read error, else
// returns the read uint32.
uint32_t ReadAndHashLittleEndianUInt32(base::File* file,
                                       crypto::SecureHash* hash) {
  uint8_t buffer[4] = {};
  if (ReadAndHashBuffer(buffer, 4, file, hash) != 4)
    return UINT32_MAX;
  return buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0];
}

// Read to the end of the file, updating the hash and all verifiers.
bool ReadHashAndVerifyArchive(base::File* file,
                              crypto::SecureHash* hash,
                              const VerifierCollection& verifiers) {
  uint8_t buffer[1 << 12] = {};
  size_t len = 0;
  while ((len = ReadAndHashBuffer(buffer, arraysize(buffer), file, hash)) > 0) {
    for (auto& verifier : verifiers)
      verifier->VerifyUpdate(buffer, len);
  }
  for (auto& verifier : verifiers) {
    if (!verifier->VerifyFinal())
      return false;
  }
  return true;
}

// The remaining contents of a Crx3 file are [header-size][header][archive].
// [header] is an encoded protocol buffer and contains both a signed and
// unsigned section. The unsigned section contains a set of key/signature pairs,
// and the signed section is the encoding of another protocol buffer. All
// signatures cover [prefix][signed-header-size][signed-header][archive].
VerifierResult VerifyCrx3(
    base::File* file,
    crypto::SecureHash* hash,
    const std::vector<std::vector<uint8_t>>& required_key_hashes,
    std::string* public_key,
    std::string* crx_id,
    bool require_publisher_key) {
  // Parse [header-size] and [header].
  const uint32_t header_size = ReadAndHashLittleEndianUInt32(file, hash);
  if (header_size > kMaxHeaderSize)
    return VerifierResult::ERROR_HEADER_INVALID;
  std::vector<uint8_t> header_bytes(header_size);
  // Assuming kMaxHeaderSize can fit in an int, the following cast is safe.
  if (ReadAndHashBuffer(header_bytes.data(), header_size, file, hash) !=
      static_cast<int>(header_size))
    return VerifierResult::ERROR_HEADER_INVALID;
  CrxFileHeader header;
  if (!header.ParseFromArray(header_bytes.data(), header_size))
    return VerifierResult::ERROR_HEADER_INVALID;

  // Parse [signed-header].
  const std::string& signed_header_data_str = header.signed_header_data();
  SignedData signed_header_data;
  if (!signed_header_data.ParseFromString(signed_header_data_str))
    return VerifierResult::ERROR_HEADER_INVALID;
  const std::string& crx_id_encoded = signed_header_data.crx_id();
  const std::string declared_crx_id = id_util::GenerateIdFromHex(
      base::HexEncode(crx_id_encoded.data(), crx_id_encoded.size()));

  // Create a little-endian representation of [signed-header-size].
  const int signed_header_size = signed_header_data_str.size();
  const uint8_t header_size_octets[] = {
      signed_header_size, signed_header_size >> 8, signed_header_size >> 16,
      signed_header_size >> 24};

  // Create a set of all required key hashes.
  std::set<std::vector<uint8_t>> required_key_set(required_key_hashes.begin(),
                                                  required_key_hashes.end());
  if (require_publisher_key) {
    required_key_set.emplace(std::begin(kPublisherKeyHash),
                             std::end(kPublisherKeyHash));
  }

  using ProofFetcher = const RepeatedProof& (CrxFileHeader::*)() const;
  ProofFetcher rsa = &CrxFileHeader::sha256_with_rsa;
  ProofFetcher ecdsa = &CrxFileHeader::sha256_with_ecdsa;

  std::string public_key_bytes;
  VerifierCollection verifiers;
  verifiers.reserve(header.sha256_with_rsa_size() +
                    header.sha256_with_ecdsa_size());
  const std::vector<
      std::pair<ProofFetcher, crypto::SignatureVerifier::SignatureAlgorithm>>
      proof_types = {
          std::make_pair(rsa, crypto::SignatureVerifier::RSA_PKCS1_SHA256),
          std::make_pair(ecdsa, crypto::SignatureVerifier::ECDSA_SHA256)};

  // Initialize all verifiers and update them with
  // [prefix][signed-header-size][signed-header].
  // Clear any elements of required_key_set that are encountered, and watch for
  // the developer key.
  for (const auto& proof_type : proof_types) {
    for (const auto& proof : (header.*proof_type.first)()) {
      const std::string& key = proof.public_key();
      const std::string& sig = proof.signature();
      if (id_util::GenerateId(key) == declared_crx_id)
        public_key_bytes = key;
      std::vector<uint8_t> key_hash(crypto::kSHA256Length);
      crypto::SHA256HashString(key, key_hash.data(), key_hash.size());
      required_key_set.erase(key_hash);
      auto v = base::MakeUnique<crypto::SignatureVerifier>();
      static_assert(sizeof(unsigned char) == sizeof(uint8_t),
                    "Unsupported char size.");
      if (!v->VerifyInit(
              proof_type.second, reinterpret_cast<const uint8_t*>(sig.data()),
              sig.size(), reinterpret_cast<const uint8_t*>(key.data()),
              key.size()))
        return VerifierResult::ERROR_SIGNATURE_INITIALIZATION_FAILED;
      v->VerifyUpdate(kSignatureContext, arraysize(kSignatureContext));
      v->VerifyUpdate(header_size_octets, arraysize(header_size_octets));
      v->VerifyUpdate(
          reinterpret_cast<const uint8_t*>(signed_header_data_str.data()),
          signed_header_data_str.size());
      verifiers.push_back(std::move(v));
    }
  }
  if (public_key_bytes.empty() || !required_key_set.empty())
    return VerifierResult::ERROR_REQUIRED_PROOF_MISSING;

  // Update and finalize the verifiers with [archive].
  if (!ReadHashAndVerifyArchive(file, hash, verifiers))
    return VerifierResult::ERROR_SIGNATURE_VERIFICATION_FAILED;

  base::Base64Encode(public_key_bytes, public_key);
  *crx_id = declared_crx_id;
  return VerifierResult::OK_FULL;
}

VerifierResult VerifyCrx2(
    base::File* file,
    crypto::SecureHash* hash,
    const std::vector<std::vector<uint8_t>>& required_key_hashes,
    std::string* public_key,
    std::string* crx_id) {
  const uint32_t key_size = ReadAndHashLittleEndianUInt32(file, hash);
  if (key_size > kMaxPublicKeySize)
    return VerifierResult::ERROR_HEADER_INVALID;
  const uint32_t sig_size = ReadAndHashLittleEndianUInt32(file, hash);
  if (sig_size > kMaxSignatureSize)
    return VerifierResult::ERROR_HEADER_INVALID;
  std::vector<uint8_t> key(key_size);
  if (ReadAndHashBuffer(key.data(), key_size, file, hash) !=
      static_cast<int>(key_size))
    return VerifierResult::ERROR_HEADER_INVALID;
  for (const auto& expected_hash : required_key_hashes) {
    // In practice we expect zero or one key_hashes_ for Crx2 files.
    std::vector<uint8_t> hash(crypto::kSHA256Length);
    std::unique_ptr<crypto::SecureHash> sha256 =
        crypto::SecureHash::Create(crypto::SecureHash::SHA256);
    sha256->Update(key.data(), key.size());
    sha256->Finish(hash.data(), hash.size());
    if (hash != expected_hash)
      return VerifierResult::ERROR_REQUIRED_PROOF_MISSING;
  }

  std::vector<uint8_t> sig(sig_size);
  if (ReadAndHashBuffer(sig.data(), sig_size, file, hash) !=
      static_cast<int>(sig_size))
    return VerifierResult::ERROR_HEADER_INVALID;
  std::vector<std::unique_ptr<crypto::SignatureVerifier>> verifiers;
  verifiers.push_back(base::MakeUnique<crypto::SignatureVerifier>());
  if (!verifiers[0]->VerifyInit(crypto::SignatureVerifier::RSA_PKCS1_SHA1,
                                sig.data(), sig.size(), key.data(),
                                key.size())) {
    return VerifierResult::ERROR_SIGNATURE_INITIALIZATION_FAILED;
  }

  if (!ReadHashAndVerifyArchive(file, hash, verifiers))
    return VerifierResult::ERROR_SIGNATURE_VERIFICATION_FAILED;

  const std::string public_key_bytes(key.begin(), key.end());
  base::Base64Encode(public_key_bytes, public_key);
  *crx_id = id_util::GenerateId(public_key_bytes);
  return VerifierResult::OK_FULL;
}

}  // namespace

VerifierResult Verify(
    const base::FilePath& crx_path,
    const VerifierFormat& format,
    const std::vector<std::vector<uint8_t>>& required_key_hashes,
    const std::vector<uint8_t>& required_file_hash,
    std::string* public_key,
    std::string* crx_id) {
  std::string public_key_local;
  std::string crx_id_local;
  base::File file(crx_path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid())
    return VerifierResult::ERROR_FILE_NOT_READABLE;

  std::unique_ptr<crypto::SecureHash> file_hash =
      crypto::SecureHash::Create(crypto::SecureHash::SHA256);

  // Magic number.
  bool diff = false;
  char buffer[kCrx2FileHeaderMagicSize] = {};
  if (file.ReadAtCurrentPos(buffer, kCrx2FileHeaderMagicSize) !=
      kCrx2FileHeaderMagicSize)
    return VerifierResult::ERROR_HEADER_INVALID;
  if (!strncmp(buffer, kCrxDiffFileHeaderMagic, kCrx2FileHeaderMagicSize))
    diff = true;
  else if (strncmp(buffer, kCrx2FileHeaderMagic, kCrx2FileHeaderMagicSize))
    return VerifierResult::ERROR_HEADER_INVALID;
  file_hash->Update(buffer, sizeof(buffer));

  // Version number.
  const uint32_t version =
      ReadAndHashLittleEndianUInt32(&file, file_hash.get());
  VerifierResult result;
  if (format == VerifierFormat::CRX2_OR_CRX3 &&
      (version == 2 || (diff && version == 0)))
    result = VerifyCrx2(&file, file_hash.get(), required_key_hashes,
                        &public_key_local, &crx_id_local);
  else if (version == 3)
    result = VerifyCrx3(&file, file_hash.get(), required_key_hashes,
                        &public_key_local, &crx_id_local,
                        format == VerifierFormat::CRX3_WITH_PUBLISHER_PROOF);
  else
    result = VerifierResult::ERROR_HEADER_INVALID;
  if (result != VerifierResult::OK_FULL)
    return result;

  // Finalize file hash.
  uint8_t final_hash[crypto::kSHA256Length] = {};
  file_hash->Finish(final_hash, sizeof(final_hash));
  if (!required_file_hash.empty()) {
    if (required_file_hash.size() != crypto::kSHA256Length)
      return VerifierResult::ERROR_EXPECTED_HASH_INVALID;
    if (!crypto::SecureMemEqual(final_hash, required_file_hash.data(),
                                crypto::kSHA256Length))
      return VerifierResult::ERROR_FILE_HASH_FAILED;
  }

  // All is well. Set the out-params and return.
  if (public_key)
    *public_key = public_key_local;
  if (crx_id)
    *crx_id = crx_id_local;
  return diff ? VerifierResult::OK_DELTA : VerifierResult::OK_FULL;
}

}  // namespace crx_file
