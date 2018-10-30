// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _EFL_WINDOW_FACTORY_EFL_H_
#define _EFL_WINDOW_FACTORY_EFL_H_

#include <Evas.h>
#include <map>

#include "efl/efl_export.h"

namespace content {
class WebContents;
};

namespace efl {

class EFL_EXPORT WindowFactory {
 public:
  typedef Evas_Object* (*GetHostWindowDelegate)(const content::WebContents*);

  // TODO: Function should return NativeView, once NativeView typedef is fixed
  static Evas_Object* GetHostWindow(const content::WebContents*);

  static void SetDelegate(GetHostWindowDelegate d) { delegate_ = d; };

 private:
  static void OnWindowRemoved(void*, Evas_Object*, void*);

  static std::map<const content::WebContents*, Evas_Object*> window_map_;
  static GetHostWindowDelegate delegate_;
};

} // namespace efl

#endif
