// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Elementary.h>
#include <stdio.h>
#include <stdlib.h>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/download_manager_delegate_efl.h"
#include "common/web_contents_utils.h"
#include "content/browser/renderer_host/render_widget_host_view_efl.h"
#include "content/browser/selection/selection_controller_efl.h"
#include "content/common/paths_efl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/navigation_entry.h"
#include "context_menu_controller_efl.h"
#include "eweb_view.h"
#include "net/base/escape.h"
#include "net/base/filename_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "private/ewk_context_menu_private.h"
#include "public/ewk_context_menu_product.h"
#include "third_party/WebKit/public/platform/WebReferrerPolicy.h"
#include "tizen/system_info.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_helper_efl.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"

#if defined(OS_TIZEN)
#include <app_control.h>
#include <efl_extension.h>
#include <memory>
#include "base/macros.h"
#endif

#if defined(TIZEN_CONTEXT_MENU_QUICK_MEMO)
#include <app_manager.h>
#include <appsvc/appsvc.h>
#endif

using web_contents_utils::WebViewFromWebContents;

namespace {
#define VERTICAL true
#define HORIZONTAL false

// The height of the selection menu.
static const int kMenuHeight = 64;

// Popup will be restored after kRestoreTime if it is dismissed unexpectedly.
static const float kRestoreTime = 0.5f;

const int kMinHeightVertical = 840;
const int kMinHeightHorizontal = 420;

const std::string kWebSearchLink("https://www.google.com/search?q=");
const char* const kEditCommandUnselect = "Unselect";
const char* const kDefaultImageExtension = "jpg";
}

namespace content {

int ContextMenuControllerEfl::_popup_item_height = 96;
std::vector<ContextMenuItemEfl> ContextMenuControllerEfl::_context_menu_listdata;

#if defined(TIZEN_CONTEXT_MENU_QUICK_MEMO)
static const char* const kQuickMemoOperation =
    "http://samsung.com/appcontrol/operation/send_text_quickmemo";
static const char* const kQuickMemoAppId = "com.samsung.memo-quicksvc";
static const char* const kQuickMemoAppData =
    "http://tizen.org/appcontrol/data/text";
static const char* const kQuickMemoStr = "com.samsung.memo";
#endif

ContextMenuControllerEfl::ContextMenuControllerEfl(EWebView* wv,
                                                   WebContents& web_contents)
    : webview_(wv),
      native_view_(static_cast<Evas_Object*>(web_contents.GetNativeView())),
      top_widget_(nullptr),
      popup_(nullptr),
      list_(nullptr),
      menu_items_(nullptr),
      menu_items_parent_(nullptr),
      web_contents_(web_contents),
      is_text_selection_(false),
      restore_timer_(nullptr),
      context_menu_status_(NONE),
      weak_ptr_factory_(this) {}

ContextMenuControllerEfl::~ContextMenuControllerEfl() {
  for (std::set<DownloadItem*>::iterator it = clipboard_download_items_.begin();
      it != clipboard_download_items_.end(); ++it) {
    (*it)->RemoveObserver(this);
  }
  for (std::set<DownloadItem*>::iterator it = disk_download_items_.begin();
      it != disk_download_items_.end(); ++it) {
    (*it)->RemoveObserver(this);
  }
  _context_menu_listdata.clear();
  DeletePopup();
}

bool ContextMenuControllerEfl::PopulateAndShowContextMenu(const ContextMenuParams &params) {
  if (!webview_)
    return false;

  DCHECK(menu_items_ == NULL);
  if (menu_items_)
    return false;

  params_ = params;
  menu_items_parent_.reset(new _Ewk_Context_Menu(this));
#if defined(OS_TIZEN_TV_PRODUCT)
  menu_items_parent_.get()->SetPosition(params_.x, params_.y);
#endif
  is_text_selection_ =
      params_.is_editable ||
      (params_.link_url.is_empty() && params_.src_url.is_empty() &&
       params_.media_type == blink::WebContextMenuData::kMediaTypeNone);

  GetProposedContextMenu();
#if defined(OS_TIZEN_TV_PRODUCT)
  LOG(INFO) << "Context Menu Show";
  webview_->SmartCallback<EWebViewCallbacks::ContextMenuShow>().call(
      menu_items_parent_.get());
  return true;
#else
  webview_->SmartCallback<EWebViewCallbacks::ContextMenuCustomize>().call(
      menu_items_parent_.get());

  if (!CreateContextMenu(params))
    return false;

#if defined(USE_WAYLAND)
  ClipboardHelperEfl::GetInstance()->RefreshClipboard();
#endif

  return ShowContextMenu();
#endif
}

void ContextMenuControllerEfl::Move(int x, int y) {
  if (popup_ && is_text_selection_)
    evas_object_move(popup_, x, y);
}

void ContextMenuControllerEfl::AddItemToProposedList(Ewk_Context_Menu_Item_Type item,
                                                     Ewk_Context_Menu_Item_Tag option,
                                                     const std::string& title,
                                                     const std::string& image_url,
                                                     const std::string& link_url,
                                                     const std::string& icon_path) {
  ContextMenuItemEfl* new_menu_item = new ContextMenuItemEfl(
      item, option, title, image_url, link_url, icon_path);
  _Ewk_Context_Menu_Item* new_item =
      new _Ewk_Context_Menu_Item(new_menu_item, menu_items_parent_.get());
  menu_items_ = eina_list_append(menu_items_, new_item);
}

void ContextMenuControllerEfl::GetProposedContextMenu() {
  if (!params_.link_url.is_empty()) {
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
                          EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK,
                          dgettext("WebKit",
#if defined(OS_TIZEN_TV_PRODUCT)
                                   "IDS_WEBVIEW_OPEN"),
#else
                                   "IDS_WEBVIEW_OPT_OPEN_LINK_IN_CURRENT_TAB_ABB"),
#endif
                          params_.link_url.spec(), params_.link_url.spec());
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
        EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW,
        std::string(dgettext("WebKit",
            "IDS_WEBVIEW_OPT_OPEN_LINK_IN_NEW_TAB_ABB")),
        params_.link_url.spec(),
        params_.link_url.spec());
    AddItemToProposedList(
        EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
        EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_DATA,
        std::string(dgettext("WebKit", "IDS_TPLATFORM_OPT_COPY_LINK_TEXT")),
        std::string(), params_.link_url.spec());
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
        EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD,
        std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_COPY_LINK_URL_ABB")),
        std::string(),
        params_.link_url.spec());
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
        EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_LINK_TO_DISK,
        std::string(dgettext("WebKit", "IDS_BR_BODY_SAVE_LINK")),
        std::string(),
        params_.link_url.spec());
  }
  if (params_.is_editable &&
      ClipboardHelperEfl::GetInstance()->CanPasteFromSystemClipboard()) {
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION, EWK_CONTEXT_MENU_ITEM_TAG_PASTE,
        std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_PASTE")));
  }
  if (params_.media_type != blink::WebContextMenuData::kMediaTypeImage &&
      !params_.selection_text.empty() && params_.is_editable &&
      params_.input_field_type !=
          blink::WebContextMenuData::kInputFieldTypePassword) {
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION, EWK_CONTEXT_MENU_ITEM_TAG_CUT,
        std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_CUT_ABB")));
  }
  if (params_.media_type != blink::WebContextMenuData::kMediaTypeImage &&
      !params_.selection_text.empty() &&
      params_.input_field_type !=
          blink::WebContextMenuData::kInputFieldTypePassword) {
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION, EWK_CONTEXT_MENU_ITEM_TAG_COPY,
        std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_COPY")));
#if defined(TIZEN_CONTEXT_MENU_QUICK_MEMO)
    app_info_filter_h filter = NULL;

    if (APP_MANAGER_ERROR_NONE == app_info_filter_create(&filter)) {
      if (APP_MANAGER_ERROR_NONE ==
          app_info_filter_add_string(filter, PACKAGE_INFO_PROP_APP_ID,
                                     kQuickMemoStr)) {
        int pkg_count = 0;
        if ((APP_MANAGER_ERROR_NONE ==
             app_info_filter_count_appinfo(filter, &pkg_count)) &&
            (pkg_count > 0)) {
          AddItemToProposedList(
              EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
              EWK_CONTEXT_MENU_ITEM_TAG_QUICKMEMO,
              std::string(dgettext("WebKit", "IDS_WEBVIEW_HEADER_QUICK_MEMO")));
        } else {
          LOG(ERROR) << "Failed to get count of filtered apps.";
        }
      } else {
        LOG(ERROR) << "Failed to add app filter property to the filter handle";
      }
      app_info_filter_destroy(filter);
    } else {
      LOG(ERROR) << "Failed to create app info filter";
    }
#endif

    if (!params_.is_editable) {
      AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION, EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_WEB,
          std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_WEB_SEARCH")));
    }
  }

  if (params_.has_image_contents && !params_.src_url.is_empty()) {
    AddItemToProposedList(
        EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
        EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_CURRENT_WINDOW,
        std::string(
            dgettext("WebKit", "IDS_WEBVIEW_OPT_OPEN_IMAGE_IN_CURRENT_TAB")),
        params_.src_url.spec(), params_.src_url.spec());
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
        EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_NEW_WINDOW,
        std::string(dgettext("WebKit",
            "IDS_WEBVIEW_OPT_OPEN_IMAGE_IN_NEW_TAB_ABB")),
        params_.src_url.spec(),
        params_.src_url.spec());
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
        EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_IMAGE_TO_DISK,
        std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_SAVE_IMAGE_ABB")),
        params_.src_url.spec(),
        params_.src_url.spec());
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
        EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_TO_CLIPBOARD,
        std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_COPY_IMAGE")),
        params_.src_url.spec(),
        params_.src_url.spec());
  } else if (!params_.link_url.is_empty() && !params_.is_user_select_none) {
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
        EWK_CONTEXT_MENU_ITEM_TAG_TEXT_SELECTION_MODE,
        std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_SELECTION_MODE_ABB")),
        params_.link_url.spec(),
        params_.link_url.spec());
  }

