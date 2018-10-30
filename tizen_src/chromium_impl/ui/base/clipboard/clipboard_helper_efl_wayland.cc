// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "clipboard_helper_efl.h"

#include <Ecore_Wayland.h>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"

namespace {
const char* const kCbhmDBusObjpath = "/org/tizen/cbhm/dbus";
const char* const kCbhmDbusInterface = "org.tizen.cbhm.dbus";
const std::string kFileScheme("file://");
const std::string kHTMLImageFromClipboardAppFront("<img src=\"file://");
const std::string::size_type kHTMLImageFromClipboardAppFrontLength =
    kHTMLImageFromClipboardAppFront.length();
const std::string kHTMLImageFromClipboardAppEnd("\">");
const std::string kDataURIImagePrefix("<img alt=\"\" src=\"data:image;base64,");
const std::string kDataURIImageSuffix("\">");
bool g_eldbus_initialized = false;
}  // namespace

ClipboardHelperEfl* ClipboardHelperEfl::GetInstance() {
  return base::Singleton<ClipboardHelperEfl>::get();
}

void ClipboardHelperEfl::ShutdownIfNeeded() {
  if (g_eldbus_initialized)
    ClipboardHelperEfl::GetInstance()->CbhmEldbusDeinit();
}

ClipboardHelperEfl::ClipboardHelperEfl()
    : source_widget_(nullptr),
      clipboard_window_opened_(false),
      paste_from_clipboard_app_(false),
      accept_clipboard_app_events_(false),
      is_content_editable_(false),
      last_webview_(nullptr) {
  CbhmEldbusInit();
}

ClipboardHelperEfl::~ClipboardHelperEfl() {
  CbhmEldbusDeinit();
}

bool ClipboardHelperEfl::IsFormatAvailable(
    const ui::Clipboard::FormatType& format) const {
  // Poor man's format check.
  std::string format_text(format.ToString());
  if (format_text.find("text/plain") != std::string::npos) {
    return !clipboard_text().empty();
  } else if (format_text.find("text/html") != std::string::npos) {
    return !clipboard_html().empty();
  }
  return false;
}

void ClipboardHelperEfl::SetData(const std::string& data,
                                 ClipboardDataTypeEfl type) {
  if (!source_widget_) {
    LOG(ERROR) << "[CLIPBOARD] ClipboardHelperEfl::SetData "
                  "elm_cnp_selection_set with NULL widget";
    return;
  }

  Elm_Sel_Format data_format = ClipboardFormatToElm(type);

  if (data.length() > 204800) {
    // EFL silently truncates data passed through clipboard at 204800 bytes.
    // See: http://suprem.sec.samsung.net/jira/browse/TSAM-12220
    LOG(WARNING) << "[CLIPBOARD] EFL silently truncates clipboard "
                    "data over 204800 bytes - length of data: "
                 << data.length();
  }
  // TODO(g.ludwikowsk): change if-else to a switch
  if (data_format == ELM_SEL_FORMAT_TEXT) {
    clipboard_contents_ = net::EscapeForHTML(data);
    // Clear clipboard_contents_html_ to avoid using stale value. Html will
    // be set in next call to SetData (if at all).
    clipboard_contents_html_.clear();
    // Using ELM_SEL_FORMAT_TEXT in elm_cnp_selection_set in this case causes
    // strange effect, that html data is overriden in clipboard app. Workaround
    // is to use ELM_SEL_FORMAT_HTML even for text (which should be a valid
    // html anyway).
    // TODO: investigate this problem and create a bug
    elm_cnp_selection_set(source_widget_, ELM_SEL_TYPE_CLIPBOARD,
                          ELM_SEL_FORMAT_HTML, clipboard_contents_.c_str(),
                          clipboard_contents_.length());
  } else if (data_format == ELM_SEL_FORMAT_HTML) {
    clipboard_contents_html_ = data;
    elm_cnp_selection_set(source_widget_, ELM_SEL_TYPE_CLIPBOARD,
                          ELM_SEL_FORMAT_HTML, clipboard_contents_html_.c_str(),
                          clipboard_contents_html_.length());
  } else if (data_format == ELM_SEL_FORMAT_IMAGE) {
    // Image is downloaded by engine and we get local file path here.
    // TODO(g.ludwikowsk): investigate if it is better to just get image html
    // and let clipboard app handle the downloading.
    std::string base64_img_tag;
    if (Base64ImageTagFromImagePath(data, &base64_img_tag))
      clipboard_contents_html_ = base64_img_tag;
    else
      LOG(ERROR) << "[CLIPBOARD] ConvertImgTagToBase64 failed";
    // Clipboard service doesn't work unless we pass image paths as a file URL.
    // This behavior is not documented anywhere.
    // See: https://review.tizen.org/gerrit/#/c/86582/
    std::string file_uri = kFileScheme + data;
    elm_cnp_selection_set(source_widget_, ELM_SEL_TYPE_CLIPBOARD,
                          ELM_SEL_FORMAT_IMAGE, file_uri.c_str(),
                          file_uri.length());
  } else {
    LOG(ERROR)
        << "[CLIPBOARD] ClipboardHelperEfl::SetData unsupported data type "
        << data_format;
  }
}

