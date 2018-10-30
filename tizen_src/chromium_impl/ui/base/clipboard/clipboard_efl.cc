// Copyright 2014 Samsung Electronics. All rights reseoved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/clipboard/clipboard_efl.h"

#include <Elementary.h>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard_helper_efl.h"
#include "ui/gfx/geometry/size.h"

using base::string16;

namespace ui {
namespace {
const char kPlainTextFormat[] = "text/plain";
const char kHTMLFormat[] = "text/html";
const char kRTFFormat[] = "rtf";
const char kBitmapFormat[] = "bitmap";
const char kWebKitSmartPasteFormat[] = "webkit_smart";
const char kMimeTypePepperCustomData[] = "chromium/x-pepper-custom-data";
const char kMimeTypeWebCustomData[] = "chromium/x-web-custom-data";
} // namespace

// Clipboard factory method.
Clipboard* Clipboard::Create() {
  return new ClipboardEfl;
}

ClipboardEfl::FormatType::FormatType() {
}

Clipboard::FormatType::FormatType(const std::string& native_format)
    : data_(native_format) {
}

Clipboard::FormatType::~FormatType() {
}

std::string Clipboard::FormatType::Serialize() const {
  return data_;
}

// static
Clipboard::FormatType Clipboard::FormatType::Deserialize(
    const std::string& serialization) {
  return FormatType(serialization);
}

bool Clipboard::FormatType::Equals(const FormatType& other) const {
  return data_ == other.data_;
}

ClipboardEfl::ClipboardEfl() {
  DCHECK(CalledOnValidThread());
}

ClipboardEfl::~ClipboardEfl() {
  DCHECK(CalledOnValidThread());
}

// Main entry point used to write several values in the clipboard.
void ClipboardEfl::WriteObjects(ClipboardType type, const ObjectMap& objects) {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);

  for (ObjectMap::const_iterator iter = objects.begin();
       iter != objects.end(); ++iter) {
    DispatchObject(static_cast<ObjectType>(iter->first), iter->second);
  }
}

void ClipboardEfl::OnPreShutdown() {}

uint64_t ClipboardEfl::GetSequenceNumber(ClipboardType /* type */) const {
  DCHECK(CalledOnValidThread());
  // TODO: implement this. For now this interface will advertise
  // that the clipboard never changes. That's fine as long as we
  // don't rely on this signal.
  return 0;
}

bool ClipboardEfl::IsFormatAvailable(const Clipboard::FormatType& format,
                                  ClipboardType /* clipboard_type */) const {
  DCHECK(CalledOnValidThread());
  return ClipboardHelperEfl::GetInstance()->IsFormatAvailable(format);
}

void ClipboardEfl::Clear(ClipboardType /* type */) {
  ClipboardHelperEfl::GetInstance()->Clear();
}

void ClipboardEfl::ReadAvailableTypes(ClipboardType type, std::vector<string16>* types,
                                   bool* contains_filenames) const {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);

  if (!types || !contains_filenames) {
    NOTREACHED();
    return;
  }

  types->clear();
  *contains_filenames = false;
  if (IsFormatAvailable(GetPlainTextFormatType(), type))
    types->push_back(base::UTF8ToUTF16(GetPlainTextFormatType().ToString()));
  if (IsFormatAvailable(GetHtmlFormatType(), type))
    types->push_back(base::UTF8ToUTF16(GetHtmlFormatType().ToString()));
  if (IsFormatAvailable(GetBitmapFormatType(), type))
    types->push_back(base::UTF8ToUTF16(GetBitmapFormatType().ToString()));
}

void ClipboardEfl::ReadText(ClipboardType type, string16* result) const {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);
  std::string utf8;
  ReadAsciiText(type, &utf8);
  *result = base::UTF8ToUTF16(utf8);
}

void ClipboardEfl::ReadAsciiText(ClipboardType type, std::string* result) const {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);
  ClipboardHelperEfl::GetInstance()->RetrieveClipboardItem(
      result, ClipboardDataTypeEfl::PLAIN_TEXT);
}

void ClipboardEfl::ReadHTML(ClipboardType type, string16* markup,
    std::string* src_url, uint32_t* fragment_start,
    uint32_t* fragment_end) const {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(type, CLIPBOARD_TYPE_COPY_PASTE);
  // This is supposed to contain context for html parsing.
  // We set 0 and markup->size() respectively, which is same as other platforms.
  src_url->clear();
  markup->clear();
  *fragment_start = 0;
  *fragment_end = 0;

  std::string clipboard_data;
  if (ClipboardHelperEfl::GetInstance()->RetrieveClipboardItem(
          &clipboard_data, ClipboardDataTypeEfl::MARKUP)) {
    *markup = base::UTF8ToUTF16(clipboard_data);
    *fragment_end = markup->size();
  }
}

void ClipboardEfl::ReadRTF(ClipboardType type, std::string* result) const {
  NOTIMPLEMENTED();
}

SkBitmap ClipboardEfl::ReadImage(ClipboardType type) const {
  NOTIMPLEMENTED();
  return SkBitmap();
}

void ClipboardEfl::ReadBookmark(string16* title, std::string* url) const {
  NOTIMPLEMENTED();
}

void ClipboardEfl::ReadCustomData(ClipboardType clipboard_type,
                               const string16& type,
                               string16* result) const {
  NOTIMPLEMENTED();
}

void ClipboardEfl::ReadData(const FormatType& format, std::string* result) const {
  NOTIMPLEMENTED();
}

// static
Clipboard::FormatType Clipboard::GetFormatType(const std::string& format_string) {
  return FormatType::Deserialize(format_string);
}

// static
const Clipboard::FormatType& Clipboard::GetPlainTextFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kPlainTextFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetPlainTextWFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kPlainTextFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetWebKitSmartPasteFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kWebKitSmartPasteFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetHtmlFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kHTMLFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetRtfFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kRTFFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetBitmapFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kBitmapFormat));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetWebCustomDataFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kMimeTypeWebCustomData));
  return type;
}

// static
const Clipboard::FormatType& Clipboard::GetPepperCustomDataFormatType() {
  CR_DEFINE_STATIC_LOCAL(FormatType, type, (kMimeTypePepperCustomData));
  return type;
}

void ClipboardEfl::WriteText(const char* text_data, size_t text_len) {
  ClipboardHelperEfl::GetInstance()->SetData(std::string(text_data, text_len),
                                             ClipboardDataTypeEfl::PLAIN_TEXT);
}

void ClipboardEfl::WriteHTML(const char* markup_data, size_t markup_len,
                          const char* url_data, size_t url_len) {
  ClipboardHelperEfl::GetInstance()->SetData(
      std::string(markup_data, markup_len), ClipboardDataTypeEfl::MARKUP);
}

void ClipboardEfl::WriteRTF(const char* rtf_data, size_t data_len) {
  NOTIMPLEMENTED();
}

void ClipboardEfl::WriteBookmark(const char* title_data, size_t title_len,
                              const char* url_data, size_t url_len) {
  NOTIMPLEMENTED();
}

void ClipboardEfl::WriteWebSmartPaste() {
  NOTIMPLEMENTED();
}

void ClipboardEfl::WriteBitmap(const SkBitmap& bitmap) {
  NOTIMPLEMENTED();
}

void ClipboardEfl::WriteData(const FormatType& format, const char* data_data, size_t data_len) {
  NOTIMPLEMENTED();
}

} //namespace ui
