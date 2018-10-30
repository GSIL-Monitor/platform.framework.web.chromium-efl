// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_INPUT_PICKER_COLOR_CHOOSER_EFL_H_
#define EWK_EFL_INTEGRATION_BROWSER_INPUT_PICKER_COLOR_CHOOSER_EFL_H_

#include "content/public/browser/color_chooser.h"
#include "third_party/skia/include/core/SkColor.h"

#include <Evas.h>

namespace content {

class ColorChooser;
class WebContents;

class ColorChooserEfl : public ColorChooser {
 public:
  ColorChooserEfl(WebContents& web_contents);
  ~ColorChooserEfl() override;

  // ColorChooser implementation.
  void SetSelectedColor(SkColor color) override;
  void End() override;

 private:
  WebContents& web_contents_;
};

}

#endif // EWK_EFL_INTEGRATION_BROWSER_INPUT_PICKER_COLOR_CHOOSER_EFL_H_
