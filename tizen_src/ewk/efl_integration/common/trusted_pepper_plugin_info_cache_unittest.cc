// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/trusted_pepper_plugin_info_cache.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::FilePath;
using content::PepperPluginInfo;
using content::WebPluginMimeType;

namespace pepper {

namespace {

enum PluginFilesWriteOptions { PMF_ONLY, PMF_AND_LIBRARY };

const std::vector<std::string> kNonExistingMimes = {
    "application/x-invalid-mime", "invalid/invalid",
    "test/x-test-invalid-mime"};

FilePath AddLibrarySuffix(const FilePath& path) {
  return path.AddExtension(FILE_PATH_LITERAL(".so"));
}

bool EqualPluginMimeType(const WebPluginMimeType& lhs,
                         const WebPluginMimeType& rhs);

template <class T>
bool EqualVectors(const std::vector<T>& lhs, const std::vector<T>& rhs) {
  if (lhs.size() != rhs.size())
    return false;

  return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

bool EqualVectors(const std::vector<WebPluginMimeType>& lhs,
                  const std::vector<WebPluginMimeType>& rhs) {
  if (lhs.size() != rhs.size())
    return false;

  return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin(),
                    EqualPluginMimeType);
}

bool EqualPluginMimeType(const WebPluginMimeType& lhs,
                         const WebPluginMimeType& rhs) {
  return lhs.mime_type == rhs.mime_type &&
         EqualVectors(lhs.file_extensions, rhs.file_extensions) &&
         lhs.description == rhs.description &&
         EqualVectors(lhs.additional_param_names, rhs.additional_param_names) &&
         EqualVectors(lhs.additional_param_values, rhs.additional_param_values);
}

bool EqualPepperPluginInfo(const PepperPluginInfo& lhs,
                           const PepperPluginInfo& rhs) {
  return lhs.is_internal == rhs.is_internal &&
         lhs.is_out_of_process == rhs.is_out_of_process &&
         lhs.path == rhs.path && lhs.name == rhs.name &&
         lhs.description == rhs.description && lhs.version == rhs.version &&
         EqualVectors(lhs.mime_types, rhs.mime_types) &&
         lhs.permissions == rhs.permissions;
}

PepperPluginInfo GetTestPlugin1() {
  PepperPluginInfo plugin;

  plugin.is_internal = false;
  plugin.is_out_of_process = true;
  plugin.name = "plugin1";
  plugin.description = "Test Plugin1";
  plugin.version = "1.0";
  plugin.mime_types.emplace_back("plugin1/mime1", ".ext1", "mime1");
  plugin.mime_types.emplace_back("plugin1/mime2", ".ext2", "mime2");
  plugin.permissions = ppapi::Permission::PERMISSION_ALL_BITS;

  return plugin;
}

PepperPluginInfo GetTestPlugin2() {
  WebPluginMimeType plugin_mime;
  plugin_mime.mime_type = "plugin2/mime";

  PepperPluginInfo plugin;
  plugin.is_internal = false;
  plugin.is_out_of_process = true;
  plugin.name = "plugin2";
  plugin.description = "Test Plugin2";
  plugin.version = "1.1";
  plugin.mime_types.push_back(plugin_mime);
  plugin.permissions = ppapi::Permission::PERMISSION_ALL_BITS;

  return plugin;
}

PepperPluginInfo GetTestPlugin3() {
  PepperPluginInfo plugin;

  plugin.is_internal = false;
  plugin.is_out_of_process = true;
  plugin.name = "plugin3";
  plugin.description = "Test Plugin3";
  plugin.version = "1.0";
  plugin.mime_types.emplace_back("plugin3/mime1", ".e1", "mime1");
  plugin.mime_types.emplace_back("plugin3/mime2", ".e2", "mime2");
  plugin.permissions = ppapi::Permission::PERMISSION_ALL_BITS;

  return plugin;
}

PepperPluginInfo GetTestPlugin4() {
  PepperPluginInfo plugin;

  plugin.is_internal = false;
  plugin.is_out_of_process = true;
  plugin.name = "plugin4";
  plugin.description = "Test Plugin4";
  plugin.version = "4.0";
  plugin.mime_types.emplace_back("plugin4/mime1", ".f1", "mime1");
  plugin.mime_types.emplace_back("plugin4/mime2", ".f2", "mime2");
  plugin.permissions = ppapi::Permission::PERMISSION_ALL_BITS;

  return plugin;
}

PepperPluginInfo GetTestPlugin5() {
  PepperPluginInfo plugin;

  plugin.is_internal = false;
  plugin.is_out_of_process = true;
  plugin.name = "plugin5";
  plugin.description = "Test Plugin5";
  plugin.version = "1.10";
  plugin.mime_types.emplace_back("plugin5/mime1", ".p1", "mime1");
  plugin.mime_types.emplace_back("plugin5/mime2", ".p2", "mime2");
  plugin.permissions = ppapi::Permission::PERMISSION_ALL_BITS;

  return plugin;
}

PepperPluginInfo GetTestPlugin6() {
  PepperPluginInfo plugin;

  plugin.is_internal = false;
  plugin.is_out_of_process = true;
  plugin.name = "plugin6";
  plugin.description = "Test Plugin6";
  plugin.version = "1.10";
  plugin.mime_types.emplace_back("application/x-shockwave-flash", ".swf",
                                 "mime1");
  plugin.permissions = ppapi::Permission::PERMISSION_ALL_BITS;

  return plugin;
}

std::string PluginMimesToJSON(const PepperPluginInfo& plugin) {
  std::ostringstream oss;

  for (size_t i = 0; i < plugin.mime_types.size(); ++i) {
    if (i > 0)
      oss << ",\n";

    const auto& mime = plugin.mime_types[i];
    oss << "    {"
        << "       \"type\" : \"" << mime.mime_type << "\"";

    if (!mime.description.empty()) {
      oss << ",\n"
          << "       \"description\" : \"" << mime.description << "\"";
    }

    if (!mime.file_extensions.empty()) {
      oss << ",\n"
          << "       \"extensions\" : [ ";

      for (size_t j = 0; j < mime.file_extensions.size(); ++j) {
        if (j > 0)
          oss << ", ";

        oss << "\"" << mime.file_extensions[j] << "\"";
      }

      oss << " ]";
    }

    oss << "    }";
  }

  return oss.str();
}

std::string GeneratePMF(const PepperPluginInfo& plugin) {
  std::ostringstream pmf_stream;
  pmf_stream << "{"
             << "  \"type\": \"pepper\",\n"
             << "  \"name\": \"" << plugin.name << "\",\n"
             << "  \"description\": \"" << plugin.description << "\",\n"
             << "  \"version\": \"" << plugin.version << "\",\n"
             << "  \"mimes\": [\n"
             << PluginMimesToJSON(plugin) << "\n"
             << "  ],\n"
             << "  \"program\": { \n"
             << "    \"url\": \"" << plugin.path.AsUTF8Unsafe() << "\"\n"
             << "  }\n"
             << "}";

  return pmf_stream.str();
}

// uses |PepperPluginInfo.name| as file paths.
void WritePluginFiles(const FilePath& plugin_dir,
                      PluginFilesWriteOptions files_to_write,
                      PepperPluginInfo* plugin) {
  FilePath base_path = plugin_dir.AppendASCII(plugin->name);
  plugin->path = AddLibrarySuffix(base_path);

  std::string pmf = GeneratePMF(*plugin);
  FilePath pmf_path = base_path.AddExtension(FILE_PATH_LITERAL(".pmf"));

  PepperPluginInfo info;
  ASSERT_TRUE(ParsePepperPluginManifest(pmf_path, pmf, &info));
  ASSERT_TRUE(EqualPepperPluginInfo(*plugin, info));

  int data_written = base::WriteFile(pmf_path, pmf.c_str(), pmf.size());
  ASSERT_GE(data_written, 0);
  ASSERT_EQ(pmf.size(), static_cast<size_t>(data_written));

  if (files_to_write == PMF_AND_LIBRARY) {
    std::string plugin_lib = plugin->name;
    int plugin_data_written =
        base::WriteFile(plugin->path, plugin_lib.c_str(), plugin_lib.size());
    ASSERT_GE(plugin_data_written, 0);
    ASSERT_EQ(plugin_lib.size(), static_cast<size_t>(plugin_data_written));
  }

  bool fill_plugin_res = FillPepperPluginInfoFromManifest(pmf_path, &info);
  if (files_to_write == PMF_AND_LIBRARY) {
    ASSERT_TRUE(fill_plugin_res);
    ASSERT_TRUE(EqualPepperPluginInfo(*plugin, info));
  } else {
    ASSERT_FALSE(fill_plugin_res);
  }
}

class TrustedPepperPluginInfoCacheTest : public testing::Test {
 public:
  TrustedPepperPluginInfoCacheTest() {}

