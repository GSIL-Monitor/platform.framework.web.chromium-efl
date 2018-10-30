// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINT_WEB_VIEW_HELPER_H_
#define PRINT_WEB_VIEW_HELPER_H_

#include "base/files/file_path.h"
#include "common/print_pages_params.h"
#include "third_party/WebKit/public/platform/WebCanvas.h"

namespace content {
class RenderView;
}

namespace blink {
class WebLocalFrame;
}

namespace printing {
class PdfMetafileSkia;
}

struct PrintPagesParams;

class PrintWebViewHelperEfl {
 public:
  PrintWebViewHelperEfl(content::RenderView* view, const base::FilePath& filename);
  virtual ~PrintWebViewHelperEfl();
  void PrintToPdf(int width, int height);
  void InitPrintSettings(int width, int height, bool fit_to_paper_size);

 private:
  bool PrintPagesToPdf(blink::WebLocalFrame* frame, int page_count,
      const gfx::Size& canvas_size);
  bool PrintPageInternal(const PrintPageParams& params,
                         const gfx::Size& canvas_size,
                         blink::WebLocalFrame* frame,
                         printing::PdfMetafileSkia* metafile);
  bool RenderPagesForPrint(blink::WebLocalFrame* frame);

  std::unique_ptr<PrintPagesParams> print_pages_params_;
  content::RenderView* view_;
  base::FilePath filename_;
};

#endif // PRINT_WEB_VIEW_HELPER_H_
