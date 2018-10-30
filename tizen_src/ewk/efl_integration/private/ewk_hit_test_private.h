// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_hit_test_private_h
#define ewk_hit_test_private_h

#include <map>
#include <string>

#include <Eina.h>

#include "common/hit_test_params.h"
#include "public/ewk_hit_test_internal.h"
#include "third_party/skia/include/core/SkBitmap.h"

typedef std::map<std::string, std::string> NodeAttributesMap;

class _Ewk_Hit_Test {
 public:
  explicit _Ewk_Hit_Test(const Hit_Test_Params& params);
  ~_Ewk_Hit_Test();

  Ewk_Hit_Test_Result_Context GetResultContext() const;

  const char* GetLinkUri() const;
  const char* GetLinkTitle() const;
  const char* GetLinkLabel() const;

  const char* GetImageUri() const;
  const char* GetImageFilenameExtension() const;
  void*       GetImageBuffer() const;
  size_t      GetImageBufferLength() const;

  const char* GetMediaUri() const;

  const char* GetNodeTagName() const;
  const char* GetNodeValue() const;
  Eina_Hash*  GetNodeAttributeHash() const;

 private:
  const Ewk_Hit_Test_Result_Context context;
  const std::string linkURI;
  const std::string linkTitle; // the title of link
  const std::string linkLabel; // the text of the link
  const std::string imageURI;
  const std::string mediaURI;
  const bool isEditable;
  const Ewk_Hit_Test_Mode mode;

  // store node attributes in a map
  const class Hit_Test_Node_Data {
   public:
    explicit Hit_Test_Node_Data(const Hit_Test_Params::Node_Data& data);
    ~Hit_Test_Node_Data();

    const std::string tagName;      // tag name for hit element
    const std::string nodeValue;    // node value for hit element
    Eina_Hash* attributeHash; // attribute data for hit element

   private:
    DISALLOW_COPY_AND_ASSIGN(Hit_Test_Node_Data);
  } nodeData;

  // when hit node is image we need to store image buffer and filename extension
  const class Hit_Test_Image_Buffer {
   public:
    explicit Hit_Test_Image_Buffer(const Hit_Test_Params::Image_Data& data);

    const std::string fileNameExtension; // image filename extension for hit element
    SkBitmap imageBitmap;          // image pixels data

   private:
    DISALLOW_COPY_AND_ASSIGN(Hit_Test_Image_Buffer);
  } imageData;

  DISALLOW_COPY_AND_ASSIGN(_Ewk_Hit_Test);
};


#endif // ewk_hit_test_private_h
