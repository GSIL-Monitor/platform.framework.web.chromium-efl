// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/constants.h"

namespace extensions {

const char kExtensionScheme[] = "chrome-extension";

const base::FilePath::CharType kManifestFilename[] =
    FILE_PATH_LITERAL("manifest.json");
const base::FilePath::CharType kLocaleFolder[] =
    FILE_PATH_LITERAL("_locales");
const base::FilePath::CharType kMessagesFilename[] =
    FILE_PATH_LITERAL("messages.json");
const base::FilePath::CharType kPlatformSpecificFolder[] =
    FILE_PATH_LITERAL("_platform_specific");
const base::FilePath::CharType kMetadataFolder[] =
    FILE_PATH_LITERAL("_metadata");
const base::FilePath::CharType kVerifiedContentsFilename[] =
    FILE_PATH_LITERAL("verified_contents.json");
const base::FilePath::CharType kComputedHashesFilename[] =
    FILE_PATH_LITERAL("computed_hashes.json");
const base::FilePath::CharType kIndexedRulesetFilename[] =
    FILE_PATH_LITERAL("generated_indexed_ruleset");

const char kInstallDirectoryName[] = "Extensions";

const char kTempExtensionName[] = "CRX_INSTALL";

const char kDecodedImagesFilename[] = "DECODED_IMAGES";

const char kDecodedMessageCatalogsFilename[] = "DECODED_MESSAGE_CATALOGS";

const char kGeneratedBackgroundPageFilename[] =
    "_generated_background_page.html";

const char kModulesDir[] = "_modules";

const base::FilePath::CharType kExtensionFileExtension[] =
    FILE_PATH_LITERAL(".crx");
const base::FilePath::CharType kExtensionKeyFileExtension[] =
    FILE_PATH_LITERAL(".pem");

// If auto-updates are turned on, default to running every 5 hours.
const int kDefaultUpdateFrequencySeconds = 60 * 60 * 5;

const char kLocalAppSettingsDirectoryName[] = "Local App Settings";
const char kLocalExtensionSettingsDirectoryName[] = "Local Extension Settings";
const char kSyncAppSettingsDirectoryName[] = "Sync App Settings";
const char kSyncExtensionSettingsDirectoryName[] = "Sync Extension Settings";
const char kManagedSettingsDirectoryName[] = "Managed Extension Settings";
const char kStateStoreName[] = "Extension State";
const char kRulesStoreName[] = "Extension Rules";
const char kWebStoreAppId[] = "ahfgeienlihckogmohjhadlkjgocpleb";

const uint8_t kWebstoreSignaturesPublicKey[] = {
    0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
    0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
    0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0x8f, 0xfb, 0xbf,
    0x5c, 0x37, 0x63, 0x94, 0x3c, 0xb0, 0xee, 0x01, 0xc4, 0xb5, 0xa6, 0x9a,
    0xb1, 0x9f, 0x46, 0x74, 0x6f, 0x16, 0x38, 0xa0, 0x32, 0x27, 0x35, 0xdd,
    0xf0, 0x71, 0x6b, 0x0e, 0xdc, 0xf6, 0x25, 0xcb, 0xb2, 0xed, 0xea, 0xfb,
    0x32, 0xd5, 0xaf, 0x1e, 0x03, 0x43, 0x03, 0x46, 0xf0, 0xa7, 0x39, 0xdb,
    0x23, 0x96, 0x1d, 0x65, 0xe5, 0x78, 0x51, 0xf0, 0x84, 0xb0, 0x0e, 0x12,
    0xac, 0x0e, 0x5b, 0xdc, 0xc9, 0xd6, 0x4c, 0x7c, 0x00, 0xd5, 0xb8, 0x1b,
    0x88, 0x33, 0x3e, 0x2f, 0xda, 0xeb, 0xaa, 0xf7, 0x1a, 0x75, 0xc2, 0xae,
    0x3a, 0x54, 0xde, 0x37, 0x8f, 0x10, 0xd2, 0x28, 0xe6, 0x84, 0x79, 0x4d,
    0x15, 0xb4, 0xf3, 0xbd, 0x3f, 0x56, 0xd3, 0x3c, 0x3f, 0x18, 0xab, 0xfc,
    0x2e, 0x05, 0xc0, 0x1e, 0x08, 0x31, 0xb6, 0x61, 0xd0, 0xfd, 0x9f, 0x4f,
    0x3f, 0x64, 0x0d, 0x17, 0x93, 0xbc, 0xad, 0x41, 0xc7, 0x48, 0xbe, 0x00,
    0x27, 0xa8, 0x4d, 0x70, 0x42, 0x92, 0x05, 0x54, 0xa6, 0x6d, 0xb8, 0xde,
    0x56, 0x6e, 0x20, 0x49, 0x70, 0xee, 0x10, 0x3e, 0x6b, 0xd2, 0x7c, 0x31,
    0xbd, 0x1b, 0x6e, 0xa4, 0x3c, 0x46, 0x62, 0x9f, 0x08, 0x66, 0x93, 0xf9,
    0x2a, 0x51, 0x31, 0xa8, 0xdb, 0xb5, 0x9d, 0xb9, 0x0f, 0x73, 0xe8, 0xa0,
    0x09, 0x32, 0x01, 0xe9, 0x7b, 0x2a, 0x8a, 0x36, 0xa0, 0xcf, 0x17, 0xb0,
    0x50, 0x70, 0x9d, 0xa2, 0xf9, 0xa4, 0x6f, 0x62, 0x4d, 0xb6, 0xc9, 0x31,
    0xfc, 0xf3, 0x08, 0x12, 0xff, 0x93, 0xbd, 0x62, 0x31, 0xd8, 0x1c, 0xea,
    0x1a, 0x9e, 0xf5, 0x81, 0x28, 0x7f, 0x75, 0x5e, 0xd2, 0x27, 0x7a, 0xc2,
    0x96, 0xf5, 0x9d, 0xdb, 0x18, 0xfc, 0x76, 0xdc, 0x46, 0xf0, 0x57, 0xc0,
    0x58, 0x34, 0xc8, 0x22, 0x2d, 0x2a, 0x65, 0x75, 0xa7, 0xd9, 0x08, 0x62,
    0xcd, 0x02, 0x03, 0x01, 0x00, 0x01};

const size_t kWebstoreSignaturesPublicKeySize =
    arraysize(kWebstoreSignaturesPublicKey);

const int kMainThreadId = 0;

const char kMimeTypeJpeg[] = "image/jpeg";
const char kMimeTypePng[] = "image/png";

const int64_t kInvalidServiceWorkerVersionId = -1;

}  // namespace extensions

