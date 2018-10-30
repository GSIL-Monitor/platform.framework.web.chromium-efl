// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/os_crypt.h"

#include <stddef.h>

#include <memory>

#include "base/base_paths.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "crypto/encryptor.h"
#include "crypto/symmetric_key.h"

#if defined(S_TERRACE_SUPPORT)
#include "base/synchronization/lock.h"
#endif

namespace {

// Salt for Symmetric key derivation.
const char kSalt[] = "saltysalt";

// Key size required for 128 bit AES.
const size_t kDerivedKeySizeInBits = 128;

#if defined(S_TERRACE_SUPPORT)
// Key size required for 256 bit AES.
const size_t kDerivedKeySizeInBits256 = 256;
#endif  // defined(S_TERRACE_SUPPORT)

// Constant for Symmetic key derivation.
const size_t kEncryptionIterations = 1;

// Size of initialization vector for AES 128-bit.
const size_t kIVBlockSizeAES128 = 16;

// Prefix for cypher text returned by obfuscation version.  We prefix the
// cyphertext with this string so that future data migration can detect
// this and migrate to full encryption without data loss.
const char kObfuscationPrefix[] = "v10";

// Generates a newly allocated SymmetricKey object based a hard-coded password.
// Ownership of the key is passed to the caller.  Returns NULL key if a key
// generation error occurs.
crypto::SymmetricKey* GetEncryptionKey() {
  // We currently "obfuscate" by encrypting and decrypting with hard-coded
  // password.  We need to improve this password situation by moving a secure
  // password into a system-level key store.
  // http://crbug.com/25404 and http://crbug.com/49115
  std::string password = "Ekd15zhd";
  std::string salt(kSalt);

  // Create an encryption key from our password and salt.
  std::unique_ptr<crypto::SymmetricKey> encryption_key(
      crypto::SymmetricKey::DeriveKeyFromPassword(crypto::SymmetricKey::AES,
                                                  password,
                                                  salt,
                                                  kEncryptionIterations,
                                                  kDerivedKeySizeInBits));
  DCHECK(encryption_key.get());

  return encryption_key.release();
}

#if defined(S_TERRACE_SUPPORT)

static const int kIVSize = 16;

crypto::SymmetricKey* GetEncryptionKey256(std::string password) {
  // This function will generate a unique key for passowrd
  std::string salt(kSalt);

  // Create an encryption key from our password and salt.
  std::unique_ptr<crypto::SymmetricKey> encryption_key(
      crypto::SymmetricKey::DeriveKeyFromPassword(
          crypto::SymmetricKey::AES, password, salt, kEncryptionIterations,
          kDerivedKeySizeInBits256));
  DCHECK(encryption_key.get());

  return encryption_key.release();
}

void GenerateInitializationVector(std::string* output) {
  static const char alphanum[] =
      "0123456789!@#$%^&*ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "!@#$%^&(){}[]abcdefghijklmnopqrstuvwxyz";
  srand(time(0));
  int string_range = sizeof(alphanum) - 1;
  std::unique_ptr<unsigned char[]> initialization_vector(
      new unsigned char[kIVSize + 1]);

  DCHECK(initialization_vector);
  for (unsigned i = 0; i < kIVSize; i++)
    initialization_vector[i] = alphanum[rand() % string_range];
  initialization_vector[kIVSize] = 0;  // terminating IV with null

  output->assign(reinterpret_cast<const char*>(initialization_vector.get()));
}

long (*pWBS_Enc)(unsigned char* ct,
                 unsigned char* pt,
                 long size,
                 unsigned char* iv);

long (*pWBS_Dec)(unsigned char* pt,
                 unsigned char* ct,
                 long size,
                 unsigned char* iv);

bool InitWbsLib() {
  base::FilePath path;
  if (!PathService::Get(base::DIR_LOCAL_LIBRARY, &path)) {
    LOG(ERROR) << "InitWbsLib: PathService::Get(DIR_LOCAL_LIBRARY) failed";
    return false;
  }
  path = path.Append(FILE_PATH_LITERAL("libwbs.so"));

  base::NativeLibraryLoadError error;
  static base::NativeLibrary library = base::LoadNativeLibrary(path, &error);
  if (!library) {
    if (!PathService::Get(base::DIR_LIBRARY_APK, &path)) {
      LOG(ERROR) << "InitWbsLib: PathService::Get(DIR_LIBRARY_APK) failed";
      return false;
    }

    path = path.ReplaceExtension("apk!");
    path = path.Append(FILE_PATH_LITERAL("lib/armeabi-v7a/libwbs.so"));

    library = base::LoadNativeLibrary(path, &error);
    if (!library) {
      LOG(ERROR) << "InitWbsLib: LoadNativeLibrary failed [error="
                 << error.ToString() << "]";
      return false;
    }
  }

  pWBS_Enc = (long (*)(unsigned char*, unsigned char*, long, unsigned char*))
      base::GetFunctionPointerFromNativeLibrary(library, "WBS_Enc");
  pWBS_Dec = (long (*)(unsigned char*, unsigned char*, long, unsigned char*))
      base::GetFunctionPointerFromNativeLibrary(library, "WBS_Dec");
  return true;
}

long WBS_Enc(unsigned char* ct,
             unsigned char* pt,
             long size,
             unsigned char* iv) {
  if (!pWBS_Enc && !InitWbsLib()) {
    LOG(ERROR) << "WBS_Enc: library is not loaded";
    return 0;
  }
  return pWBS_Enc(ct, pt, size, iv);
}

long WBS_Dec(unsigned char* pt,
             unsigned char* ct,
             long size,
             unsigned char* iv) {
  if (!pWBS_Dec && !InitWbsLib()) {
    LOG(ERROR) << "WBS_Dec: library is not loaded";
    return 0;
  }
  return pWBS_Dec(pt, ct, size, iv);
}

#endif  // defined(S_TERRACE_SUPPORT)

}  // namespace