void ClipboardHelperEfl::Clear() {
  if (!source_widget_) {
    LOG(ERROR) << "[CLIPBOARD] ClipboardHelperEfl::Clear "
                  "elm_cnp_selection_set with NULL widget";
    return;
  }
  elm_cnp_selection_set(source_widget_, ELM_SEL_TYPE_CLIPBOARD,
                        ELM_SEL_FORMAT_TEXT, nullptr, 0);
}

bool ClipboardHelperEfl::Base64ImageTagFromImagePath(const std::string& path,
                                                     std::string* image_html) {
  std::string file_contents;
  if (!base::ReadFileToString(base::FilePath(path), &file_contents)) {
    LOG(ERROR) << "[CLIPBOARD] couldn't read file: " << path;
    return false;
  }

  base::Base64Encode(file_contents, &file_contents);

  *image_html = kDataURIImagePrefix;
  *image_html += file_contents;
  *image_html += kDataURIImageSuffix;
  return true;
}

// We get images from clipboard app as <img> tags with local path ("file://"
// scheme), but we can't paste them in http and https pages (browsers block
// this for security reasons). To work around this limitation we convert local
// images to base64 encoded data URI.
bool ClipboardHelperEfl::ConvertImgTagToBase64(const std::string& tag,
                                               std::string* out_tag) {
  std::string::size_type front_pos = tag.find(kHTMLImageFromClipboardAppFront);
  if (front_pos == std::string::npos) {
    LOG(ERROR) << "[CLIPBOARD] couldn't find "
               << kHTMLImageFromClipboardAppFront
               << " in html from clipboard app: " << tag;
    return false;
  }

  std::string front_stripped =
      tag.substr(front_pos + kHTMLImageFromClipboardAppFrontLength);
  std::string::size_type back_pos =
      front_stripped.find(kHTMLImageFromClipboardAppEnd);
  if (back_pos == std::string::npos) {
    LOG(ERROR) << "[CLIPBOARD] couldn't find " << kHTMLImageFromClipboardAppEnd
               << " in html from clipboard app: " << front_stripped;
    return false;
  }

  std::string image_path = front_stripped.substr(0, back_pos);
  return Base64ImageTagFromImagePath(image_path, out_tag);
}

void ClipboardHelperEfl::GetClipboardTextPost(ClipboardHelperEfl* self) {
  if (!elm_cnp_selection_get(self->source_widget_, ELM_SEL_TYPE_CLIPBOARD,
                             ELM_SEL_FORMAT_TEXT, SelectionGetCbText, self))
    LOG(ERROR)
        << "[CLIPBOARD] GetClipboardTextPost elm_cnp_selection_get failed";
}