namespace extension_misc {

const char kFeedbackExtensionId[] = "gfdkimpbcpahaombhbimeihdjnejgicl";
const char kPdfExtensionId[] = "mhjfbmdgcfjbbpaeojofohoefgiehjai";
const char kQuickOfficeComponentExtensionId[] =
    "bpmcpldpdmajfigpchkicefoigmkfalc";
const char kQuickOfficeInternalExtensionId[] =
    "ehibbfinohgbchlgdbfpikodjaojhccn";
const char kQuickOfficeExtensionId[] = "gbkeegbaiigmenfmjfclcdgdpimamgkj";
const char kMimeHandlerPrivateTestExtensionId[] =
    "oickdpebdnfbgkcaoklfcdhjniefkcji";

const char kProdHangoutsExtensionId[] = "nckgahadagoaajjgafhacjanaoiihapd";
const char* const kHangoutsExtensionIds[6] = {
    kProdHangoutsExtensionId,
    "ljclpkphhpbpinifbeabbhlfddcpfdde",  // Debug.
    "ppleadejekpmccmnpjdimmlfljlkdfej",  // Alpha.
    "eggnbpckecmjlblplehfpjjdhhidfdoj",  // Beta.
    "jfjjdfefebklmdbmenmlehlopoocnoeh",  // Packaged App Debug.
    "knipolnnllmklapflnccelgolnpehhpl"   // Packaged App Prod.
    // Keep in sync with _api_features.json and _manifest_features.json.
};

// Error returned when scripting of a page is denied due to enterprise policy.
const char kPolicyBlockedScripting[] =
    "This page cannot be scripted due to an ExtensionsSettings policy.";

}  // namespace extension_misc