bool OSCrypt::EncryptString16(const base::string16& plaintext,
                              std::string* ciphertext) {
  return EncryptString(base::UTF16ToUTF8(plaintext), ciphertext);
}

bool OSCrypt::DecryptString16(const std::string& ciphertext,
                              base::string16* plaintext) {
  std::string utf8;
  if (!DecryptString(ciphertext, &utf8))
    return false;

  *plaintext = base::UTF8ToUTF16(utf8);
  return true;
}

bool OSCrypt::EncryptString(const std::string& plaintext,
                            std::string* ciphertext) {
  // This currently "obfuscates" by encrypting with hard-coded password.
  // We need to improve this password situation by moving a secure password
  // into a system-level key store.
  // http://crbug.com/25404 and http://crbug.com/49115

  if (plaintext.empty()) {
    *ciphertext = std::string();
    return true;
  }

  std::unique_ptr<crypto::SymmetricKey> encryption_key(GetEncryptionKey());
  if (!encryption_key.get())
    return false;

  std::string iv(kIVBlockSizeAES128, ' ');
  crypto::Encryptor encryptor;
  if (!encryptor.Init(encryption_key.get(), crypto::Encryptor::CBC, iv))
    return false;

  if (!encryptor.Encrypt(plaintext, ciphertext))
    return false;

  // Prefix the cypher text with version information.
  ciphertext->insert(0, kObfuscationPrefix);
  return true;
}

bool OSCrypt::DecryptString(const std::string& ciphertext,
                            std::string* plaintext) {
  // This currently "obfuscates" by encrypting with hard-coded password.
  // We need to improve this password situation by moving a secure password
  // into a system-level key store.
  // http://crbug.com/25404 and http://crbug.com/49115

  if (ciphertext.empty()) {
    *plaintext = std::string();
    return true;
  }

  // Check that the incoming cyphertext was indeed encrypted with the expected
  // version.  If the prefix is not found then we'll assume we're dealing with
  // old data saved as clear text and we'll return it directly.
  // Credit card numbers are current legacy data, so false match with prefix
  // won't happen.
  if (!base::StartsWith(ciphertext, kObfuscationPrefix,
                        base::CompareCase::SENSITIVE)) {
    *plaintext = ciphertext;
    return true;
  }

  // Strip off the versioning prefix before decrypting.
  std::string raw_ciphertext = ciphertext.substr(strlen(kObfuscationPrefix));

  std::unique_ptr<crypto::SymmetricKey> encryption_key(GetEncryptionKey());
  if (!encryption_key.get())
    return false;

  std::string iv(kIVBlockSizeAES128, ' ');
  crypto::Encryptor encryptor;
  if (!encryptor.Init(encryption_key.get(), crypto::Encryptor::CBC, iv))
    return false;

  if (!encryptor.Decrypt(raw_ciphertext, plaintext))
    return false;

  return true;
}

#if defined(S_TERRACE_SUPPORT)

std::string OSCrypt::GetKey256(std::string password) {
  std::unique_ptr<crypto::SymmetricKey> encryption_key(
      GetEncryptionKey256(password));
  if (!encryption_key.get())
    return nullptr;

  // FIXME: m61_3163 bringup, needs to be checked by author
  // https://codereview.chromium.org/2934893003
  // This is DEK used to encrypt password with AES256
  return encryption_key.get()->key();
}