#if defined(OS_TIZEN)
  if (!params_.is_editable && !params_.selection_text.empty()) {
    // TODO: IDS_WEBVIEW_OPT_SHARE should be added for all po files.
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
                          EWK_CONTEXT_MENU_ITEM_TAG_SHARE,
                          dgettext("WebKit", "IDS_WEBVIEW_OPT_SHARE"));
  }
#endif

  if (params_.is_editable) {
    RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(
        web_contents_.GetRenderWidgetHostView());
    const SelectionControllerEfl* controller =
        webview_->GetSelectionController();
    bool should_add_select_and_select_all =
        (!controller) ? true : !controller->IsCaretModeForced();

    if (rwhv && rwhv->HasSelectableText() && should_add_select_and_select_all) {
      if (params_.selection_text.empty() &&
          params_.input_field_type !=
              blink::WebContextMenuData::kInputFieldTypePassword) {
        AddItemToProposedList(
            EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
            EWK_CONTEXT_MENU_ITEM_TAG_SELECT_WORD,
            std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_SELECT_ABB")));
      }

      // This is a tricky way to check if everything in the editable field is
      // selected, because there is no other known way.
      //
      // * params_.selection_text contains selected text
      // * rwhv->GetSelectionText() contains selected
      //   text and some text around selection
      //
      // If both strings are the same length, it means that the selection has
      // no more text around it, so everything is already selected.
      //
      // There is also one more issue with this hack: if the last character of
      // textarea is '\n', there will be additional '\n' reported in selection
      // with surrounding characters, which isn't reported in selection text
      // from params_. This means that lengths aren't equal, so normal length
      // check won't work.
      //
      // To work around this, we assume that everything is already selected if
      // selection with surroundings is one character longer than selection,
      // and that last character is '\n'.
      size_t surroundings_length = rwhv->GetSelectionText().length();
      size_t selection_length = params_.selection_text.length();
      base::char16 surroundings_last_char =
          rwhv->GetSelectionText()[surroundings_length - 1];

      if (surroundings_length != selection_length &&
          !(surroundings_length == selection_length + 1 &&
            surroundings_last_char == '\n')) {
        AddItemToProposedList(
            EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
            EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL,
            std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_SELECT_ALL_ABB")));
      }
    }
  } else {
    if (params_.media_type != blink::WebContextMenuData::kMediaTypeImage &&
        is_text_selection_) {
      AddItemToProposedList(
          EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
          EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL,
          std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_SELECT_ALL_ABB")));
    }
  }

// TODO: DnD context menu item is shown only for images now so that
// behavioral test will pass. When decision is made whether context menu or
// long-press DnD will be used, this code will be left intact or removed.
#if defined(OS_TIZEN)
  if (IsMobileProfile() && params_.is_draggable &&
      params_.media_type == blink::WebContextMenuData::kMediaTypeImage) {
    AddItemToProposedList(
        EWK_CONTEXT_MENU_ITEM_TYPE_ACTION, EWK_CONTEXT_MENU_ITEM_TAG_DRAG,
        std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_DRAG_AND_DROP_ABB")));
  }
#endif

  if (params_.is_editable &&
      (!IsTvProfile() || webview_->GetSettings()->getClipboardEnabled()) &&
      ClipboardHelperEfl::GetInstance()->CanPasteFromClipboardApp()) {
    AddItemToProposedList(EWK_CONTEXT_MENU_ITEM_TYPE_ACTION, EWK_CONTEXT_MENU_ITEM_TAG_CLIPBOARD,
        std::string(dgettext("WebKit", "IDS_WEBVIEW_OPT_CLIPBOARD")));
  }
}

bool ContextMenuControllerEfl::CreateContextMenu(
    const ContextMenuParams& params) {
  Eina_List* ittr;
  void* data;

  if (is_text_selection_) {
    popup_ = elm_ctxpopup_add(native_view_);
#if defined(OS_TIZEN)
    elm_ctxpopup_horizontal_set(popup_, EINA_TRUE);
#endif
  } else {
    top_widget_ = webview_->GetElmWindow();
    evas_object_data_set(top_widget_, "ContextMenuContollerEfl", this);
    popup_ = elm_popup_add(top_widget_);

    if (IsMobileProfile())
      elm_popup_align_set(popup_, ELM_NOTIFY_ALIGN_FILL, 1.0);
    else
      elm_popup_align_set(popup_, 0.5, 0.5);
  }

  if (!popup_)
    return false;

  if (!IsMobileProfile())
    evas_object_smart_member_add(popup_, webview_->evas_object());

  elm_object_tree_focus_allow_set(popup_, EINA_FALSE);

  evas_object_data_set(popup_, "ContextMenuContollerEfl", this);

  _context_menu_listdata.clear();
  if (is_text_selection_) {
    EINA_LIST_FOREACH(menu_items_, ittr, data) {
      _Ewk_Context_Menu_Item* item = static_cast<_Ewk_Context_Menu_Item*> (data);
      ContextMenuItemEfl* context_item = item->GetMenuItem();
      Elm_Object_Item* appended_item = 0;

      if (!context_item->Title().empty()) {
        appended_item = elm_ctxpopup_item_append(popup_,
            context_item->Title().c_str(), 0, ContextMenuItemSelectedCallback,
            context_item);
      } else {
        appended_item = elm_ctxpopup_item_append(popup_, 0, 0,
            ContextMenuItemSelectedCallback,
            context_item);
      }

      if (appended_item)
        _context_menu_listdata.push_back(*context_item);
    }
#if defined(OS_TIZEN)
    // Workaround
    // Need to set "copypaste" style, to let moving handles
    // when ctxpopup is visible
    // http://107.108.218.239/bugzilla/show_bug.cgi?id=11613
    elm_object_style_set(popup_, "copypaste");
#endif

    evas_object_size_hint_weight_set(popup_, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    evas_object_size_hint_max_set(popup_, -1, _popup_item_height);
    evas_object_size_hint_min_set(popup_, 0, _popup_item_height);
  } else {
    std::string selected_link(params_.link_url.spec());
    if (!selected_link.empty()) {
      if (base::StartsWith(selected_link,
                           std::string("mailto:"),
                           base::CompareCase::INSENSITIVE_ASCII)
          || base::StartsWith(selected_link,
                              std::string("tel:"),
                              base::CompareCase::INSENSITIVE_ASCII)) {
        size_t current_pos = 0;
        size_t next_delimiter = 0;
        // Remove the First part and Extract actual URL or title
        // (mailto:abc@d.com?id=345  ==> abc@d.com?id=345)
        if ((next_delimiter = selected_link.find(":", current_pos)) !=
            std::string::npos) {
          current_pos = next_delimiter + 1;
        }

        std::string last_part = selected_link.substr(current_pos);
        std::string core_part = selected_link.substr(current_pos);
        current_pos = 0;
        next_delimiter = 0;
        // Trim the core by removing ? at the end (abc@d.com?id=345  ==>
        // abc@d.com)
        if ((next_delimiter = core_part.find("?", current_pos)) !=
            std::string::npos) {
          last_part =
              core_part.substr(current_pos, next_delimiter - current_pos);
        }
      } else {
        elm_object_part_text_set(popup_, "title,text", selected_link.c_str());
      }
    } else {
      std::string selected_image(params_.src_url.spec());
      if (!selected_image.empty())
        elm_object_part_text_set(popup_, "title,text", selected_image.c_str());
    }

    evas_object_size_hint_weight_set(popup_, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    list_ = elm_list_add(popup_);
    elm_list_mode_set(list_, ELM_LIST_EXPAND);
    evas_object_data_set(list_, "ContextMenuContollerEfl", this);

    Eina_List* ittr;
    void* data;
    EINA_LIST_FOREACH(menu_items_, ittr, data) {
      _Ewk_Context_Menu_Item* item = static_cast<_Ewk_Context_Menu_Item*> (data);
      ContextMenuItemEfl* context_item = item->GetMenuItem();
      if (!context_item->Title().empty()) {
        Elm_Object_Item* appended_item = elm_list_item_append(list_,
            context_item->Title().c_str(),
            NULL,
            NULL,
            ContextMenuItemSelectedCallback,
            context_item);

        if (appended_item)
          _context_menu_listdata.push_back(*context_item);
      }
    }

    ContextMenuPopupListResize();

    evas_object_show(list_);
    elm_object_content_set(popup_, list_);
    evas_object_event_callback_add(popup_, EVAS_CALLBACK_RESIZE,
        ContextMenuPopupResize, this);
  }

  if (_context_menu_listdata.empty()) {
    DeletePopup();
    return false;
  }
  return true;
}

void ContextMenuControllerEfl::ContextMenuHWBackKey(void* data, Evas_Object* obj,
                                                    void* event_info) {
  auto menu_controller = static_cast<ContextMenuControllerEfl*>(data);
  if (menu_controller) {
    auto selection_controller =
        menu_controller->webview_->GetSelectionController();

    if (selection_controller)
      selection_controller->ToggleCaretAfterSelection();

    menu_controller->HideContextMenu();
    evas_object_data_del(obj, "ContextEfl");
  }
}

// static
Eina_Bool ContextMenuControllerEfl::RestoreTimerCallback(void* data) {
  auto* menu_controller = static_cast<ContextMenuControllerEfl*>(data);
  if (!menu_controller)
    return ECORE_CALLBACK_CANCEL;

  auto selection_controller =
      menu_controller->webview_->GetSelectionController();

  if (menu_controller->popup_ && selection_controller &&
      selection_controller->GetSelectionStatus())
    evas_object_show(menu_controller->popup_);

  menu_controller->restore_timer_ = nullptr;

  return ECORE_CALLBACK_CANCEL;
}

void ContextMenuControllerEfl::ContextMenuCancelCallback(void* data, Evas_Object* obj,
                                                         void* event_info) {
  ContextMenuControllerEfl* menu_controller =
      static_cast<ContextMenuControllerEfl*>(
          evas_object_data_get(obj, "ContextMenuContollerEfl"));
  if (!menu_controller)
    return;

  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(
      menu_controller->web_contents_.GetRenderWidgetHostView());
  if (!rwhv)
    return;

  // context_menu is dismissed when we touch outside of popup.
  // It is policy of elm_ctxpopup by EFL team.
  // But according to wcs TC(21~23) context_menu should be displayed,
  // when touchevent is consumed. So we should show context_menu again when
  // touchevent is consumed.
  if (rwhv->IsTouchstartConsumed() || rwhv->IsTouchendConsumed()) {
    evas_object_show(menu_controller->popup_);
    return;
  }

  // TODO: If |elm_ctxpopup_auto_hide_disabled_set| is works properly,
  // We should remove this timer.
  // Bug: http://suprem.sec.samsung.net/jira/browse/TNEXT-67
  menu_controller->restore_timer_ =
      ecore_timer_add(kRestoreTime, RestoreTimerCallback, menu_controller);
}

void ContextMenuControllerEfl::ContextMenuItemSelectedCallback(void* data,
    Evas_Object* obj, void* event_info) {
  Evas_Object* pop_up = obj;
  ContextMenuControllerEfl* menu_controller =
      static_cast<ContextMenuControllerEfl*>
      (evas_object_data_get(pop_up, "ContextMenuContollerEfl"));

  if (menu_controller) {
    ContextMenuItemEfl* selected_menu_item = static_cast<ContextMenuItemEfl*>(data);
    if (selected_menu_item)
       menu_controller->MenuItemSelected(selected_menu_item);
  }
}

void ContextMenuControllerEfl::RequestSelectionRect() const {
  RenderWidgetHostViewEfl* rwhv = static_cast<RenderWidgetHostViewEfl*>(
      web_contents_.GetRenderWidgetHostView());
  CHECK(rwhv);
  rwhv->RequestSelectionRect();
}

gfx::Point ContextMenuControllerEfl::CalculateSelectionMenuPosition(
    const gfx::Rect& selection_rect) {
  auto rwhv = static_cast<RenderWidgetHostViewEfl*>(
      web_contents_.GetRenderWidgetHostView());
  auto selection_controller = webview_->GetSelectionController();
  float device_scale_factor =
      display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  int visible_top_controls_height =
      rwhv->visible_top_controls_height() * device_scale_factor;
  int visible_bottom_controls_height = 0;
  if (visible_top_controls_height) {
    visible_bottom_controls_height =
        rwhv->GetVisibleBottomControlsHeightInPix();
  }

  auto visible_viewport_rect = selection_controller->GetVisibleViewportRect();
  auto forbidden_rect =
      selection_controller->GetForbiddenRegionRect(selection_rect);
  gfx::Point position;
  // Selection menu should always be placed at the centre of visible selection
  // area horizontally.
  position.set_x(forbidden_rect.CenterPoint().x());
  // If possible, selection menu should always be placed above forbidden
  // region.
  if (forbidden_rect.y() > visible_viewport_rect.y() + kMenuHeight) {
    position.set_y(forbidden_rect.y());
    return position;
  }

  if (forbidden_rect.y() >
      visible_viewport_rect.y() + kMenuHeight + visible_top_controls_height) {
    // If possible, selection menu should always be placed above forbidden
    // region.
    position.set_y(forbidden_rect.y() + visible_top_controls_height);
  } else if (forbidden_rect.bottom() + kMenuHeight +
                 visible_bottom_controls_height <
             visible_viewport_rect.bottom()) {
    // If there is no sufficient space above, we're trying to place selection
    // menu below forbidden region.
    position.set_y(forbidden_rect.bottom() + kMenuHeight);
  } else {
    // If there is no sufficient space above and below, selection menu will be
    // shown at the centre of visible selection area vertically.
    position.set_y(forbidden_rect.CenterPoint().y());
  }

  return position;
}

void ContextMenuControllerEfl::OnSelectionRectReceived(
    gfx::Rect selection_rect) {
  auto rwhv = static_cast<RenderWidgetHostViewEfl*>(
      web_contents_.GetRenderWidgetHostView());
  auto controller = webview_->GetSelectionController();

  if (IsMobileProfile() && (context_menu_status_ != INPROGRESS))
    return;
  if (!popup_ || !rwhv || !controller)
    return;

  // Consider a webview offset.
  selection_rect.Offset(rwhv->GetViewBoundsInPix().OffsetFromOrigin());

  auto visible_viewport_rect = controller->GetVisibleViewportRect();

  // In case of the caret mode selection_rect is empty
  if (!controller->IsCaretMode() &&
      gfx::IntersectRects(visible_viewport_rect, selection_rect).IsEmpty())
    return;

  gfx::Point selection_menu_position =
      CalculateSelectionMenuPosition(selection_rect);
  if (!visible_viewport_rect.Contains(selection_menu_position))
    return;

  evas_object_smart_callback_add(popup_, "dismissed", ContextMenuCancelCallback,
                                 this);

  if (IsMobileProfile())
    elm_ctxpopup_auto_hide_disabled_set(popup_, EINA_TRUE);

#if defined(OS_TIZEN)
  eext_object_event_callback_add(popup_, EEXT_CALLBACK_BACK,
                                 ContextMenuHWBackKey, this);
#endif
  evas_object_move(popup_, selection_menu_position.x(),
                   selection_menu_position.y());

  controller->ContextMenuStatusVisible();

  evas_object_show(popup_);
  evas_object_raise(popup_);
  if (IsMobileProfile())
    context_menu_status_ = VISIBLE;
}

bool ContextMenuControllerEfl::ShowContextMenu() {
  if (!popup_)
    return false;
  if (IsMobileProfile())
    context_menu_status_ = INPROGRESS;

  if (is_text_selection_) {
    // Selection menu will be shown in OnSelectionRectReceived after receiving
    // selection rectangle form the renderer.
    RequestSelectionRect();
  } else {
    evas_object_smart_callback_add(popup_, "block,clicked",
                                   BlockClickedCallback, this);
#if defined(OS_TIZEN)
    eext_object_event_callback_add(popup_, EEXT_CALLBACK_BACK, ContextMenuHWBackKey, this);
    eext_object_event_callback_add(popup_, EEXT_CALLBACK_MORE, ContextMenuHWBackKey, this);
#endif
    evas_object_show(popup_);
    evas_object_raise(popup_);
    if (IsMobileProfile())
      context_menu_status_ = VISIBLE;
  }
  return true;
}

void ContextMenuControllerEfl::HideSelectionHandle() {
  SelectionControllerEfl* controller = webview_->GetSelectionController();
  if (controller)
    controller->HideHandles();
}

void ContextMenuControllerEfl::ClearSelection() {
  auto controller = webview_->GetSelectionController();
  if (controller)
    controller->ClearSelection();
}

void ContextMenuControllerEfl::RequestShowSelectionHandleAndContextMenu(
    bool trigger_selection_change) {
  SelectionControllerEfl* controller = webview_->GetSelectionController();
  if (controller) {
    controller->SetSelectionStatus(true);
    controller->SetWaitsForRendererSelectionChanges(
        SelectionControllerEfl::Reason::RequestedByContextMenu);
    if (trigger_selection_change)
      controller->TriggerOnSelectionChange();
  }
}

void ContextMenuControllerEfl::OnClipboardDownload(
    content::DownloadItem* item,
    content::DownloadInterruptReason interrupt_reason) {
  item->SetUserData(DownloadManagerDelegateEfl::kDownloadTemporaryFile,
                    base::WrapUnique(new base::SupportsUserData::Data()));
  item->AddObserver(this);
  clipboard_download_items_.insert(item);
}

void EmptyDialogClosedCallback(
    bool /* success */,
    const base::string16& /* user_input */) {
}

void ContextMenuControllerEfl::OnDiskDownload(
    content::DownloadItem* item,
    content::DownloadInterruptReason interrupt_reason) {
  if (!item) {
    save_fail_dialog_.reset(JavaScriptModalDialogEfl::CreateDialogAndShow(
        &web_contents_, JavaScriptModalDialogEfl::ALERT,
        base::UTF8ToUTF16(std::string(dgettext("WebKit", "IDS_BR_POP_FAIL"))),
        base::string16(), base::Bind(&EmptyDialogClosedCallback)));
    return;
  }
  item->AddObserver(this);
  disk_download_items_.insert(item);
}

void ContextMenuControllerEfl::OnDownloadUpdated(content::DownloadItem* download) {
  if(download && download->AllDataSaved()) {
    if (clipboard_download_items_.find(download) != clipboard_download_items_.end()) {
      const std::string& download_path = download->GetForcedFilePath().value();
      ClipboardHelperEfl::GetInstance()->SetData(download_path,
                                                 ClipboardDataTypeEfl::IMAGE);
      download->RemoveObserver(this);
      clipboard_download_items_.erase(download);
    }
    if (disk_download_items_.find(download) != disk_download_items_.end()) {
      file_saved_dialog_.reset(JavaScriptModalDialogEfl::CreateDialogAndShow(
          &web_contents_, JavaScriptModalDialogEfl::ALERT,
          base::UTF8ToUTF16(
              std::string(dgettext("WebKit", "IDS_BR_POP_SAVED"))),
          base::string16(), base::Bind(&EmptyDialogClosedCallback)));
      download->RemoveObserver(this);
      disk_download_items_.erase(download);
    }
  }
}

base::FilePath ContextMenuControllerEfl::DownloadFile(
    const GURL url,
    const base::FilePath outputDir,
    const base::FilePath suggested_filename,
    const DownloadUrlParameters::OnStartedCallback& callback =
        DownloadUrlParameters::OnStartedCallback()) {
  LOG(INFO) << "Downloading file: " << url << "to: "<< outputDir.value();
  const GURL referrer = web_contents_.GetVisibleURL();
  DownloadManager* dlm = BrowserContext::GetDownloadManager(
      web_contents_.GetBrowserContext());

  std::unique_ptr<DownloadUrlParameters> dl_params(
      DownloadUrlParameters::CreateForWebContentsMainFrame(&web_contents_, url, NO_TRAFFIC_ANNOTATION_YET));
  dl_params->set_post_id(-1);
  dl_params->set_referrer(
      content::Referrer(referrer, blink::kWebReferrerPolicyAlways));
  dl_params->set_referrer_encoding("utf8");
  base::FilePath fileName;
  if (suggested_filename.empty())
    fileName = net::GenerateFileName(url, "", "", "", "", "");
  else
    fileName = suggested_filename;
  base::FilePath fullPath = outputDir.Append(fileName);

  while (PathExists(fullPath)) {
    unsigned int i;
    base::FilePath fileNameTmp;
    for (i = 0; PathExists(fullPath) && i <= 999; i++) {
      char buffer[6];
      snprintf(buffer, sizeof(buffer), "(%d)", i);
      fileNameTmp = fileName.InsertBeforeExtension(std::string(buffer));
      fullPath = outputDir.Append(fileNameTmp);
    }
  }

  dl_params->set_suggested_name(base::UTF8ToUTF16(fileName.value()));
  dl_params->set_file_path(fullPath);
  dl_params->set_prompt(true);
  dl_params->set_callback(callback);
  dlm->DownloadUrl(std::move(dl_params));
  return fullPath;
}

bool ContextMenuControllerEfl::TriggerDownloadCb(const GURL url) {
  BrowserContextEfl* browser_context =
      static_cast<BrowserContextEfl*>(web_contents_.GetBrowserContext());
  if (browser_context) {
    EwkDidStartDownloadCallback* start_download_callback =
        browser_context->WebContext()->DidStartDownloadCallback();
    if (start_download_callback) {
      DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
      start_download_callback->TriggerCallback(url.spec());
      return true;
    }
  }
  return false;
}

void ContextMenuControllerEfl::OpenURL(
    const GURL url,
    const WindowOpenDisposition disposition) {
  if (!url.is_valid())
    return;

  content::OpenURLParams params(url,
                                content::Referrer(
                                    web_contents_.GetVisibleURL(),
                                    blink::kWebReferrerPolicyAlways),
                                -1, /* -1 to indicate the main frame */
                                disposition,
                                ui::PAGE_TRANSITION_LINK,
                                false);

  web_contents_.GetDelegate()->OpenURLFromTab(&web_contents_, params);
}

#if defined(OS_TIZEN)
static void destroy_app_handle(app_control_h* handle) {
  ignore_result(app_control_destroy(*handle));
}

#if defined(OS_TIZEN_MOBILE)
void ContextMenuControllerEfl::LaunchBrowserWithWebSearch(
    const std::string& text) {
  app_control_h app_control;
  app_control_create(&app_control);

  app_control_set_operation(app_control, APP_CONTROL_OPERATION_SEARCH);
#if defined(OS_TIZEN_MOBILE_PRODUCT)
  app_control_add_extra_data(
      app_control, "http://tizen.org/appcontrol/data/keyword", text.c_str());
#else
  app_control_add_extra_data(app_control, APP_CONTROL_DATA_TEXT, text.c_str());
#endif
  app_control_send_launch_request(app_control, NULL, NULL);

  app_control_destroy(app_control);
}
#endif

void ContextMenuControllerEfl::LaunchShareApp(const std::string& text) {
  app_control_h handle = nullptr;
  int error_code = app_control_create(&handle);
  if (error_code < 0 || !handle) {
    LOG(ERROR) << __PRETTY_FUNCTION__
               << " : Failed to create share service. Error code: "
               << error_code;
    return;
  }

  std::unique_ptr<app_control_h, decltype(&destroy_app_handle)> handle_ptr(
      &handle, destroy_app_handle);

  error_code =
      app_control_set_operation(handle, APP_CONTROL_OPERATION_SHARE_TEXT);
  if (error_code < 0) {
    LOG(ERROR) << __PRETTY_FUNCTION__
               << ": Failed to set share text operation. Error code: "
               << error_code;
    return;
  }

  error_code =
      app_control_add_extra_data(handle, APP_CONTROL_DATA_TEXT, text.c_str());
  if (error_code < 0) {
    LOG(ERROR) << __PRETTY_FUNCTION__
               << ": Failed to add data for share application. Error code: "
               << error_code;
    return;
  }

  error_code = app_control_send_launch_request(handle, nullptr, nullptr);
  if (error_code < 0) {
    LOG(ERROR) << __PRETTY_FUNCTION__
               << ": Failed to send launch request for share application. "
               << "Error code: " << error_code;
    return;
  }
}
#endif

void ContextMenuControllerEfl::ContextMenuPopupResize(void* data,
                                                      Evas* e,
                                                      Evas_Object* obj,
                                                      void* event_info) {
  auto thiz = static_cast<ContextMenuControllerEfl*>(data);
  if (thiz)
    thiz->ContextMenuPopupListResize();
}

void ContextMenuControllerEfl::BlockClickedCallback(void* data, Evas_Object*, void*)
{
  ContextMenuControllerEfl* menu_controller = static_cast<ContextMenuControllerEfl*>(data);

  if (!menu_controller)
    return;

  menu_controller->HideContextMenu();
}

void ContextMenuControllerEfl::ContextMenuPopupListResize() {
  if (!IsMobileProfile())
    return;

  int orientation = webview_->GetOrientation();
  if ((orientation == 90 || orientation == 270) &&
      _context_menu_listdata.size() > 3)
    evas_object_size_hint_min_set(list_, 0, kMinHeightHorizontal);
  else if ((orientation == 0 || orientation == 180) &&
           _context_menu_listdata.size() > 7)
    evas_object_size_hint_min_set(list_, 0, kMinHeightVertical);
  else
    evas_object_size_hint_min_set(
        list_, 0, _context_menu_listdata.size() * _popup_item_height);

  evas_object_show(list_);
}

#if defined(TIZEN_CONTEXT_MENU_QUICK_MEMO)
// TODO(swarali.sr) : When Enabling TIZEN_CONTEXT_MENU_QUICK_MEMO flag ,
// also add changes to build files from
// https://review.tizen.org/gerrit/#/c/132436.Those changes are omitted at the
// moment, because of dependancy on product flags.

void ContextMenuControllerEfl::QuickMemoContextMenuSelected() {
  const char* selectedText = webview_->CacheSelectedText();
  if (strlen(selectedText) == 0)
    return;

  bundle* b = bundle_create();
  if (!b) {
    LOG(ERROR) << "Could not create service!!";
    return;
  }

  appsvc_set_operation(b, kQuickMemoOperation);
  appsvc_set_appid(b, kQuickMemoAppId);

  appsvc_add_data(b, kQuickMemoAppData, selectedText);
  appsvc_add_data(b, "text_type", "plain_text");

  appsvc_run_service(b, 0, nullptr, nullptr);

  bundle_free(b);
}
#endif

void ContextMenuControllerEfl::CustomMenuItemSelected(ContextMenuItemEfl* menu_item) {
  _Ewk_Context_Menu_Item item(
      new ContextMenuItemEfl(menu_item->GetContextMenuOptionType(),
                             menu_item->GetContextMenuOption(),
                             menu_item->Title(), params_.src_url.spec(),
                             params_.link_url.spec(), std::string()),
      menu_items_parent_.get());
  webview_->SmartCallback<EWebViewCallbacks::ContextMenuItemSelected>().call(&item);
}

void ContextMenuControllerEfl::MenuItemSelected(ContextMenuItemEfl* menu_item)
{
  if (!menu_item)
    return;

  if (!webview_)
    return;

  HideContextMenu();

  // FIXME: Add cases as and when required
  switch(menu_item->GetContextMenuOption())
  {
    case EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK: {
      OpenURL(GURL(params_.link_url.spec()), WindowOpenDisposition::CURRENT_TAB);
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW: {
      OpenURL(GURL(params_.link_url.spec()), WindowOpenDisposition::NEW_FOREGROUND_TAB);
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_BACKGROUND: {
      if (IsMobileProfile())
        OpenURL(GURL(params_.link_url.spec()),
                WindowOpenDisposition::NEW_BACKGROUND_TAB);
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_SHARE: {
#if defined(OS_TIZEN)
      LaunchShareApp(base::UTF16ToUTF8(params_.selection_text));
      webview_->ExecuteEditCommand(kEditCommandUnselect, nullptr);
#else
      NOTIMPLEMENTED();
#endif
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_GO_BACK: {
      webview_->GoBack();
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_GO_FORWARD: {
      webview_->GoForward();
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_RELOAD: {
      webview_->ReloadBypassingCache();
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_COPY: {
      webview_->ExecuteEditCommand("copy", NULL);
      if (params_.is_editable) {
        HideSelectionHandle();
        Eina_Rectangle left_rect, right_rect;
        webview_->GetSelectionRange(&left_rect, &right_rect);
        webview_->MoveCaret(gfx::Point(right_rect.x, right_rect.y));
      } else {
        webview_->ExecuteEditCommand(kEditCommandUnselect, nullptr);
      }
      break;
    }
#if defined(TIZEN_CONTEXT_MENU_QUICK_MEMO)
    case EWK_CONTEXT_MENU_ITEM_TAG_QUICKMEMO:
      QuickMemoContextMenuSelected();
      webview_->ExecuteEditCommand(kEditCommandUnselect, nullptr);
      break;
#endif
    case EWK_CONTEXT_MENU_ITEM_TAG_TEXT_SELECTION_MODE: {
      webview_->SelectFocusedLink();
      RequestShowSelectionHandleAndContextMenu();
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_TO_CLIPBOARD: {
      // FIXME(g.ludwikowsk): Find the correct path for temporarily saving
      // images to clipboard.
      base::FilePath copy_image_dir("/tmp/");
      base::FilePath suggested_filename;
      if (params_.src_url.has_query()) {
        suggested_filename = base::FilePath(net::UnescapeURLComponent(
            params_.src_url.query(),
            net::UnescapeRule::SPACES | net::UnescapeRule::PATH_SEPARATORS |
                net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS));
      } else {
        suggested_filename = net::GenerateFileName(GURL(params_.src_url.spec()),
                                                   "", "", "", "", "");
      }

      if (suggested_filename.Extension().empty()) {
        suggested_filename =
            suggested_filename.AddExtension(kDefaultImageExtension);
      }

      base::FilePath fullPath = copy_image_dir.Append(suggested_filename);
      // For files with the same name clipboard app refuses to add another one,
      // saying that it is already copied to clipboard. When DownloadFile
      // tries to download a file which already exists, it appends a number
      // to the file name. This causes clipboard app to recognize it as a
      // different file and clipboard is filled with duplicate images.
      // To prevent this, we do not download file if a file with the same name
      // already exists. We still call SetData, so clipboard app would show
      // toast popup for duplicates. This is for Tizen 2.4 compatibility.
      if (PathExists(fullPath)) {
        ClipboardHelperEfl::GetInstance()->SetData(fullPath.value(),
                                                   ClipboardDataTypeEfl::IMAGE);
      } else {
        DownloadFile(GURL(params_.src_url.spec()), copy_image_dir,
                     suggested_filename,
                     base::Bind(&ContextMenuControllerEfl::OnClipboardDownload,
                                weak_ptr_factory_.GetWeakPtr()));
      }
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_DATA: {
      ClipboardHelperEfl::GetInstance()->SetData(
          base::UTF16ToUTF8(params_.link_text), ClipboardDataTypeEfl::URL);
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD: {
      ClipboardHelperEfl::GetInstance()->SetData(params_.link_url.spec(),
                                                 ClipboardDataTypeEfl::URL);
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_LINK_TO_DISK: {
      if (!TriggerDownloadCb(GURL(params_.link_url.spec()))) {
        base::FilePath path;
        if (!PathService::Get(PathsEfl::DIR_DOWNLOADS, &path)) {
          LOG(ERROR) << "Could not get downloads directory.";
          break;
        }
        DownloadFile(GURL(params_.link_url.spec()), path, base::FilePath(),
                     base::Bind(&ContextMenuControllerEfl::OnDiskDownload,
                                weak_ptr_factory_.GetWeakPtr()));
      }
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_IMAGE_TO_DISK: {
      if (!TriggerDownloadCb(GURL(params_.src_url.spec()))) {
        base::FilePath path;
        if (!PathService::Get(PathsEfl::DIR_DOWNLOAD_IMAGE, &path)) {
          LOG(ERROR) << "Could not get image downloads directory.";
          break;
        }
        DownloadFile(GURL(params_.src_url.spec()), path, base::FilePath(),
                     base::Bind(&ContextMenuControllerEfl::OnDiskDownload,
                                weak_ptr_factory_.GetWeakPtr()));
      }
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_CURRENT_WINDOW: {
      OpenURL(GURL(params_.src_url.spec()), WindowOpenDisposition::CURRENT_TAB);
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_NEW_WINDOW: {
      OpenURL(GURL(params_.src_url.spec()), WindowOpenDisposition::NEW_FOREGROUND_TAB);
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_SELECT_WORD: {
      webview_->ExecuteEditCommand("SelectWord", NULL);
      RequestShowSelectionHandleAndContextMenu();
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL: {
      webview_->ExecuteEditCommand("SelectAll", NULL);
      // Trigger OnSelectionChange manually for non-editable selection
      RequestShowSelectionHandleAndContextMenu(!params_.is_editable);
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_PASTE: {
      webview_->ExecuteEditCommand("paste", NULL);
      ClearSelection();
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_CUT: {
      webview_->ExecuteEditCommand("cut", NULL);
      ClearSelection();
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_WEB: {
      std::string search_text = base::UTF16ToUTF8(params_.selection_text);
      webview_->ClearSelection();
#if defined(OS_TIZEN_MOBILE)
      LaunchBrowserWithWebSearch(search_text);
#else
      OpenURL(GURL(kWebSearchLink + search_text),
              WindowOpenDisposition::NEW_FOREGROUND_TAB);
#endif
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_CLIPBOARD: {
      // TODO: set true for richly editable.
      // Paste operations are blocked by Editor if content is not richly editible.
      // Need to find if webview_ has rich editor.
      ClipboardHelperEfl::GetInstance()->OpenClipboardWindow(webview_, true);
      break;
    }
    case EWK_CONTEXT_MENU_ITEM_TAG_DRAG: {
#if defined(OS_TIZEN)
      if (IsMobileProfile()) {
        webview_->EnterDragState();
        context_menu_show_pos_.SetPoint(params_.x, params_.y);
      } else {
        NOTIMPLEMENTED();
      }
#endif
      break;
    }
    default:
      CustomMenuItemSelected(menu_item);
  }
}

void ContextMenuControllerEfl::HideContextMenu() {
#if defined(OS_TIZEN_TV_PRODUCT)
  LOG(INFO) << "Context Menu Hide";
  webview_->SmartCallback<EWebViewCallbacks::ContextMenuHide>().call(
      menu_items_parent_.get());
#endif

  if (IsMobileProfile() &&
      (context_menu_status_ == HIDDEN || context_menu_status_ == NONE))
    return;

  if (is_text_selection_) {
    SelectionControllerEfl* controller = webview_->GetSelectionController();
    if (controller)
      controller->ContextMenuStatusHidden();
  }

  if (popup_) {
    if (IsMobileProfile() || IsWearableProfile()) {
      evas_object_hide(popup_);
    } else {
      evas_object_event_callback_del(popup_, EVAS_CALLBACK_RESIZE,
                                     ContextMenuPopupResize);
      evas_object_del(popup_);
      popup_ = nullptr;
    }
  }

  if (restore_timer_) {
    ecore_timer_del(restore_timer_);
    restore_timer_ = nullptr;
  }

  if (IsMobileProfile()) {
    context_menu_status_ = HIDDEN;
  } else if (menu_items_) {
    void* data;
    EINA_LIST_FREE(menu_items_, data) {
      _Ewk_Context_Menu_Item* item = static_cast<_Ewk_Context_Menu_Item*>(data);
      delete item;
    }
    menu_items_ = nullptr;
  }
  menu_items_parent_.reset();
}

void ContextMenuControllerEfl::AppendItem(const _Ewk_Context_Menu_Item* item) {
  menu_items_ = eina_list_append(menu_items_, item);
}

void ContextMenuControllerEfl::RemoveItem(const _Ewk_Context_Menu_Item* item) {
  menu_items_ = eina_list_remove(menu_items_, item);
  delete item;
}

void ContextMenuControllerEfl::Resize(const gfx::Rect& webview_rect) {
  if (save_fail_dialog_)
    save_fail_dialog_->SetPopupSize(webview_rect.width(),
                                    webview_rect.height());
  if (file_saved_dialog_)
    file_saved_dialog_->SetPopupSize(webview_rect.width(),
                                     webview_rect.height());

  if (!popup_)
    return;

  if (IsMobileProfile() && context_menu_status_ != VISIBLE)
    return;

  if (is_text_selection_) {
    RequestSelectionRect();
  } else {
    evas_object_resize(popup_, webview_rect.width(), webview_rect.height());
  }
}

void ContextMenuControllerEfl::DeletePopup() {
  if (top_widget_)
    evas_object_data_set(top_widget_, "ContextMenuContollerEfl", 0);

  if (is_text_selection_) {
    SelectionControllerEfl* controller = webview_->GetSelectionController();
    if (controller)
      controller->ContextMenuStatusHidden();
  }

  if (popup_) {
    evas_object_event_callback_del(popup_, EVAS_CALLBACK_RESIZE,
                                   ContextMenuPopupResize);
    evas_object_data_set(popup_, "ContextMenuContollerEfl", 0);
    if (!IsMobileProfile())
      evas_object_smart_member_del(popup_);
    evas_object_del(popup_);
    popup_ = nullptr;
    list_ = nullptr;
  }

  if (menu_items_) {
    void* data;
    EINA_LIST_FREE(menu_items_, data) {
      _Ewk_Context_Menu_Item *item = static_cast<_Ewk_Context_Menu_Item*> (data);
      delete item;
    }
    menu_items_ = nullptr;
  }
  if (IsMobileProfile())
    context_menu_status_ = NONE;
  menu_items_parent_.reset();
}
}
