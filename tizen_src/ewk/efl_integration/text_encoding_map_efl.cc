// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "text_encoding_map_efl.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

// The below Encodings have been taken from
// chromium/src/third_party/WebKit/Source/wtf/text/TextCodecLatin1.cpp
// chromium/src/third_party/WebKit/Source/wtf/text/TextCodecUTF8.cpp
// chromium/src/third_party/WebKit/Source/wtf/text/TextCodecUserDefined.cpp
// chromium/src/third_party/WebKit/Source/wtf/text/TextCodecUTF16.cpp
// chromium/src/third_party/WebKit/Source/wtf/text/TextCodecICU.cpp
//
// The TextEncodingNameMap(present in
// /chromium/src/third_party/WebKit/Source/wtf/text/TextEncodingRegistry.cpp)
// is contructed by combining the Encodings and aliases present in the above
// files.
//
// The character encoding list used by chrome browser is also used as reference.
// It is located in :
// chromium/src/chrome/browser/character_encoding.cc

const std::vector<std::string> TextEncodingNameMap = {
    "windows-1252", "ISO-8859-1",    "US-ASCII",       "UTF-8",
    "UTF-16LE",     "UTF-16BE",      "x-user-defined", "ISO-8859-8-I",
    "GBK",          "GB_2312-80",    "EUC-KR",         "Windows-1254",
    "Windows-874",  "Big5",          "macintosh",      "EUC-JP",
    "Big5-HKSCS",   "Shift_JIS",     "EUC-JP",         "ISO-2022-JP",
    "windows-874",  "windows-949",   "ISO-8859-15",    "ISO-8859-2",
    "windows-1250", "windows-1251",  "windows-1258",   "KOI8-R",
    "ISO-8859-7",   "windows-1253",  "windows-1256",   "windows-1257",
    "ISO-8859-8",   "windows-1254",  "windows-1255",   "ISO-8859-6",
    "ISO-8859-1",   "ISO-8859-3",    "ISO-8859-4",     "ISO-8859-10",
    "ISO-8859-13",  "ISO-8859-14",   "ISO-8859-5",     "ISO-8859-9",
    "cp864",        "x-mac-cyrillic"};

}  // namespace

// static
TextEncodingMapEfl* TextEncodingMapEfl::GetInstance() {
  return base::Singleton<TextEncodingMapEfl>::get();
}

bool TextEncodingMapEfl::isTextEncodingValid(const char* name) {
  if (!name || !name[0])
    return 0;

  std::string str(name);
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return encoding_name_set_.find(str) != encoding_name_set_.end();
}

TextEncodingMapEfl::TextEncodingMapEfl() {
  for (size_t i = 0; i < TextEncodingNameMap.size(); i++) {
    std::string name = TextEncodingNameMap[i];
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    encoding_name_set_.insert(name);
  }
}