bool OSCrypt::EncryptString(const std::string& plaintext,
                            std::string* ciphertext,
                            std::string& aes256key) {
  if (plaintext.empty()) {
    *ciphertext = std::string();
    return true;
  }

  std::unique_ptr<crypto::SymmetricKey> sym_key(
      crypto::SymmetricKey::Import(crypto::SymmetricKey::AES, aes256key));
  if (!sym_key.get())
    return false;

  std::string iv(kIVBlockSizeAES128, ' ');
  crypto::Encryptor encryptor;
  if (!encryptor.Init(sym_key.get(), crypto::Encryptor::CBC, iv))
    return false;

  if (!encryptor.Encrypt(plaintext, ciphertext))
    return false;

  // Prefix the cypher text with version information.
  ciphertext->insert(0, kObfuscationPrefix);
  return true;
}

bool OSCrypt::DecryptString(const std::string& ciphertext,
                            std::string* plaintext,
                            std::string& aes256key) {
  if (ciphertext.empty()) {
    *plaintext = std::string();
    return true;
  }

  // Check that the incoming cyphertext was indeed encrypted with the expected
  // version.  If the prefix is not found then we'll assume we're dealing with
  // old data saved as clear text and we'll return it directly.
  // Credit card numbers are current legacy data, so false match with prefix
  // won't happen.
  if (!base::StartsWith(ciphertext, kObfuscationPrefix,
                        base::CompareCase::SENSITIVE)) {
    *plaintext = ciphertext;
    return true;
  }

  // Strip off the versioning prefix before decrypting.
  std::string raw_ciphertext = ciphertext.substr(strlen(kObfuscationPrefix));

  std::unique_ptr<crypto::SymmetricKey> sym_key(
      crypto::SymmetricKey::Import(crypto::SymmetricKey::AES, aes256key));
  if (!sym_key.get())
    return false;

  std::string iv(kIVBlockSizeAES128, ' ');
  crypto::Encryptor encryptor;
  if (!encryptor.Init(sym_key.get(), crypto::Encryptor::CBC, iv))
    return false;

  if (!encryptor.Decrypt(raw_ciphertext, plaintext))
    return false;

  return true;
}

void OSCrypt::WBSEncrypt(const std::string& raw_data,
                         std::string* out_encrypted_data,
                         std::string* out_iv,
                         std::vector<unsigned char>* out_encrypted_key) {
  std::string key = GetKey256(raw_data);

  // Fix for the key containing 00 binary data for the entered password
  // generate key using chrome passkey.
  if (strlen(key.data()) < 32)
    key = GetKey256("Ekd15zhd");

  EncryptString(raw_data, out_encrypted_data, key);

  GenerateInitializationVector(out_iv);
  unsigned char* cipher_text =
      new unsigned char[(key.length() / kIVSize) * kIVSize + kIVSize];
  long cipher_text_length = WBS_Enc(
      cipher_text,
      reinterpret_cast<unsigned char*>(const_cast<char*>(key.data())),
      strlen(key.data()),
      reinterpret_cast<unsigned char*>(const_cast<char*>(out_iv->data())));

  out_encrypted_key->assign(cipher_text, cipher_text + cipher_text_length);
  if (cipher_text) {
    delete[] cipher_text;
    cipher_text = nullptr;
  }
}

void OSCrypt::WBSDecrypt(const std::string& encrypted_data,
                         const std::string& iv,
                         const std::vector<unsigned char>& encrypted_key,
                         std::string* out_decrypted_data) {
  long key_length = encrypted_key.size();
  if (key_length <= 0)
    key_length = 48;

  // This is for prevent crash of libwbs.so.
  // The crash can be occurred, if function is called twice at same time.
  static base::Lock* lock = new base::Lock;
  base::AutoLock auto_lock(*lock);

  unsigned char* decrypted_key = new unsigned char[key_length];
  WBS_Dec(decrypted_key, const_cast<unsigned char*>(&encrypted_key.front()),
          key_length,
          reinterpret_cast<unsigned char*>(const_cast<char*>(iv.data())));

  if (!decrypted_key)
    return;

  std::string key(reinterpret_cast<const char*>(decrypted_key));
  DecryptString(encrypted_data, out_decrypted_data, key);

  if (decrypted_key) {
    delete[] decrypted_key;
    decrypted_key = nullptr;
  }
}

#endif  // defined(S_TERRACE_SUPPORT)
