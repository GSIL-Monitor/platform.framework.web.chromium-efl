// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_hit_test_private.h"

static void FreeHitTestAttributeHashData(void* data) {
   eina_stringshare_del(static_cast<Eina_Stringshare*>(data));
 }

_Ewk_Hit_Test::_Ewk_Hit_Test(const Hit_Test_Params& params)
    : context(static_cast<Ewk_Hit_Test_Result_Context>(params.context)),
      linkURI(params.linkURI),
      linkTitle(params.linkTitle),
      linkLabel(params.linkLabel),
      imageURI(params.imageURI),
      isEditable(params.isEditable),
      mode(static_cast<Ewk_Hit_Test_Mode>(params.mode)),
      nodeData(params.nodeData),
      imageData(params.imageData) {
}

_Ewk_Hit_Test::~_Ewk_Hit_Test() {
}

_Ewk_Hit_Test::Hit_Test_Node_Data::Hit_Test_Node_Data(const Hit_Test_Params::Node_Data& data)
    : tagName(data.tagName),
      nodeValue(data.nodeValue),
      attributeHash(NULL) {
  const NodeAttributesMap& nodeAttributes = data.attributes;
  if (!nodeAttributes.empty()) {
    attributeHash = eina_hash_string_superfast_new(FreeHitTestAttributeHashData);
    NodeAttributesMap::const_iterator attributeMapEnd = nodeAttributes.end();
    for (NodeAttributesMap::const_iterator it = nodeAttributes.begin(); it != attributeMapEnd; ++it) {
      eina_hash_add(attributeHash, it->first.c_str(), eina_stringshare_add(it->second.c_str()));
    }
  }
}

_Ewk_Hit_Test::Hit_Test_Node_Data::~Hit_Test_Node_Data() {
  eina_hash_free(attributeHash);
}

//(warning) deep copy of skia buffer.
_Ewk_Hit_Test::Hit_Test_Image_Buffer::Hit_Test_Image_Buffer(const Hit_Test_Params::Image_Data& data):
  fileNameExtension(data.fileNameExtension) {
  if (imageBitmap.tryAllocPixels(data.imageBitmap.info())) {
    data.imageBitmap.readPixels(imageBitmap.info(), imageBitmap.getPixels(),
                                imageBitmap.rowBytes(), 0, 0);
  }
}


Ewk_Hit_Test_Result_Context _Ewk_Hit_Test::GetResultContext() const {
  return context;
}

const char* _Ewk_Hit_Test::GetLinkUri() const {
  return linkURI.c_str();
}

const char* _Ewk_Hit_Test::GetLinkTitle() const {
  return linkTitle.c_str();
}

const char* _Ewk_Hit_Test::GetLinkLabel() const {
  return linkLabel.c_str();
}

const char* _Ewk_Hit_Test::GetImageUri() const {
  return imageURI.c_str();
}

const char* _Ewk_Hit_Test::GetImageFilenameExtension() const {
  return imageData.fileNameExtension.c_str();
}

void* _Ewk_Hit_Test::GetImageBuffer() const {
  return imageData.imageBitmap.getPixels();
}

size_t _Ewk_Hit_Test::GetImageBufferLength() const {
  return imageData.imageBitmap.getSize();
}

const char* _Ewk_Hit_Test::GetMediaUri() const {
  return mediaURI.c_str();
}

const char* _Ewk_Hit_Test::GetNodeTagName() const {
  return nodeData.tagName.c_str();
}

const char* _Ewk_Hit_Test::GetNodeValue() const {
  return nodeData.nodeValue.c_str();
}

Eina_Hash* _Ewk_Hit_Test::GetNodeAttributeHash() const {
  return nodeData.attributeHash;
}
