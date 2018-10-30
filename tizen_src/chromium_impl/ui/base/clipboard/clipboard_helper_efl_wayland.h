// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CLIPBOARD_HELPER_EFL_WAYLAND_H_
#define CLIPBOARD_HELPER_EFL_WAYLAND_H_

#include <Eldbus.h>
#include <Elementary.h>
#include <string>

#include "base/callback.h"
#include "base/memory/singleton.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/ui_base_export.h"

enum class ClipboardDataTypeEfl;
class EWebView;

class UI_BASE_EXPORT ClipboardHelperEfl {
 public:
  using ExecCommandCallback = base::Callback<void(const char*, const char*)>;
  static ClipboardHelperEfl* GetInstance();
  void SetData(const std::string& data, ClipboardDataTypeEfl type);
  void Clear();
  bool CanPasteFromClipboardApp() const;
  bool CanPasteFromSystemClipboard() const {
    // FIXME(g.ludwikowsk): for now it is the same as CanPasteFromClipboardApp,
    // but we should also take into account that if there are only images in
    // clipboard and we are in a text input then we can't paste, but can show
    // clipboard window (like in tizen 2.4)
    return CanPasteFromClipboardApp();
  }
  bool IsFormatAvailable(const ui::Clipboard::FormatType& format) const;
  void RefreshClipboard();
  bool RetrieveClipboardItem(std::string* data,
                             ClipboardDataTypeEfl format) const;

  // OnWebviewFocusIn needs a callback to EWebView's ExecuteEditCommand
  // function, because we can't include eweb_view.h header in chromium_impl
  // directory to call ExecuteEditCommand directly.
  // We also keep a pointer to EWebView instance associated with the callback,
  // to know if we should reset exec_commnad_callback_ when that EWebView is
  // destroyed.
  void OnWebviewFocusIn(EWebView* webview,
                        Evas_Object* source,
                        bool is_content_editable,
                        ExecCommandCallback exec_command_callback);
  void MaybeInvalidateActiveWebview(EWebView* webview);
  void OpenClipboardWindow(EWebView* webview, bool richly_editable);
  void CloseClipboardWindow();
  bool IsClipboardWindowOpened() const {
    // FIXME(g.ludwikowsk): This doesn't work currently, see comment in
    // ::CbhmOnNameOwnerChanged
    return clipboard_window_opened_;
  }
  void SetContentEditable(bool is_content_editable) {
    is_content_editable_ = is_content_editable;
  }

  static void ShutdownIfNeeded();

 private:
  friend struct base::DefaultSingletonTraits<ClipboardHelperEfl>;

  static Eina_Bool SelectionGetCbHTML(void* data,
                                      Evas_Object* obj,
                                      Elm_Selection_Data* ev);
  static Eina_Bool SelectionGetCbText(void* data,
                                      Evas_Object* obj,
                                      Elm_Selection_Data* ev);
  static Eina_Bool SelectionGetCbAppHTML(void* data,
                                         Evas_Object* obj,
                                         Elm_Selection_Data* ev);
  static Eina_Bool SelectionGetCbAppText(void* data,
                                         Evas_Object* obj,
                                         Elm_Selection_Data* ev);

  static bool Base64ImageTagFromImagePath(const std::string& path,
                                          std::string* image_html);
  static bool ConvertImgTagToBase64(const std::string& tag,
                                    std::string* out_tag);

  static void CbhmOnNameOwnerChanged(void* data,
                                     const char* bus,
                                     const char* old_id,
                                     const char* new_id);
  static void OnClipboardItemClicked(void* data,
                                     const Eldbus_Message* msg);
  static void GetClipboardTextPost(ClipboardHelperEfl* self);

  ClipboardHelperEfl();
  ~ClipboardHelperEfl();

  void RefreshClipboardForApp();
  void ProcessClipboardAppEvent(Elm_Selection_Data* ev, Elm_Sel_Format format);

  const std::string& clipboard_html() const {
    return paste_from_clipboard_app_
               ? clipboard_contents_html_from_clipboard_app_
               : clipboard_contents_html_;
  }
  const std::string& clipboard_text() const {
    return paste_from_clipboard_app_ ? clipboard_contents_from_clipboard_app_
                                     : clipboard_contents_;
  }

  void CbhmEldbusInit();
  void CbhmEldbusDeinit();
  int CbhmNumberOfItems() const;

  // elm_cnp_selection_* API requires an elm object
  // TODO(g.ludwikowsk): what happens when we use different source widgets
  // (multiple webviews)?
  Evas_Object* source_widget_;

  std::string clipboard_contents_html_;
  std::string clipboard_contents_html_from_clipboard_app_;
  std::string clipboard_contents_;
  std::string clipboard_contents_from_clipboard_app_;

  // Clipboard app integration:
  Eldbus_Proxy* eldbus_proxy_;
  Eldbus_Connection* cbhm_conn_;
  ExecCommandCallback exec_command_callback_;
  bool clipboard_window_opened_;
  bool paste_from_clipboard_app_;
  bool accept_clipboard_app_events_;
  bool is_content_editable_;
  EWebView* last_webview_;

  DISALLOW_COPY_AND_ASSIGN(ClipboardHelperEfl);
};

#endif /* CLIPBOARD_HELPER_EFL_WAYLAND_H_ */
