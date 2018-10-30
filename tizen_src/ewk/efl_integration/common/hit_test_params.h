// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HIT_TEST_PARAMS_H_
#define HIT_TEST_PARAMS_H_

#include <map>
#include <string>

#include "ipc/ipc_message.h"
#include "ipc/ipc_param_traits.h"
#include "ui/gfx/geometry/size.h"
#include "third_party/skia/include/core/SkBitmap.h"

#include "ui/gfx/ipc/gfx_param_traits.h"

typedef std::map<std::string, std::string> NodeAttributesMap;

struct Hit_Test_Params {
  int context;
  std::string linkURI;
  std::string linkTitle; // the title of link
  std::string linkLabel; // the text of the link
  std::string imageURI;
  bool isEditable;
  int mode;

  struct Node_Data {
    std::string tagName;      // tag name for hit element
    std::string nodeValue;    // node value for hit element
    NodeAttributesMap attributes;
  } nodeData;

  struct Image_Data {
    std::string fileNameExtension; // image filename extension for hit element
    SkBitmap imageBitmap;          // image pixels data
  } imageData;
};

#endif // HIT_TEST_PARAMS_H_
