// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/cert/cert_database.h"

#include <cert.h>
#include <certdb.h>
#include <pk11pub.h>
#include <secmod.h>

#include "base/logging.h"
#include "base/observer_list_threadsafe.h"
#include "crypto/nss_util.h"
#include "crypto/scoped_nss_types.h"
#include "net/base/net_errors.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util_nss.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "crypto/ec_private_key.h"
#endif

namespace net {

CertDatabase::CertDatabase()
    : observer_list_(new base::ObserverListThreadSafe<Observer>) {
  crypto::EnsureNSSInit();
}

CertDatabase::~CertDatabase() {}

#if defined(USE_EFL)
int CertDatabase::ImportCACert(net::X509Certificate* cert) {
  if (!CERT_IsCACert(cert->os_cert_handle(), NULL))
    return ERR_IMPORT_CA_CERT_NOT_CA;

  if (cert->os_cert_handle()->isperm)
    return ERR_IMPORT_CERT_ALREADY_EXISTS;

  // CK_INVALID_HANDLE in case of CA cert
  CK_OBJECT_HANDLE key = CK_INVALID_HANDLE;
  // Get PersistentStorage for PK11
  crypto::ScopedPK11Slot slot(PK11_GetInternalKeySlot());

  std::string nickname = x509_util::GetDefaultUniqueNickname(
      cert->os_cert_handle(), CA_CERT, slot.get());

  {
    base::AutoLock lock(lock_);
    SECStatus rv;
    rv = PK11_ImportCert(slot.get(), cert->os_cert_handle(), key,
                         nickname.c_str(), PR_FALSE /*ignored*/);
    if (rv != SECSuccess) {
      LOG(ERROR) << "Couldn't import CA certificate: \""
          << nickname
          << "\", error code: " << PORT_GetError();
      return ERR_IMPORT_CA_CERT_FAILED;
    } else {
      // We have to set trust separately:
      CERTCertTrust trust
          = {CERTDB_VALID_CA, CERTDB_VALID_CA, CERTDB_VALID_CA};
      trust.sslFlags |= CERTDB_TRUSTED_CA | CERTDB_TRUSTED_CLIENT_CA;
      rv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(),
                                cert->os_cert_handle(), &trust);
      if (rv != SECSuccess) {
        LOG(ERROR) << "Couldn't set trust for CA certificate: \""
            << nickname
            << "\", error code: " << PORT_GetError();
        return ERR_IMPORT_CA_CERT_FAILED;
      }
    }
  }

  LOG(INFO) << "Imported CA certificate: \"" << nickname << "\"";

  return OK;
}
#endif

#if defined(OS_TIZEN_TV_PRODUCT)
int CertDatabase::CheckUserCert(X509Certificate* cert_obj) {
  if (!cert_obj)
    return ERR_CERT_INVALID;
  if (cert_obj->HasExpired())
    return ERR_CERT_DATE_INVALID;
  // Check if the private key corresponding to the certificate exist
  // We shouldn't accept any random client certificate sent by a CA.
  // Note: The NSS source documentation wrongly suggests that this
  // also imports the certificate if the private key exists. This
  // doesn't seem to be the case.
  CERTCertificate* cert = cert_obj->os_cert_handle();
  PK11SlotInfo* slot = PK11_KeyForCertExists(cert, NULL, NULL);
  if (!slot)
    return ERR_NO_PRIVATE_KEY_FOR_CERT;
  PK11_FreeSlot(slot);
  return OK;
}

int CertDatabase::AddUserCert(X509Certificate* cert_obj) {
  CERTCertificate* cert = cert_obj->os_cert_handle();
  CK_OBJECT_HANDLE key;
  crypto::ScopedPK11Slot slot(PK11_KeyForCertExists(cert, &key, NULL));
  if (!slot.get())
    return ERR_NO_PRIVATE_KEY_FOR_CERT;

  std::string nickname =
      x509_util::GetDefaultUniqueNickname(cert, USER_CERT, slot.get());

  // If an user cert was already imported in db,
  // some error was occured in a second attempt to import that user cert.
  // User cert that already imported doesn't need to import every time
  // because a db is reused.
  if (cert_obj->os_cert_handle()->isperm) {
    LOG(INFO) << "User cert already exists ("
              << PK11_GetSlotName(cert_obj->os_cert_handle()->slot) << "): \""
              << nickname << "\"";
    return OK;
  }

  SECStatus result_value =
      PK11_ImportCert(slot.get(), cert, key, nickname.c_str(), PR_FALSE);
  if (result_value != SECSuccess) {
    LOG(ERROR) << "Couldn't import user certificate. " << PORT_GetError();
    return ERR_ADD_USER_CERT_FAILED;
  }
  NotifyObserversCertDBChanged();
  return OK;
}

bool CertDatabase::ImportPrivateKey(const std::string& key_buffer) {
  std::vector<uint8> output;
  if (!crypto::ECPrivateKey::ConvertPEMtoDERFromPrivateKey(key_buffer,
                                                           &output)) {
    LOG(ERROR) << "Private Key Convert fail";
    return false;
  }

  SECItem der_private_key_info;
  SECStatus result_value;
  PK11SlotInfo* slot = PK11_GetInternalKeySlot();

  der_private_key_info.data = output.data();
  der_private_key_info.len = output.size();

  result_value = PK11_ImportDERPrivateKeyInfo(
      slot, &der_private_key_info, NULL, NULL, PR_TRUE, PR_TRUE, KU_ALL, NULL);

  if (result_value != SECSuccess) {
    LOG(ERROR) << "Couldn't import Private Key. " << PORT_GetError();
    return false;
  }

  LOG(INFO) << "Imported Private Key";
  return true;
}
#endif

}  // namespace net
