// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/print_web_view_helper_efl.h"

#include "base/logging.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "common/render_messages_ewk.h"
#include "printing/metafile_skia_wrapper.h"
#include "printing/pdf_metafile_skia.h"
#include "printing/units.h"
#include "base/compiler_specific.h"
#include "third_party/WebKit/public/web/WebPrintParams.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

namespace {

int ConvertUnit(int value, int old_unit, int new_unit) {
  DCHECK_GT(new_unit, 0);
  DCHECK_GT(old_unit, 0);
  // With integer arithmetic, to divide a value with correct rounding, you need
  // to add half of the divisor value to the dividend value. You need to do the
  // reverse with negative number.
  if (value >= 0)
    return ((value * new_unit) + (old_unit / 2)) / old_unit;
  else
    return ((value * new_unit) - (old_unit / 2)) / old_unit;
}

void ComputeWebKitPrintParamsInDesiredDpi(const PrintParams& print_params,
    blink::WebPrintParams* webkit_print_params) {
  int dpi = static_cast<int>(print_params.dpi);
  webkit_print_params->printer_dpi = dpi;
  webkit_print_params->print_scaling_option = print_params.print_scaling_option;

  webkit_print_params->print_content_area.width =
      ConvertUnit(print_params.content_size.width(), dpi,
                  print_params.desired_dpi);
  webkit_print_params->print_content_area.height =
      ConvertUnit(print_params.content_size.height(), dpi,
                  print_params.desired_dpi);

  webkit_print_params->printable_area.x =
      ConvertUnit(print_params.printable_area.x(), dpi,
                  print_params.desired_dpi);
  webkit_print_params->printable_area.y =
      ConvertUnit(print_params.printable_area.y(), dpi,
                  print_params.desired_dpi);
  webkit_print_params->printable_area.width =
      ConvertUnit(print_params.printable_area.width(), dpi,
                  print_params.desired_dpi);
  webkit_print_params->printable_area.height =
      ConvertUnit(print_params.printable_area.height(),
                  dpi, print_params.desired_dpi);

  webkit_print_params->paper_size.width =
      ConvertUnit(print_params.page_size.width(), dpi,
                  print_params.desired_dpi);
  webkit_print_params->paper_size.height =
      ConvertUnit(print_params.page_size.height(), dpi,
                  print_params.desired_dpi);
}

} //namespace

PrintWebViewHelperEfl::PrintWebViewHelperEfl(content::RenderView* view,
    const base::FilePath& filename)
  : view_(view),
    filename_(filename) {
}

PrintWebViewHelperEfl::~PrintWebViewHelperEfl() {
}

void PrintWebViewHelperEfl::PrintToPdf(int width, int height) {
  if (!view_->GetWebView() || !view_->GetWebView()->MainFrame() ||
      !view_->GetWebView()->MainFrame()->IsWebLocalFrame())
    return;

  InitPrintSettings(width, height, true);
  RenderPagesForPrint(view_->GetWebView()->MainFrame()->ToWebLocalFrame());
}

void PrintWebViewHelperEfl::InitPrintSettings(
    int width, int height, bool fit_to_paper_size) {
  PrintPagesParams settings;
  settings.params.content_size.SetSize(width, height);
  settings.params.desired_dpi = 600;
  settings.params.dpi = 600;
  settings.params.page_size.SetSize(width, height);
  settings.params.print_to_pdf = true;
  settings.pages.clear();

  settings.params.print_scaling_option =
      blink::kWebPrintScalingOptionSourceSize;
  if (fit_to_paper_size) {
    settings.params.print_scaling_option =
        blink::kWebPrintScalingOptionFitToPrintableArea;
  }

  print_pages_params_.reset(new PrintPagesParams(settings));
}

bool PrintWebViewHelperEfl::RenderPagesForPrint(blink::WebLocalFrame* frame) {
  DCHECK(frame);
  const PrintPagesParams& params = *print_pages_params_;
  const PrintParams& print_params = params.params;
  blink::WebPrintParams webkit_print_params;
  ComputeWebKitPrintParamsInDesiredDpi(print_params, &webkit_print_params);
  int page_count = frame->PrintBegin(webkit_print_params);
  gfx::Size print_canvas_size(webkit_print_params.print_content_area.width,
      webkit_print_params.print_content_area.height);
  bool result = PrintPagesToPdf(frame, page_count, print_canvas_size);
  frame->PrintEnd();
  return result;
}

bool PrintWebViewHelperEfl::PrintPagesToPdf(blink::WebLocalFrame* frame,
    int page_count, const gfx::Size& canvas_size) {
  printing::PdfMetafileSkia metafile(printing::SkiaDocumentType::PDF);
  if (!metafile.Init())
    return false;

  const PrintPagesParams& params = *print_pages_params_;
  std::vector<int> printed_pages;

  if (params.pages.empty()) {
    for (int i = 0; i < page_count; ++i)
      printed_pages.push_back(i);
  } else {
    for (size_t i = 0; i < params.pages.size(); ++i) {
      if (params.pages[i] >= 0 && params.pages[i] < page_count)
        printed_pages.push_back(params.pages[i]);
    }
  }

  if (printed_pages.empty())
    return false;

  PrintPageParams page_params;
  page_params.params = params.params;

  for (size_t i = 0; i < printed_pages.size(); ++i) {
    page_params.page_number = printed_pages[i];
    if (!PrintPageInternal(page_params, canvas_size, frame, &metafile)) {
      LOG(ERROR) << "Could not write page #"
                 << page_params.page_number << " in pdf.";
    }
  }

  metafile.FinishDocument();

  // Get the size of the resulting metafile.
  uint32_t buf_size = metafile.GetDataSize();
  DCHECK_GT(buf_size, 0u);

  DidPrintPagesParams printed_page_params;
  printed_page_params.data_size = 0;
  printed_page_params.document_cookie = params.params.document_cookie;
  printed_page_params.filename = filename_;

  {
    std::unique_ptr<base::SharedMemory> shared_mem(
        content::RenderThread::Get()->HostAllocateSharedMemoryBuffer(
             buf_size).release());
    if (!shared_mem.get()) {
      NOTREACHED() << "AllocateSharedMemoryBuffer failed";
      return false;
    }

    if (!shared_mem->Map(buf_size)) {
      NOTREACHED() << "Map failed";
      return false;
    }
    metafile.GetData(shared_mem->memory(), buf_size);
    printed_page_params.data_size = buf_size;
    printed_page_params.metafile_data_handle =
        base::SharedMemory::DuplicateHandle(shared_mem->handle());
  } // shared memory handle scope

  return view_->Send(new EwkHostMsg_DidPrintPagesToPdf(
      view_->GetRoutingID(), printed_page_params));
}

bool PrintWebViewHelperEfl::PrintPageInternal(
    const PrintPageParams& params,
    const gfx::Size& canvas_size,
    blink::WebLocalFrame* frame,
    printing::PdfMetafileSkia* metafile) {
  PrintParams result;
  double scale_factor = 1.0f;
  gfx::Rect canvas_area(canvas_size);
  cc::PaintCanvas* canvas = metafile->GetVectorCanvasForNewPage(
      params.params.page_size, canvas_area, scale_factor);
  if (!canvas)
    return false;

  printing::MetafileSkiaWrapper::SetMetafileOnCanvas(canvas, metafile);

  frame->PrintPage(params.page_number, canvas);
  // Done printing. Close the device context to retrieve the compiled metafile.
  if (!metafile->FinishPage()) {
    NOTREACHED() << "metafile failed";
    return false;
  }
  return true;
}
