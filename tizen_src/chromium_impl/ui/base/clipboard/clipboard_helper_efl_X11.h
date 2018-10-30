// Copyright 2014 Samsung Electronics. All rights reseoved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLIPBOARD_HELPER_EFL_X11_H_
#define CLIPBOARD_HELPER_EFL_X11_H_

#include <Ecore.h>
#include <Elementary.h>
#include <string>
#include "ecore_x_wayland_wrapper.h"

#include "base/memory/singleton.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/ui_base_export.h"

// This class is based on ClipboardHelper class in WK2/Tizen.
// Coding Style has been changed as per Chromium coding style.

// TODO: Get some documentation how this works with CBHM.
// CBHM is an application/daemon running in Tizen device.
// This helper class interacts with CBHM using Ecore/X message passing API.
// There are a few magic strings in implementation.
class EWebView;
enum class ClipboardDataTypeEfl;

template <typename T>
struct DefaultSingletonTraits;

class UI_BASE_EXPORT ClipboardHelperEfl {
 public:
  static ClipboardHelperEfl* GetInstance();

  void SetData(const std::string& data, ClipboardDataTypeEfl type);
  void Clear();
  static int NumberOfItems();
  bool CanPasteFromSystemClipboard() {
    return ClipboardHelperEfl::NumberOfItems() > 0;
  }
  bool CanPasteFromClipboardApp() {
    return ClipboardHelperEfl::NumberOfItems() > 0;
  }
  bool IsFormatAvailable(const ui::Clipboard::FormatType& format);
  bool RetrieveClipboardItem(std::string* result, ClipboardDataTypeEfl format);
  void OpenClipboardWindow(EWebView* view, bool richly_editable);
  void CloseClipboardWindow();
  bool IsClipboardWindowOpened();
  bool getSelectedCbhmItem(Ecore_X_Atom* pDataType);
  void UpdateClipboardWindowState(Ecore_X_Event_Window_Property* ev);
  static void connectClipboardWindow();

 private:
  ClipboardHelperEfl();
  friend struct base::DefaultSingletonTraits<ClipboardHelperEfl>;

  Ecore_X_Window GetCbhmWindow();
  bool SendCbhmMessage(const std::string& message);
  bool SetClipboardItem(Ecore_X_Atom data_type, const std::string& data);
  std::string GetCbhmReply(Ecore_X_Window xwin,
                           Ecore_X_Atom property,
                           Ecore_X_Atom* data_type);
  void clearClipboardHandler();
  void initializeAtomList();
  bool RetrieveClipboardItem(int index,
                             Elm_Sel_Format* format,
                             std::string* data);
  DISALLOW_COPY_AND_ASSIGN(ClipboardHelperEfl);

  Ecore_Event_Handler* m_selectionClearHandler;
  Ecore_Event_Handler* m_selectionNotifyHandler;
  Ecore_Event_Handler* property_change_handler_;
  bool clipboard_window_opened_;
};

#endif /* CLIPBOARD_HELPER_EFL_X11_H_ */
