// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_FILE_UTILITIES_HOST_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_FILE_UTILITIES_HOST_IMPL_H_

#include "content/common/file_utilities.mojom.h"

namespace content {

class FileUtilitiesHostImpl : public content::mojom::FileUtilitiesHost {
 public:
  explicit FileUtilitiesHostImpl(int process_id);
  ~FileUtilitiesHostImpl() override;

  static void Create(int process_id,
                     content::mojom::FileUtilitiesHostRequest request);

 private:
  // blink::mojom::FileUtilitiesHost implementation.
  void GetFileInfo(const base::FilePath& path,
                   GetFileInfoCallback callback) override;

  const int process_id_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_FILE_UTILITIES_HOST_IMPL_H_
