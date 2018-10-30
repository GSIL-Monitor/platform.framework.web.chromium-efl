// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_COMMON_TRUSTED_PEPPER_PLUGIN_INFO_CACHE_H_
#define EWK_EFL_INTEGRATION_COMMON_TRUSTED_PEPPER_PLUGIN_INFO_CACHE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/containers/hash_tables.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "content/public/common/pepper_plugin_info.h"
#include "url/gurl.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace pepper {

// Class holding (caching) discovered plugins in given directories.
// Plugin needs to have manifest file pmf which descibes it.
//
// See |ParsePepperPluginManifest| for Plugin Manifest File (pmf) specification.
class TrustedPepperPluginInfoCache {
 public:
  // Gets singleton of representing cache built for predefined directories.
  static TrustedPepperPluginInfoCache* GetInstance();

  // Special constructor only used during unit testing:
  explicit TrustedPepperPluginInfoCache(
      const std::vector<base::FilePath>& paths);

  ~TrustedPepperPluginInfoCache();

  // Adds all the plugins present in the cache to the |plugins| vector.
  //
  // If the plugins vector is empty then adds all plugins present currently
  // in the cache to the vector.
  //
  // If the plugins vector is not empty then adds only those plugins that
  // are present in the cache, but are not present in the vector.
  //
  // |plugins| is *non-null* pointer to vector which will be filled
  void GetPlugins(std::vector<content::PepperPluginInfo>* plugins) const;

  // Searches cached pepper plugins whether there is plugin which supports
  // given |mime_type| or |url|. If |mime_type| is empty, method tries to find
  // plugin basing on |url| extension.
  //
  // If plugin was found function returns true and sets |found_plugin|
  // information for plugin found. Otherwise function returns false and
  // doesn't touch |found_plugin| argument.
  //
  // |found_plugin| must be *non-null* pointer
  bool FindPlugin(std::string mime_type,
                  const GURL& url,
                  content::PepperPluginInfo* found_plugin) const;

 private:
  friend struct base::DefaultSingletonTraits<TrustedPepperPluginInfoCache>;

  // Builds plugin cache based on predefined directories.
  TrustedPepperPluginInfoCache();

  // Discover plugins in given directory by scanning it recursively
  // and adds their descriptions to the cache.
  void AddPluginsFromDirectory(const base::FilePath& dir);

  using PluginEntry =
      std::pair<base::FilePath, std::unique_ptr<content::PepperPluginInfo>>;

  // cached plugin infos
  std::vector<PluginEntry> plugins_;

  // PepperPluginInfo is non owning pointer to plugins element
  std::unordered_map<std::string, content::PepperPluginInfo*> mime_map_;

  DISALLOW_COPY_AND_ASSIGN(TrustedPepperPluginInfoCache);
};

// Exposed in header for UnitTesting:
//
// Parses Plugin Manifest File (PMF) provided as string |contents|
// and fills properly |out_info|.
//
// |pmf| is path from which PMF was loaded from and its used to fill
// |PepperPluginInfo::path| field if such information is not present in the PMF.
//
// This function does not performs any operation on filesystem.
//
// Sample PMFs:
//
// libsample_plugin.pmf:
// {
//   "type" : "pepper",
//   "name" : "Sample Trusted Plugin",
//   "version" : "1.0",
//   "description" : "Example Pepper Trusted Plugin",
//   "mimes" : [
//      {
//        "type" : "application/x-ppapi-example",
//        "description" : "Example Ppapi",
//        "extensions" : [ ".ext1", ".ext2" ]
//      },
//      {
//          "type" : "application/x-ppapi-example2"
//      }
//   ]
// }
//
// test_plugin.pmf:
// {
//   "type" : "pepper",
//   "name" : "Test Trusted Plugin",
//   "version" : "1.2",
//   "description" : "Test Trusted Pepper Plugin",
//   "mimes": [ { type: "application/x-ppapi-test" } ],
//   "program": { "url": "libtest_plugin.so" }
// }
//
// PMF is a JSON wich have following fields:
// - type (mandatory)
//     type of the plugin, must be set to "pepper'
// - name (optional)
//     plugin name, value for |PepperPluginInfo::name|
// - description (optional)
//     plugin description, value for |PepperPluginInfo::description|
// - version (optional)
//     plugin version, value for |PepperPluginInfo::version|
// - mimes (mandatory)
//     provides list of mime types, value for |PepperPluginInfo::mime_types|
// - mimes/type (mandatory)
//     defines mime_type itself, value for |WebPluginMimeType::mime_type|
// - mimes/description (optional)
//     defines mime_type description, value for |WebPluginMimeType::description|
// - mimes/extensions (optional)
//     defines list of the file extensions for this mime type,
//     value for |WebPluginMimeType::file_extensions|
// - program (optional)
//     provides path for pepper plugin shared library, value for
//     |PepperPluginInfo::path|. If value is not present in the manifest,
//     then path is deduced from |pmf|, by replacing .pmf extension form the
//     manifest to platform dependent library extension (.so on POSIX systems).
// - program/url (mandatory if program argument is present)
//     provides relative or absolute path to pepper plugin shared library.
//
// Following fields are not present in PMF and are set to given values:
// - PepperPluginInfo::is_internal is set to |false|
// - PepperPluginInfo::is_out_of_process is set to |true|
//
// Function returns true if manifest is parsed and follows specification
// given above.
bool ParsePepperPluginManifest(const base::FilePath& pmf,
                               const std::string& contents,
                               content::PepperPluginInfo* out_info);

// Exposed in header for UnitTesting:
//
// Reads plugin manifest from |pmf| path, parses it and validates
// whether plugin path (|PepperPluginInfo::path|) is valid (exsists).
//
// See |ParsePepperPluginManifest| for Plugin Manifest File (pmf) specification.
bool FillPepperPluginInfoFromManifest(const base::FilePath& pmf,
                                      content::PepperPluginInfo* out_info);

}  // namespace pepper

#endif  // EWK_EFL_INTEGRATION_COMMON_TRUSTED_PEPPER_PLUGIN_INFO_CACHE_H_