bool ClipboardHelperEfl::RetrieveClipboardItem(
    std::string* data,
    ClipboardDataTypeEfl format) const {
  // elm_cnp_selection_get is async, so no point in calling it here. We have
  // to do polling or call it on some specific moments, like being focused.
  Elm_Sel_Format elm_format = ClipboardFormatToElm(format);
  if (elm_format == ELM_SEL_FORMAT_TEXT) {
    *data = clipboard_text();
    return true;
  } else if (elm_format == ELM_SEL_FORMAT_HTML) {
    *data = clipboard_html();
    return true;
  }

  LOG(ERROR) << "[CLIPBOARD] Unexpected selection data format: "
             << static_cast<int>(elm_format);
  return false;
}

bool ClipboardHelperEfl::CanPasteFromClipboardApp() const {
  return CbhmNumberOfItems() > 0;
}

void ClipboardHelperEfl::RefreshClipboard() {
  if (!source_widget_) {
    LOG(ERROR) << "[CLIPBOARD] ClipboardHelperEfl::RefreshClipboard "
                  "elm_cnp_selection_get with NULL widget";
    return;
  }

  paste_from_clipboard_app_ = false;

  // We need to get both html and plain text from clipboard.
  // elm_cnp_selection_get doesn't allow requesting both types on the same time
  // and doesn't support multiple calls at once - we get only one callback
  // anyway. If elm_cnp_selection_get succeeds for ELM_SEL_FORMAT_HTML, we
  // call elm_cnp_selection_get for ELM_SEL_FORMAT_TEXT when callback returns.
  //
  // elm_cnp_selection_get failing for ELM_SEL_FORMAT_HTML is normal operation
  // if html is not available. In such case we request ELM_SEL_FORMAT_TEXT
  // immediately.
  if (elm_cnp_selection_get(source_widget_, ELM_SEL_TYPE_CLIPBOARD,
                            ELM_SEL_FORMAT_HTML, SelectionGetCbHTML, this))
    return;

  // Avoid using stale clipboard_contents_html_ value. elm_cnp_selection_get
  // failed for ELM_SEL_FORMAT_HTML, so there is no html data in clipboard.
  clipboard_contents_html_.clear();
  if (!elm_cnp_selection_get(source_widget_, ELM_SEL_TYPE_CLIPBOARD,
                             ELM_SEL_FORMAT_TEXT, SelectionGetCbText, this))
    LOG(INFO) << "[CLIPBOARD] elm_cnp_selection_get in RefreshClipboard failed"
              << " for both HTML and TEXT";
}

Eina_Bool ClipboardHelperEfl::SelectionGetCbHTML(void* data,
                                                 Evas_Object* obj,
                                                 Elm_Selection_Data* ev) {
  auto self = static_cast<ClipboardHelperEfl*>(data);

  if (!ev->data) {
    LOG(ERROR) << "[CLIPBOARD] ClipboardHelperEfl::SelectionGetCbHTML no data";
    return EINA_TRUE;
  }

  // Convert ev->data to std::string, because it might not be
  // a null terminated c-string.
  std::string selection_data(static_cast<char*>(ev->data), ev->len);

  // Process HTML, because we get images from clipboard app as lone <img> tag
  // with local file path, which can't be used in normal web pages.
  // See comment in ::ConvertImgTagToBase64
  std::string img_tag;
  if (ConvertImgTagToBase64(selection_data, &img_tag)) {
    self->clipboard_contents_html_ = img_tag;
  } else {
    self->clipboard_contents_html_ = selection_data;
    // We received html from clipboard app, but we also want plain text version
    // of the selection. See comment in ::RefreshClipboard why we do it here.
    // Also we have to post it on UI thread, because ::SelectionGetCbHTML is
    // running as elm_cnp_selection_get callback, so we can't call
    // elm_cnp_selection_get from inside.
    // TODO(g.ludwikowsk): should we also get text for images?
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                     base::Bind(GetClipboardTextPost, self));
  }
  return EINA_TRUE;
}