  void SetUp() override {
    InitializePluginsDir();
    InitializeEmptyDir();
    cache_.reset(new TrustedPepperPluginInfoCache(dir_paths_));
    ASSERT_NE(nullptr, cache_.get());
  }

 protected:
  // Valid plugins added to temp dir, should be present in cache
  std::vector<PepperPluginInfo> valid_plugins_;

  // Plugins with invalid manifests which shouldn't be added
  // to the cache
  std::vector<PepperPluginInfo> invalid_plugins_;

  std::unique_ptr<TrustedPepperPluginInfoCache> cache_;

 private:
  void CreatePluginInDir(const FilePath& dir,
                         PluginFilesWriteOptions valid,
                         PepperPluginInfo plugin) {
    WritePluginFiles(dir, valid, &plugin);
    if (valid == PMF_AND_LIBRARY)
      valid_plugins_.push_back(plugin);
    else
      invalid_plugins_.push_back(plugin);
  }

  void InitializePluginsDir() {
    ASSERT_TRUE(plugins_dir_.CreateUniqueTempDir());
    dir_paths_.push_back(plugins_dir_.GetPath());

    FilePath dir1;
    ASSERT_TRUE(
        CreateTemporaryDirInDir(plugins_dir_.GetPath(), "plugins1_", &dir1));

    CreatePluginInDir(dir1, PMF_AND_LIBRARY, GetTestPlugin1());
    CreatePluginInDir(dir1, PMF_AND_LIBRARY, GetTestPlugin2());

    FilePath dir2;
    ASSERT_TRUE(
        CreateTemporaryDirInDir(plugins_dir_.GetPath(), "plugins2_", &dir2));

    CreatePluginInDir(dir2, PMF_AND_LIBRARY, GetTestPlugin3());
    CreatePluginInDir(dir2, PMF_ONLY, GetTestPlugin4());

    FilePath dir3;
    ASSERT_TRUE(
        CreateTemporaryDirInDir(plugins_dir_.GetPath(), "plugins3_", &dir3));
    CreatePluginInDir(dir3, PMF_ONLY, GetTestPlugin5());
    CreatePluginInDir(dir3, PMF_AND_LIBRARY, GetTestPlugin6());
  }

