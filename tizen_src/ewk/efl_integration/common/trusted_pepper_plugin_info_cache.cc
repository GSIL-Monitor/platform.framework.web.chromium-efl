// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/trusted_pepper_plugin_info_cache.h"

#include <iterator>
#include <unordered_set>

#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "common/content_switches_efl.h"
#include "content/public/common/pepper_plugin_info.h"
#include "net/base/mime_util.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "url/gurl.h"

#define PARSING_ERROR(pmf) \
  LOG(WARNING) << "Malformed PMF: " << pmf.LossyDisplayName() << ": "
using base::FilePath;
using content::PepperPluginInfo;
using content::WebPluginMimeType;

namespace pepper {

namespace {

// TODO(a.bujalski, d.sura):
// Path for TrustedPepperPlugins providing TV specific WebAPIs.
// Currently we don`t have tizen environment which defines TrustedPepperPlugins
// paths ("/opt/usr/apps/pepper" and "/usr/lib/pepper"), thus we use hardcoded
// values. We should use proper environment variable as soon as specific
// variable is added for TrustedPepperPlugins.
// Our environment variable should be registered in Tizen3.0 Multi-user
// Platform Metadata. See:
// https://wiki.tizen.org/wiki/Multi-user_Platform_Metadata
const FilePath::CharType* kPepperPluginsPaths[] = {
    // "/opt/usr/apps/pepper" is mount point for squash-fs archive contating
    // Pepper Plugins providing implementation for TV specific WebAPIs.
    // This archive is bundled together with Tizen TV image. Additionally
    // this archive can be updated without full platform update.
    // Updates are downloaded from Samsung servers.
    FILE_PATH_LITERAL("/opt/usr/apps/pepper"),
    FILE_PATH_LITERAL("/usr/lib/pepper")};

const FilePath::CharType kPepperPluginManifestExtension[] =
    FILE_PATH_LITERAL(".pmf");
const FilePath::CharType kPepperPluginExtension[] = FILE_PATH_LITERAL(".so");
const char kPepperManifestNameKey[] = "name";
const char kPepperManifestProgramKey[] = "program";
const char kPepperManifestUrlKey[] = "url";
const char kPepperManifestVersionKey[] = "version";
const char kPepperManifestDescriptionKey[] = "description";
const char kPepperManifestTypeKey[] = "type";
const char kPepperManifestTypePepper[] = "pepper";
const char kPepperManifestMimesKey[] = "mimes";
const char kPepperManifestMimesTypeKey[] = "type";
const char kPepperManifestMimesDescriptionKey[] = "description";
const char kPepperManifestMimesExtensionsKey[] = "extensions";

using Hash = BASE_HASH_NAMESPACE::hash<FilePath>;
using FilesContainer = std::unordered_set<FilePath, Hash>;

bool GetStringValue(const base::DictionaryValue* dict,
                    const std::string& key,
                    std::string* out_value) {
  std::string val;
  if (dict->GetStringWithoutPathExpansion(key, &val) && !val.empty()) {
    *out_value = val;
    return true;
  }

  return false;
}

bool GetStringValue(const base::ListValue* list,
                    size_t i,
                    std::string* out_value) {
  std::string val;
  if (list->GetString(i, &val) && !val.empty()) {
    *out_value = val;
    return true;
  }

  return false;
}

bool GetListValue(const base::DictionaryValue* dict,
                  const std::string& key,
                  const base::ListValue** out_value) {
  const base::ListValue* val = nullptr;
  if (dict->GetListWithoutPathExpansion(key, &val) && val) {
    *out_value = val;
    return true;
  }

  return false;
}

bool GetDictionaryValue(const base::DictionaryValue* dict,
                        const std::string& key,
                        const base::DictionaryValue** out_value) {
  const base::DictionaryValue* val = nullptr;
  if (dict->GetDictionaryWithoutPathExpansion(key, &val) && val) {
    *out_value = val;
    return true;
  }

  return false;
}

bool GetDictionaryValue(const base::ListValue* list,
                        size_t i,
                        const base::DictionaryValue** out_value) {
  const base::DictionaryValue* val = nullptr;
  if (list->GetDictionary(i, &val) && val) {
    *out_value = val;
    return true;
  }

  return false;
}

void GetMimeTypeExtensions(const base::DictionaryValue* dict,
                           std::vector<std::string>* out_exensions) {
  const base::ListValue* list;
  std::string val;
  if (GetListValue(dict, kPepperManifestMimesExtensionsKey, &list)) {
    for (size_t i = 0; i < list->GetSize(); ++i)
      if (GetStringValue(list, i, &val))
        out_exensions->push_back(val);
  } else if (GetStringValue(dict, kPepperManifestMimesExtensionsKey, &val)) {
    out_exensions->push_back(val);
  }
}

bool GetWebPluginMimeType(const FilePath& pmf,
                          const base::ListValue* mimes,
                          size_t i,
                          WebPluginMimeType* plugin_mime) {
  std::string type;
  std::string description;
  std::vector<std::string> extensions;

  const base::DictionaryValue* dict = nullptr;
  if (GetDictionaryValue(mimes, i, &dict)) {
    // "type" : (mandatory)
    if (!GetStringValue(dict, kPepperManifestMimesTypeKey, &type)) {
      PARSING_ERROR(pmf) << "MIME type attribute not found";
      return false;
    }

    GetStringValue(dict, kPepperManifestMimesDescriptionKey, &description);
    GetMimeTypeExtensions(dict, &extensions);
  } else {  // The MIME list item is not a dictionary.
    // We allow the MIME to be just a string type,
    // without the description and extensions.
    if (GetStringValue(mimes, i, &type)) {
      PARSING_ERROR(pmf) << "The MIME has no type entry";
      return false;
    }
  }

  *plugin_mime = WebPluginMimeType(type, "", description);
  plugin_mime->file_extensions = extensions;
  return true;
}

bool GetPuginMimeTypes(const FilePath& pmf,
                       const base::ListValue* mimes_list,
                       std::vector<WebPluginMimeType>* mime_types) {
  if (!mimes_list->IsType(base::Value::Type::LIST)) {
    PARSING_ERROR(pmf) << "The mimes must be a list";
    return false;
  }

  for (size_t i = 0; i < mimes_list->GetSize(); ++i) {
    WebPluginMimeType mime_type;
    if (GetWebPluginMimeType(pmf, mimes_list, i, &mime_type))
      mime_types->push_back(mime_type);
  }

  return true;
}

bool ReadPluginMimeTypes(const FilePath& pmf,
                         const base::DictionaryValue* dict,
                         PepperPluginInfo* info) {
  // Get the MIME types.
  const base::ListValue* mimes_list = nullptr;
  if (!GetListValue(dict, kPepperManifestMimesKey, &mimes_list)) {
    PARSING_ERROR(pmf) << "Can't read the MIME types list";
    return false;
  }

  std::vector<WebPluginMimeType> mime_types;
  if (!GetPuginMimeTypes(pmf, mimes_list, &mime_types)) {
    PARSING_ERROR(pmf) << "Can't parse MIME types";
    return false;
  }

  if (mime_types.empty()) {
    PARSING_ERROR(pmf) << " no mime types defined for the plugin";
    return false;
  }

  info->mime_types = mime_types;
  return true;
}

bool ReadPluginPath(const FilePath& pmf,
                    const base::DictionaryValue* dict,
                    PepperPluginInfo* info) {
  // Path to the plugin. The url attribute has higher priority
  // than the manifest name.
  const base::DictionaryValue* program_dict;
  if (!GetDictionaryValue(dict, kPepperManifestProgramKey, &program_dict)) {
    if (dict->HasKey(kPepperManifestProgramKey)) {
      PARSING_ERROR(pmf) << "invalid \"program\" key present:"
                         << " not a dictionary";
      return false;
    }

    // No program key present.
    // Path to manifest is:     /pepper/plugins_dir/my_plugin.pmf
    // Path to plugin will be:  /pepper/plugins_dir/my_plugin.so
    info->path = pmf.RemoveExtension().AddExtension(kPepperPluginExtension);
    return true;
  }

  std::string val;
  if (GetStringValue(program_dict, kPepperManifestUrlKey, &val)) {
    FilePath plugin_path(val);
    if (plugin_path.IsAbsolute())
      info->path = plugin_path;
    else
      info->path = pmf.DirName().Append(val);
  } else {
    PARSING_ERROR(pmf) << "\"program\" key present but it does not contain"
                          " \"url\" entry";
    return false;
  }

  return true;
}

bool ValidatePluginType(const FilePath& pmf,
                        const base::DictionaryValue* dict) {
  std::string val;
  if (!GetStringValue(dict, kPepperManifestTypeKey, &val)) {
    PARSING_ERROR(pmf) << "No type attribute in the PMF file";
    return false;
  }

  if (val != kPepperManifestTypePepper) {
    PARSING_ERROR(pmf) << "Unknown plugin type: " << val;
    return false;
  }

  return true;
}

bool ValidatePepperPluginInfo(const FilePath& pmf,
                              const PepperPluginInfo& info) {
  if (!PathExists(info.path)) {
    PARSING_ERROR(pmf) << " invalid plugin path: "
                       << info.path.LossyDisplayName();
    return false;
  }

  return true;
}

}  // namespace

TrustedPepperPluginInfoCache* TrustedPepperPluginInfoCache::GetInstance() {
  return base::Singleton<TrustedPepperPluginInfoCache>::get();
}

TrustedPepperPluginInfoCache::TrustedPepperPluginInfoCache(
    const std::vector<base::FilePath>& paths) {
  for (const auto& path : paths)
    AddPluginsFromDirectory(path);
}

TrustedPepperPluginInfoCache::~TrustedPepperPluginInfoCache() {}

void TrustedPepperPluginInfoCache::GetPlugins(
    std::vector<PepperPluginInfo>* plugins) const {
  if (plugins->empty()) {
    plugins->reserve(plugins_.size());
    std::for_each(plugins_.cbegin(), plugins_.cend(),
                  [plugins](const PluginEntry& element) {
                    plugins->push_back(*(element.second));
                  });
  } else {
    FilesContainer plugins_not_to_add;
    for (const auto& plugin : *plugins)
      plugins_not_to_add.insert(plugin.path);

    for (const auto& plugin : plugins_) {
      if (!plugins_not_to_add.count(plugin.second->path))
        plugins->push_back(*(plugin.second));
    }
  }
}

bool TrustedPepperPluginInfoCache::FindPlugin(
    std::string mime_type,
    const GURL& url,
    PepperPluginInfo* found_plugin) const {
  if (mime_type.empty()) {
    // Try to guess the MIME type based on the extension.
    auto filename = url.ExtractFileName();
    auto extension_pos = filename.rfind('.');
    if (extension_pos != std::string::npos) {
      auto extension = filename.substr(extension_pos + 1);
      net::GetWellKnownMimeTypeFromExtension(extension, &mime_type);
    }

    if (mime_type.empty())
      return false;
  }

  auto it = mime_map_.find(mime_type);
  if (it != mime_map_.cend()) {
    *found_plugin = *(it->second);
    return true;
  }

  return false;
}

TrustedPepperPluginInfoCache::TrustedPepperPluginInfoCache() {
  for (const auto& path : kPepperPluginsPaths)
    AddPluginsFromDirectory(base::FilePath(path));

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kTrustedPepperPluginsSearchPaths)) {
    const std::string value = command_line->GetSwitchValueASCII(
        switches::kTrustedPepperPluginsSearchPaths);
    std::vector<std::string> paths = base::SplitString(
        value, ";", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    for (const auto& path : paths)
      AddPluginsFromDirectory(base::FilePath(path));
  }
}

