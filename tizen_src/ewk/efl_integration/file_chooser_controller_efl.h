// Copyright 2014-2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef file_chooser_controller_efl_h
#define file_chooser_controller_efl_h

#include <Evas.h>

#include "content/public/common/file_chooser_params.h"

namespace content {

class RenderFrameHost;

class FileChooserControllerEfl {
 public:
  FileChooserControllerEfl(RenderFrameHost* render_frame_host,
                           const content::FileChooserParams* params);
  ~FileChooserControllerEfl() {}  // LCOV_EXCL_LINE

  void Open();

  const content::FileChooserParams* GetParams() { return params_; }
  void SetParams(const content::FileChooserParams* params) {
    params_ = params;
  }

  RenderFrameHost* GetRenderFrameHost() {
    return render_frame_host_;
  }

 private:
  void ParseParams();
  void CreateAndShowFileChooser();
  bool LaunchFileChooser();
  bool LaunchCamera(const std::string& mime_type);

  RenderFrameHost* render_frame_host_;
  const content::FileChooserParams* params_;
  std::string title_;
  std::string file_name_;
  Eina_Bool is_save_file_;
  Eina_Bool folder_only_;

  DISALLOW_COPY_AND_ASSIGN(FileChooserControllerEfl);
};

} // namespace

#endif // file_chooser_controller_efl_h