  void InitializeEmptyDir() {
    ASSERT_TRUE(empty_dir_.CreateUniqueTempDir());
    dir_paths_.push_back(empty_dir_.GetPath());
  }

  base::ScopedTempDir plugins_dir_;
  base::ScopedTempDir empty_dir_;
  std::vector<base::FilePath> dir_paths_;
};

TEST(ParsePepperPluginManifestTest, MinimalManifest) {
  std::string pmfs[] = {
      "{"
      "  \"type\": \"pepper\","
      "  \"mimes\": ["
      "    { \"type\": \"application/x-ppapi-minimal-manifest\" }"
      "  ]"
      "}",

      "{"
      "  \"not-specified-key\": \"some value\","
      "  \"type\": \"pepper\","
      "  \"mimes\": ["
      "    { \"type\": \"application/x-ppapi-minimal-manifest\" }"
      "  ],"
      "  \"excessive-key\": \"excessive-value\""
      "}"};

  for (const auto& pmf : pmfs) {
    LOG(INFO) << "Parsing pmf: " << pmf;
    FilePath path(FILE_PATH_LITERAL("minimal_test.pmf"));

    PepperPluginInfo info;
    ASSERT_TRUE(ParsePepperPluginManifest(path, pmf, &info));

    EXPECT_FALSE(info.is_internal);
    EXPECT_TRUE(info.is_out_of_process);
    EXPECT_EQ(info.permissions, ppapi::Permission::PERMISSION_ALL_BITS);
    EXPECT_TRUE(info.name.empty());
    EXPECT_TRUE(info.description.empty());
    EXPECT_TRUE(info.version.empty());

    ASSERT_EQ(1u, info.mime_types.size());
    EXPECT_EQ("application/x-ppapi-minimal-manifest",
              info.mime_types[0].mime_type);
    EXPECT_TRUE(info.mime_types[0].file_extensions.empty());

    FilePath expected_path(AddLibrarySuffix(path.RemoveExtension()));
    EXPECT_EQ(expected_path, info.path);
  }
}

TEST(ParsePepperPluginManifestTest, EmptyManifest) {
  std::string pmfs[] = {"", "{}"};
  FilePath path(FILE_PATH_LITERAL("empty_test.pmf"));

  for (const auto& pmf : pmfs) {
    PepperPluginInfo info;
    EXPECT_FALSE(ParsePepperPluginManifest(path, pmf, &info));
  }
}

TEST(ParsePepperPluginManifestTest, BadPluginType) {
  std::string pmfs[] = {
      // missing type
      "{"
      "   \"mimes\": [ { \"type\": \"application/x-ppapi-bad-type\" } ]"
      "}",

      "{"
      "   \"type\": \"bad-type\","
      "   \"mimes\": [ { \"type\": \"application/x-ppapi-bad-type\" } ]"
      "}",

      // space before pepper
      "{"
      "   \"type\": \" pepper\","
      "   \"mimes\": [ { \"type\": \"application/x-ppapi-bad-type\" } ]"
      "}"};
  FilePath path(FILE_PATH_LITERAL("bad_type_test.pmf"));

  for (const auto& pmf : pmfs) {
    LOG(INFO) << "Parsing pmf: " << pmf;

    PepperPluginInfo info;
    EXPECT_FALSE(ParsePepperPluginManifest(path, pmf, &info));
  }
}

TEST(ParsePepperPluginManifestTest, BadJSON) {
  std::string pmfs[] = {
      // no coma
      "{"
      "   \"type\": \"pepper\""  // no coma
      "   \"mimes\": [ { \"type\": \"application/x-ppapi-bad-json\" } ]"
      "}",

      // Comma after last entry
      "{"
      "   \"type\": \"pepper\","
      "   \"mimes\": [ { \"type\": \"application/x-ppapi-bad-json\" } ],"
      "}",

      // no double quotes (")
      "{"
      "   type: pepper,"
      "   mimes: [ { \"type\": \"application/x-ppapi-bad-json\" } ],"
      "}",

      // root object is not a dictionary
      "["
      "  { \"type\": \"pepper\" },"
      "  { \"mimes\": ["
      "    { \"type\": \"application/x-ppapi-minimal-manifest\" }"
      "  ] }"
      "]",

      // root object is not a dictionary
      " \"type\": \"pepper\" "};
  FilePath path(FILE_PATH_LITERAL("bad_type_test.pmf"));

  for (const auto& pmf : pmfs) {
    LOG(INFO) << "Parsing pmf: " << pmf;

    PepperPluginInfo info;
    EXPECT_FALSE(ParsePepperPluginManifest(path, pmf, &info));
  }
}

TEST(ParsePepperPluginManifestTest, MultipleMimes) {
  std::string pmf =
      "{"
      "  \"type\" : \"pepper\","
      "  \"name\" : \"Sample Trusted Plugin\","
      "  \"version\" : \"1.0\","
      "  \"description\" : \"Example Pepper Trusted Plugin\","
      "  \"mimes\" : ["
      "     {"
      "       \"type\" : \"application/x-ppapi-example\","
      "       \"description\" : \"Example Ppapi\","
      "       \"extensions\" : [ \".ext1\", \".ext2\" ]"
      "     },"
      "     {"
      "       \"type\" : \"application/x-ppapi-example2\""
      "     }"
      "   ]"
      " }";

  PepperPluginInfo info;
  FilePath path(FILE_PATH_LITERAL("libsample_plugin.pmf"));
  ASSERT_TRUE(ParsePepperPluginManifest(path, pmf, &info));

  EXPECT_FALSE(info.is_internal);
  EXPECT_TRUE(info.is_out_of_process);
  EXPECT_EQ(info.permissions, ppapi::Permission::PERMISSION_ALL_BITS);
  EXPECT_EQ("Sample Trusted Plugin", info.name);
  EXPECT_EQ("Example Pepper Trusted Plugin", info.description);
  EXPECT_EQ("1.0", info.version);

  ASSERT_EQ(2u, info.mime_types.size());
  EXPECT_EQ("application/x-ppapi-example", info.mime_types[0].mime_type);
  EXPECT_TRUE(
      base::EqualsASCII(info.mime_types[0].description, "Example Ppapi"));
  ASSERT_EQ(2u, info.mime_types[0].file_extensions.size());
  EXPECT_EQ(".ext1", info.mime_types[0].file_extensions[0]);
  EXPECT_EQ(".ext2", info.mime_types[0].file_extensions[1]);

  EXPECT_EQ("application/x-ppapi-example2", info.mime_types[1].mime_type);
  EXPECT_TRUE(info.mime_types[1].description.empty());
  EXPECT_TRUE(info.mime_types[1].file_extensions.empty());

  FilePath expected_path(AddLibrarySuffix(path.RemoveExtension()));
  EXPECT_EQ(info.path, expected_path);
}

TEST(ParsePepperPluginManifestTest, EmptyMimes) {
  std::string pmfs[] = {
      // missing mimes key
      "{"
      "  \"type\" : \"pepper\""
      "}",

      // no mimes at all
      "{"
      "  \"type\" : \"pepper\","
      "  \"mimes\" : ["
      "  ]"
      "}",

      // missing type
      "{"
      "  \"type\" : \"pepper\","
      "  \"mimes\" : ["
      "    {"
      "       \"description\" : \"Example Ppapi\","
      "       \"extensions\" : [ \".ext1\", \".ext2\" ]"
      "    }"
      "  ]"
      "}",
  };
  FilePath path(FILE_PATH_LITERAL("libsample_plugin.pmf"));

  for (const auto& pmf : pmfs) {
    LOG(INFO) << "Parsing pmf: " << pmf;

    PepperPluginInfo info;
    EXPECT_FALSE(ParsePepperPluginManifest(path, pmf, &info));
  }
}

TEST(ParsePepperPluginManifestTest, InvalidProgramUrlKey) {
  std::string pmfs[] = {
      // empty program
      "{"
      "  \"type\" : \"pepper\","
      "  \"mimes\": [ { \"type\": \"application/x-ppapi-invalid-url\" } ],"
      "  \"program\": { }"
      "}",

      // valid path but not in url key
      "{"
      "  \"type\" : \"pepper\","
      "  \"mimes\": [ { \"type\": \"application/x-ppapi-invalid-url\" } ],"
      "  \"program\": \"libtest_plugin.so\""
      "}",

      // no url key
      "{"
      "  \"type\" : \"pepper\","
      "  \"mimes\": [ { \"type\": \"application/x-ppapi-invalid-url\" } ],"
      "  \"program\": { \"path\": \"libtest_plugin.so\" }"  // not an url key
      "}",

      // empty url key
      "{"
      "  \"type\" : \"pepper\","
      "  \"mimes\": [ { \"type\": \"application/x-ppapi-invalid-url\" } ],"
      "  \"program\": { \"url\": \"\" }"
      "}"};

  FilePath path(FILE_PATH_LITERAL("plugin_manifest.pmf"));
  for (const auto& pmf : pmfs) {
    LOG(INFO) << "Parsing pmf: " << pmf;

    PepperPluginInfo info;
    EXPECT_FALSE(ParsePepperPluginManifest(path, pmf, &info));
  }
}

TEST(ParsePepperPluginManifestTest, ProgramUrlPaths) {
  FilePath path(FILE_PATH_LITERAL("/my_plugin_dir/plugin_manifest.pmf"));

  std::pair<std::string, FilePath::StringType> pmf_and_path_list[] = {
      // Relative path
      {"{"
       "  \"type\" : \"pepper\","
       "  \"name\" : \"Test Trusted Plugin\","
       "  \"version\" : \"1.2\","
       "  \"description\" : \"Test Trusted Pepper Plugin\","
       "  \"mimes\": [ { \"type\": \"application/x-ppapi-test\" } ],"
       "  \"program\": { \"url\": \"libtest_plugin.so\" }"
       "}",
       FILE_PATH_LITERAL("/my_plugin_dir/libtest_plugin.so")},

      // Absolute path
      {"{"
       "  \"type\" : \"pepper\","
       "  \"name\" : \"Test Trusted Plugin\","
       "  \"version\" : \"1.2\","
       "  \"description\" : \"Test Trusted Pepper Plugin\","
       "  \"mimes\": [ { \"type\": \"application/x-ppapi-test\" } ],"
       "  \"program\": { \"url\": \"/my_lib_dir/libtest_plugin.so\" }"
       "}",
       FILE_PATH_LITERAL("/my_lib_dir/libtest_plugin.so")},

      // Deduced path
      {"{"
       "  \"type\" : \"pepper\","
       "  \"name\" : \"Test Trusted Plugin\","
       "  \"version\" : \"1.2\","
       "  \"description\" : \"Test Trusted Pepper Plugin\","
       "  \"mimes\": [ { \"type\": \"application/x-ppapi-test\" } ]"
       "}",
       FILE_PATH_LITERAL("/my_plugin_dir/plugin_manifest.so")}};

  for (const auto& entry : pmf_and_path_list) {
    const auto& pmf = entry.first;
    const auto& expected_path = entry.second;
    LOG(INFO) << "Parsing pmf: " << pmf;

    PepperPluginInfo info;
    EXPECT_TRUE(ParsePepperPluginManifest(path, pmf, &info));

    EXPECT_FALSE(info.is_internal);
    EXPECT_TRUE(info.is_out_of_process);
    EXPECT_EQ(info.permissions, ppapi::Permission::PERMISSION_ALL_BITS);
    EXPECT_EQ("Test Trusted Plugin", info.name);
    EXPECT_EQ("Test Trusted Pepper Plugin", info.description);
    EXPECT_EQ("1.2", info.version);

    EXPECT_EQ(1u, info.mime_types.size());
    if (info.mime_types.size() == 1) {
      EXPECT_EQ("application/x-ppapi-test", info.mime_types[0].mime_type);
      EXPECT_TRUE(info.mime_types[0].description.empty());
      EXPECT_TRUE(info.mime_types[0].file_extensions.empty());
    }

    EXPECT_EQ(FilePath(expected_path), info.path);
  }
}

TEST(FillPepperPluginInfoFromManifestTest, InvalidManifest) {
  std::string pmf =
      // empty url key
      "{"
      "  \"type\" : \"pepper\","
      "  \"mimes\": [ { \"type\": \"application/x-ppapi-invalid-url\" } ],"
      "  \"program\": { \"url\": \"\" }"
      "}";

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  FilePath path;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir.GetPath(), &path));