Eina_Bool ClipboardHelperEfl::SelectionGetCbText(void* data,
                                                 Evas_Object* obj,
                                                 Elm_Selection_Data* ev) {
  auto self = static_cast<ClipboardHelperEfl*>(data);

  if (!ev->data) {
    LOG(ERROR) << "[CLIPBOARD] ClipboardHelperEfl::SelectionGetCbText no data";
    return EINA_TRUE;
  }

  // Convert ev->data to std::string, because it might not be
  // a null terminated c-string.
  std::string selection_data(static_cast<char*>(ev->data), ev->len);

  self->clipboard_contents_ = selection_data;
  return EINA_TRUE;
}

void ClipboardHelperEfl::OnWebviewFocusIn(
    EWebView* webview,
    Evas_Object* source,
    bool is_content_editable,
    ExecCommandCallback exec_command_callback) {
  source_widget_ = source;
  last_webview_ = webview;
  // Normally we are notified about change in contentEditable status of focused
  // node by RenderWidgetHostViewEfl through ::SetContentEditable when focused
  // node changes. But there is another way in which contentEditable status can
  // change for us - if user switches to another tab in browser we won't get
  // information from RWHVEfl since from its point of view the focused node
  // didn't change. That is why also EWebView informs us when it gains focus.
  is_content_editable_ = is_content_editable;
  exec_command_callback_ = exec_command_callback;
  accept_clipboard_app_events_ = true;
  RefreshClipboard();
}

void ClipboardHelperEfl::MaybeInvalidateActiveWebview(EWebView* webview) {
  if (last_webview_ == webview) {
    CloseClipboardWindow();
    accept_clipboard_app_events_ = false;
    last_webview_ = nullptr;
    exec_command_callback_.Reset();
  }
}

void ClipboardHelperEfl::OnClipboardItemClicked(void* data,
                                                const Eldbus_Message* /*msg*/) {
  // ::OnClipboardItemClicked calls elm_cnp_selection_get (async), which
  // calls us back when clipboard data is ready. We save this data and ask the
  // engine to perform "paste" execCommand (also async), which calls us back
  // for the data saved before. When waiting for these asynchronous calls we
  // might get a second "ItemClicked" and ::OnClipboardItemClicked, which can
  // have surprising results.
  //
  // elm_cnp_selection_get API has a habit of ignoring consecutive requests,
  // so second "ItemClicked" might be ignored. Otherwise the result might be
  // pasting the same content twice, despite that the user clicked different
  // items in clipboard app, because it only keeps track of the latest clicked
  // item.
  //
  // It is also a possibility that this might be a non-issue, because the
  // phenomenon is limited by user interaction speed.
  //
  // TODO(g.ludwikowsk): investigate if we should add a queue of clipboard app
  // paste requests, or a way to ignore other requests before the first one is
  // completed, or if it is ok to do nothing.

  auto self = static_cast<ClipboardHelperEfl*>(data);

  if (!self->source_widget_) {
    LOG(ERROR) << "[CLIPBOARD] ClipboardHelperEfl::OnClipboardItemClicked "
                  "elm_cnp_selection_get with NULL widget";
    return;
  }
  // We get clipboard's "ItemClicked" signal also when we are suspended
  // and clipboard is being used in other applications, or when we are
  // unfocused and browser UI is the target of clipboard app.
  //
  // Ignore events if webview is not focused.
  if (!self->accept_clipboard_app_events_) {
    LOG(INFO) << "[CLIPBOARD] OnClipboardItemClicked not focused - not "
                 "accepting clipboard app_events";
    return;
  }

  if (self->is_content_editable_) {
    if (elm_cnp_selection_get(self->source_widget_, ELM_SEL_TYPE_CLIPBOARD,
                              ELM_SEL_FORMAT_HTML, SelectionGetCbAppHTML, self))
      return;
    // elm_cnp_selection_get failing for ELM_SEL_FORMAT_HTML is normal
    // operation if html is not available. In such case we request
    // ELM_SEL_FORMAT_TEXT.
  }

  // Avoid using stale clipboard_contents_html_from_clipboard_app_ value.
  // elm_cnp_selection_get failed for ELM_SEL_FORMAT_HTML, so there is no html
  // data in clipboard.
  self->clipboard_contents_html_from_clipboard_app_.clear();

  if (!elm_cnp_selection_get(self->source_widget_, ELM_SEL_TYPE_CLIPBOARD,
                             ELM_SEL_FORMAT_TEXT, SelectionGetCbAppText, self))
    LOG(INFO) << "[CLIPBOARD] elm_cnp_selection_get in OnClipboardItemClicked "
              << "failed for both HTML and TEXT";
}

