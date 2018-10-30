// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/upload_element_reader.h"

namespace net {

const UploadBytesElementReader* UploadElementReader::AsBytesReader() const {
  return nullptr;
}

const UploadFileElementReader* UploadElementReader::AsFileReader() const {
  return nullptr;
}

bool UploadElementReader::IsInMemory() const {
  return false;
}

#if defined(USE_EFL)
int64_t UploadElementReader::GetSizeSync() const {
  LOG(ERROR) << "Trying to get size of unsupported UploadElementReader";
  return -1;
}

bool UploadElementReader::DumpReaderData(std::string& data) const {
  LOG(ERROR) << "Trying to dump data from unsupported UploadElementReader";
  return false;
}
#endif

}  // namespace net