  int data_written = WriteFile(path, pmf.c_str(), pmf.size());
  ASSERT_GE(data_written, 0);
  ASSERT_EQ(pmf.size(), static_cast<size_t>(data_written));

  PepperPluginInfo info;
  EXPECT_FALSE(FillPepperPluginInfoFromManifest(path, &info));
}

TEST(FillPepperPluginInfoFromManifestTest, NonExsistingManifest) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  FilePath path;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir.GetPath(), &path));

  FilePath non_existing_path = path.AddExtension(".non_exsisting");
  ASSERT_FALSE(base::PathExists(non_existing_path));

  PepperPluginInfo info;
  EXPECT_FALSE(FillPepperPluginInfoFromManifest(path, &info));
}

TEST(FillPepperPluginInfoFromManifestTest, WrongLibraryPath) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  FilePath path;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir.GetPath(), &path));

  FilePath non_existing_path = path.AddExtension(".non_exsisting");
  ASSERT_FALSE(base::PathExists(non_existing_path));

  std::ostringstream pmf_stream;
  pmf_stream << "{"
             << "  \"type\": \"pepper\","
             << "  \"mimes\": ["
             << "    { \"type\": \"application/x-ppapi-minimal-manifest\" }"
             << "  ],"
             << "  \"program\": "
             << "    { \"url\": \"" << non_existing_path.AsUTF8Unsafe()
             << "\" }"
             << "}";
  std::string pmf = pmf_stream.str();

  PepperPluginInfo info;
  ASSERT_TRUE(ParsePepperPluginManifest(path, pmf, &info));

  int data_written = base::WriteFile(path, pmf.c_str(), pmf.size());
  ASSERT_GE(data_written, 0);
  ASSERT_EQ(pmf.size(), static_cast<size_t>(data_written));

  EXPECT_FALSE(FillPepperPluginInfoFromManifest(path, &info));
}