void ClipboardHelperEfl::ProcessClipboardAppEvent(Elm_Selection_Data* ev,
                                                  Elm_Sel_Format format) {
  if (!ev->data) {
    LOG(ERROR)
        << "[CLIPBOARD] ClipboardHelperEfl::ProcessClipboardAppEvent no data";
    return;
  }

  if (exec_command_callback_.is_null()) {
    LOG(ERROR) << "[CLIPBOARD] exec_command_callback not set, but received "
                  "event from clipboard app";
    return;
  }

  // Convert ev->data to std::string, because it might not be
  // a null terminated c-string.
  std::string selection_data(static_cast<char*>(ev->data), ev->len);

  if (format == ELM_SEL_FORMAT_TEXT) {
    clipboard_contents_from_clipboard_app_ = selection_data;
  } else {
    std::string img_tag;
    if (ConvertImgTagToBase64(selection_data, &img_tag)) {
      clipboard_contents_html_from_clipboard_app_ = img_tag;
    } else {
      clipboard_contents_html_from_clipboard_app_ = selection_data;
    }
  }
  paste_from_clipboard_app_ = true;
  exec_command_callback_.Run("paste", nullptr);
  // Do not reset callback here, there might be multiple consecutive
  // items clicked in Clipboard app. Also do not override
  // clipboard_contents_ here (behavior from Tizen 2.4).
}

Eina_Bool ClipboardHelperEfl::SelectionGetCbAppHTML(void* data,
                                                    Evas_Object* obj,
                                                    Elm_Selection_Data* ev) {
  auto self = static_cast<ClipboardHelperEfl*>(data);
  self->ProcessClipboardAppEvent(ev, ELM_SEL_FORMAT_HTML);
  return EINA_TRUE;
}

Eina_Bool ClipboardHelperEfl::SelectionGetCbAppText(void* data,
                                                    Evas_Object* obj,
                                                    Elm_Selection_Data* ev) {
  auto self = static_cast<ClipboardHelperEfl*>(data);
  self->ProcessClipboardAppEvent(ev, ELM_SEL_FORMAT_TEXT);
  return EINA_TRUE;
}

void ClipboardHelperEfl::OpenClipboardWindow(
    EWebView* /*webview*/,
    bool /*richly_editable*/) {
  // Here we want to show clipboard app window, but only appropraite class
  // of items should be enabled in it. We have to either pass "0" or "1" as
  // string argument in the message - these values are documented in clipboard
  // porting guide, which is not available to general public:
  // "0" - text
  // "1" - text and images
  //
  // See related code in cbhmd_eldbus.c _cbhmd_eldbus_show at:
  // https://review.tizen.org/gerrit/gitweb?p=profile/mobile/platform/core/uifw/cbhm.git;a=blob;f=daemon/cbhmd_eldbus.c;h=ddb3760efac3d89fbe8e365157b613a536add768;hb=5b6f56e8be1ca50c4d3a03a2bf0edbc6c63294db#l135
  // TODO: replace string literals with constants
  if (is_content_editable_) {
    eldbus_proxy_call(
        eldbus_proxy_, "CbhmShow", nullptr, nullptr, -1, "s", "1");
  } else {
    eldbus_proxy_call(
        eldbus_proxy_, "CbhmShow", nullptr, nullptr, -1, "s", "0");
  }

  // TODO(g.ludwikowsk): investigate "clipboard,opened" callback for both
  // X11 and wayland
  // view->SmartCallback<EWebViewCallbacks::ClipboardOpened>().call(0);
}