void TrustedPepperPluginInfoCache::AddPluginsFromDirectory(
    const FilePath& dir) {
  if (!base::PathExists(dir))
    return;

  base::FileEnumerator it(dir, true, base::FileEnumerator::FILES);
  for (FilePath name = it.Next(); !name.empty(); name = it.Next()) {
    if (name.Extension() != kPepperPluginManifestExtension)
      continue;

    std::unique_ptr<PepperPluginInfo> info{new PepperPluginInfo};
    if (!FillPepperPluginInfoFromManifest(name, info.get()))
      continue;

    for (const auto& plugin_mime : info->mime_types) {
      if (mime_map_.count(plugin_mime.mime_type)) {
        LOG(WARNING)
            << "mime_type: " << plugin_mime.mime_type
            << " is already handled by plugin: "
            << mime_map_[plugin_mime.mime_type]->path.LossyDisplayName();
        continue;
      }

      mime_map_[plugin_mime.mime_type] = info.get();
    }

    plugins_.emplace_back(name, std::move(info));
  }
}

bool ParsePepperPluginManifest(const FilePath& pmf,
                               const std::string& contents,
                               PepperPluginInfo* out_info) {
  base::JSONReader reader;
  std::unique_ptr<base::Value> root = reader.ReadToValue(contents);
  if (!root) {
    PARSING_ERROR(pmf) << "Ill formatted PMF file: not a proper JSON: "
                       << reader.GetErrorMessage();
    return false;
  }

  if (!root->IsType(base::Value::Type::DICTIONARY)) {
    PARSING_ERROR(pmf) << "Ill formatted PMF file: "
                       << "the root node is not an object.";
    return false;
  }

  base::DictionaryValue* dict = 0;
  if (!root->GetAsDictionary(&dict) || !dict) {
    PARSING_ERROR(pmf) << "Can't to get the root node as an object.";
    return false;
  }

  if (!ValidatePluginType(pmf, dict))
    return false;

  // Let's set the default values for the info in case they are not present
  // in the manifest.
  PepperPluginInfo info;
  info.is_out_of_process = true;
  info.is_internal = false;
  // Trusted Pepper Plugins have all possible permissions
  info.permissions = ppapi::Permission::PERMISSION_ALL_BITS;

  GetStringValue(dict, kPepperManifestNameKey, &(info.name));
  GetStringValue(dict, kPepperManifestDescriptionKey, &(info.description));
  GetStringValue(dict, kPepperManifestVersionKey, &(info.version));

  if (!ReadPluginMimeTypes(pmf, dict, &info))
    return false;

  if (!ReadPluginPath(pmf, dict, &info))
    return false;

  *out_info = info;
  return true;
}

bool FillPepperPluginInfoFromManifest(const FilePath& pmf,
                                      PepperPluginInfo* out_info) {
  std::string contents;
  if (!base::ReadFileToString(pmf, &contents)) {
    PARSING_ERROR(pmf) << "Could not read manifest file ";
    return false;
  }

  PepperPluginInfo info;
  if (!ParsePepperPluginManifest(pmf, contents, &info))
    return false;

  if (!ValidatePepperPluginInfo(pmf, info))
    return false;

  *out_info = info;
  return true;
}

}  // namespace pepper