TEST(FillPepperPluginInfoFromManifestTest, ValidManifest) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  FilePath pmf_path;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir.GetPath(), &pmf_path));

  FilePath plugin_path;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir.GetPath(), &plugin_path));

  std::ostringstream pmf_stream;
  pmf_stream << "{"
             << "  \"type\": \"pepper\","
             << "  \"mimes\": ["
             << "    { \"type\": \"application/x-ppapi-minimal-manifest\" }"
             << "  ],"
             << "  \"program\": "
             << "    { \"url\": \"" << plugin_path.AsUTF8Unsafe() << "\" }"
             << "}";
  std::string pmf = pmf_stream.str();

  PepperPluginInfo info;
  ASSERT_TRUE(ParsePepperPluginManifest(pmf_path, pmf, &info));

  int data_written = base::WriteFile(pmf_path, pmf.c_str(), pmf.size());
  ASSERT_GE(data_written, 0);
  ASSERT_EQ(pmf.size(), static_cast<size_t>(data_written));

  EXPECT_TRUE(FillPepperPluginInfoFromManifest(pmf_path, &info));
}

TEST_F(TrustedPepperPluginInfoCacheTest, GetPlugins) {
  // Test checks:
  // 1. That plugins all plugins present in cache are listed
  //    exacly once in plugins vector passed to
  //    |TrustedPepperPluginInfoCache::GetPlugins|
  // 2. That plugins present in vector passed to
  //    |TrustedPepperPluginInfoCache::GetPlugins| are not added
  //    for the 2nd time
  // 3. Entries present in vector passed to
  //    |TrustedPepperPluginInfoCache::GetPlugins| and not present in cache
  //    are untouched.
  //
  // To acheive that:
  // - set of all plugins present in cache and not present in cache
  //   is build
  // - for all subsets it is checked that passing such subset to
  //   |TrustedPepperPluginInfoCache::GetPlugins| will result
  //   in behavior from point 1 - 3.
  using PluginEntry = std::tuple<PepperPluginInfo, bool, bool>;
  std::vector<PluginEntry> all_plugins;

  for (const auto& plugin : valid_plugins_)
    all_plugins.emplace_back(plugin, true, false);

  for (const auto& plugin : invalid_plugins_)
    all_plugins.emplace_back(plugin, false, false);

  // This test browses all subsets, so ensure that it won't last to long
  ASSERT_LT(all_plugins.size(), 16u)
      << "Too much plugins - test will lasts to long";

  uint32_t subsets_count = 1 << all_plugins.size();
  LOG(INFO) << "all_plugins.size: " << all_plugins.size();
  LOG(INFO) << "subsets_count: " << subsets_count;

  // Process all subsets of |all_plugins|
  for (uint32_t subset = 0; subset < subsets_count; ++subset) {
    LOG(INFO) << "processing subset: " << subset << " hex: " << std::hex
              << subset;

    // Build initial plugins vector and compute how much plugins should be
    // added from cache to the vector.
    std::vector<PepperPluginInfo> init_plugins;
    size_t missing_plugins_count = valid_plugins_.size();
    for (size_t i = 0; i < all_plugins.size(); ++i) {
      std::get<2>(all_plugins[i]) = (subset & (1 << i));

      if (std::get<1>(all_plugins[i]) && std::get<2>(all_plugins[i]))
        --missing_plugins_count;

      if (std::get<2>(all_plugins[i]))
        init_plugins.push_back(std::get<0>(all_plugins[i]));
    }
    std::vector<PepperPluginInfo> plugins = init_plugins;

    // Call |TrustedPepperPluginInfoCache::GetPlugins| and fill plugins vector
    cache_->GetPlugins(&plugins);

    // Test that required plugins were added to the vector
    size_t expected_plugins_size = init_plugins.size() + missing_plugins_count;
    EXPECT_EQ(expected_plugins_size, plugins.size());
    EXPECT_LE(init_plugins.size(), plugins.size());

    // Check that initial pluings in vector are left untouched
    if (init_plugins.size() <= plugins.size()) {
      EXPECT_TRUE(std::equal(init_plugins.cbegin(), init_plugins.cend(),
                             plugins.cbegin(), EqualPepperPluginInfo));
    }

    // Check that all valid are present in |plugins| exactly once
    for (const auto& plugin : valid_plugins_) {
      EXPECT_EQ(1, count_if(plugins.cbegin(), plugins.cend(),
                            [&plugin](const PepperPluginInfo& info) {
                              return EqualPepperPluginInfo(plugin, info);
                            }));
    }
  }
}