void ClipboardHelperEfl::CloseClipboardWindow() {
  eldbus_proxy_call(eldbus_proxy_, "CbhmHide", nullptr, nullptr, -1, "");
}

void ClipboardHelperEfl::CbhmOnNameOwnerChanged(void* data,
                                                const char* /*bus*/,
                                                const char* /*old_id*/,
                                                const char* /*new_id*/) {
  // Clipboard porting guide advises to register on owner name changed callback
  // for cbhm_conn_ to somehow use this to track clipboard's opened/closed
  // state. Currently this is called only once when connecting to clipboard
  // app, so it won't work.
  auto self = static_cast<ClipboardHelperEfl*>(data);
  self->clipboard_window_opened_ = !self->clipboard_window_opened_;
}

void ClipboardHelperEfl::CbhmEldbusInit() {
  cbhm_conn_ = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
  Eldbus_Object* eldbus_obj =
      eldbus_object_get(cbhm_conn_, kCbhmDbusInterface, kCbhmDBusObjpath);
  eldbus_proxy_ = eldbus_proxy_get(eldbus_obj, kCbhmDbusInterface);
  eldbus_name_owner_changed_callback_add(
      cbhm_conn_, kCbhmDbusInterface,
      ClipboardHelperEfl::CbhmOnNameOwnerChanged, this, EINA_TRUE);
  eldbus_proxy_signal_handler_add(eldbus_proxy_, "ItemClicked",
                                  ClipboardHelperEfl::OnClipboardItemClicked,
                                  this);
  g_eldbus_initialized = true;
}

void ClipboardHelperEfl::CbhmEldbusDeinit() {
  if (cbhm_conn_) {
    eldbus_connection_unref(cbhm_conn_);
    cbhm_conn_ = nullptr;
  }
  g_eldbus_initialized = false;
}

int ClipboardHelperEfl::CbhmNumberOfItems() const {
  Eldbus_Message* req;
  if (!(req = eldbus_proxy_method_call_new(eldbus_proxy_, "CbhmGetCount"))) {
    LOG(ERROR) << "[CLIPBOARD] eldbus_proxy_method_call_new failed calling "
                  "\"CbhmGetCount\"";
    return -1;
  }

  // Here we ask clipboard app about number of elements it contains, but we
  // have to specify which type of elements we want to count. We have to use
  // either 1 or 2 as integer argument in the message, but these values are not
  // documented anywhere - they can change at any time:
  // 1 - text
  // 2 - text and images
  //
  // See cbhmd_eldbus.c: https://review.tizen.org/gerrit/#/c/90102/
  int atom_index = is_content_editable_ ? 2 : 1;

  if (!eldbus_message_arguments_append(req, "i", atom_index)) {
    LOG(ERROR) << "[CLIPBOARD] eldbus_message_arguments_append failed calling "
                  "\"CbhmGetCount\"";
    eldbus_message_unref(req);
    return -1;
  }

  Eldbus_Message* reply;
  if (!(reply = eldbus_proxy_send_and_block(eldbus_proxy_, req, -1))) {
    LOG(ERROR) << "[CLIPBOARD] eldbus_proxy_send_and_block failed calling "
                  "\"CbhmGetCount\"";
    return -1;
  }
  const char* errname = nullptr;
  const char* errmsg = nullptr;
  if (eldbus_message_error_get(reply, &errname, &errmsg)) {
    LOG(ERROR) << "[CLIPBOARD] eldbus_proxy_send_and_block failed calling "
                  "\"CbhmGetCount\" with error "
               << errname << ": " << errmsg;
    eldbus_message_unref(reply);
    return -1;
  }

  int count;
  if (!eldbus_message_arguments_get(reply, "i", &count)) {
    LOG(ERROR) << "[CLIPBOARD] eldbus_message_arguments_get failed when trying "
                  "to get clipboard item count";
    eldbus_message_unref(reply);
    return -1;
  }

  eldbus_message_unref(reply);
  return count;
}
