// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextEncodingMapEfl_h
#define TextEncodingMapEfl_h

#include "base/memory/singleton.h"

#include <Eina.h>
#include "base/containers/hash_tables.h"

namespace base {
template <typename T> struct DefaultSingletonTraits;
}

class TextEncodingMapEfl {  // LCOV_EXCL_LINE
 public:
  static TextEncodingMapEfl* GetInstance();
  bool isTextEncodingValid(const char* name);
 private:
  friend struct base::DefaultSingletonTraits<TextEncodingMapEfl>;
  TextEncodingMapEfl();
  base::hash_set<std::string> encoding_name_set_;
};

#endif // TextEncodingRegistry_h