TEST_F(TrustedPepperPluginInfoCacheTest, FindValidPlugin) {
  for (const auto& plugin : valid_plugins_) {
    for (const auto& web_plugin_mime : plugin.mime_types) {
      PepperPluginInfo found_plugin;
      EXPECT_TRUE(
          cache_->FindPlugin(web_plugin_mime.mime_type, GURL(), &found_plugin));
      EXPECT_TRUE(EqualPepperPluginInfo(plugin, found_plugin));
    }
  }
}

TEST_F(TrustedPepperPluginInfoCacheTest, FindInvalidPlugin) {
  for (const auto& plugin : invalid_plugins_) {
    for (const auto& web_plugin_mime : plugin.mime_types) {
      PepperPluginInfo found_plugin;
      EXPECT_FALSE(
          cache_->FindPlugin(web_plugin_mime.mime_type, GURL(), &found_plugin));
    }
  }
}

TEST_F(TrustedPepperPluginInfoCacheTest, FindNonExistingMimes) {
  for (const auto& mime_type : kNonExistingMimes) {
    PepperPluginInfo found_plugin;
    EXPECT_FALSE(cache_->FindPlugin(mime_type, GURL(), &found_plugin));
  }
}

TEST_F(TrustedPepperPluginInfoCacheTest, FindByUrl) {
  GURL url = GURL("file:///example.swf");
  PepperPluginInfo found_plugin;
  EXPECT_TRUE(cache_->FindPlugin("", url, &found_plugin));
  EXPECT_STREQ("application/x-shockwave-flash",
               found_plugin.mime_types[0].mime_type.data());
}

}  // namespace
}  // namespace pepper
