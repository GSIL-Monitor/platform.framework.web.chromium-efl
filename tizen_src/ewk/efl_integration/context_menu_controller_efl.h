// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef context_menu_controller_h
#define context_menu_controller_h

#include <Evas.h>
#include <set>

#include "base/memory/weak_ptr.h"
#include "browser/javascript_modal_dialog_efl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_url_parameters.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/context_menu_params.h"
#include "public/ewk_context_menu_internal.h"

class EWebView;

namespace gfx {
class Rect;
}

namespace content {

class ContextMenuItemEfl {
 public:
  ContextMenuItemEfl(Ewk_Context_Menu_Item_Type item,
                     Ewk_Context_Menu_Item_Tag option,
                     const std::string& title,
                     const std::string& image_url = std::string(),
                     const std::string& link_url = std::string(),
                     const std::string& icon_path = std::string())
    : menu_type_(item)
    , menu_option_(option)
    , title_(title)
    , is_enabled_(true)
    , image_url_(image_url)
    , link_url_(link_url)
    , icon_path_(icon_path) {
  }

  ~ContextMenuItemEfl() { }

  const std::string& Title() const { return title_; }
  void SetTitle(const std::string& title) { title_ = title; }

  bool IsEnabled() const { return is_enabled_; }
  void SetEnabled(bool status) { is_enabled_ = status; }
  Ewk_Context_Menu_Item_Tag GetContextMenuOption() const { return menu_option_; }
  Ewk_Context_Menu_Item_Type GetContextMenuOptionType() const { return menu_type_; }
  const std::string& LinkURL() const { return link_url_; }
  const std::string& ImageURL() const { return image_url_; }
  const std::string& IconPath() const { return icon_path_; }

 private:
  Ewk_Context_Menu_Item_Type menu_type_;
  Ewk_Context_Menu_Item_Tag menu_option_;
  std::string title_;
  bool is_enabled_;
  std::string image_url_;
  std::string link_url_;
  std::string icon_path_;
};

class ContextMenuControllerEfl
    : public content::DownloadItem::Observer {
 public:
  static void ContextMenuCancelCallback(void* data, Evas_Object* obj, void* event_info);
  static void ContextMenuItemSelectedCallback(void* data, Evas_Object* obj, void* event_info);
  static void ContextMenuHWBackKey(void* data, Evas_Object* obj, void* event_info);

  ContextMenuControllerEfl(EWebView* wv, WebContents& web_contents);
  ~ContextMenuControllerEfl();

  bool PopulateAndShowContextMenu(const ContextMenuParams& params);
  void Move(int x, int y);
  void MenuItemSelected(ContextMenuItemEfl* menu_item);
  Eina_List* GetMenuItems() const { return menu_items_; }
  void HideContextMenu();
  gfx::Point GetContextMenuShowPos() const { return context_menu_show_pos_; };
  void Resize(const gfx::Rect& webview_rect);

  void AppendItem(const _Ewk_Context_Menu_Item* item);
  void RemoveItem(const _Ewk_Context_Menu_Item* item);

  void RequestSelectionRect() const;
  void OnSelectionRectReceived(gfx::Rect selection_rect);
  gfx::Point CalculateSelectionMenuPosition(const gfx::Rect& selection_rect);

 private:
  enum ContextMenuStatusTag { NONE = 0, HIDDEN, INPROGRESS, VISIBLE };
  static void ContextMenuPopupResize(void* data, Evas* e, Evas_Object* obj, void* event_info);
  static void BlockClickedCallback(void*, Evas_Object*, void*);
  void ContextMenuPopupListResize();
  void CustomMenuItemSelected(ContextMenuItemEfl*);
#if defined(TIZEN_CONTEXT_MENU_QUICK_MEMO)
  void QuickMemoContextMenuSelected();
#endif
  void GetProposedContextMenu();
  bool CreateContextMenu(const ContextMenuParams& params);
  bool ShowContextMenu();
  void AddItemToProposedList(Ewk_Context_Menu_Item_Type item,
                             Ewk_Context_Menu_Item_Tag option,
                             const std::string& title,
                             const std::string& image_url = std::string(),
                             const std::string& link_url = std::string(),
                             const std::string& icon_path = std::string());
  void HideSelectionHandle();
  void ClearSelection();
  void RequestShowSelectionHandleAndContextMenu(
      bool trigger_selection_change = false);
  virtual void OnDownloadUpdated(content::DownloadItem* download) override;
  void OnClipboardDownload(content::DownloadItem* item,
                           content::DownloadInterruptReason interrupt_reason);
  void OnDiskDownload(content::DownloadItem* item,
                      content::DownloadInterruptReason interrupt_reason);
  base::FilePath DownloadFile(
      const GURL url,
      const base::FilePath outputDir,
      const base::FilePath suggested_filename,
      const DownloadUrlParameters::OnStartedCallback& callback);
  bool TriggerDownloadCb(const GURL url);
  void OpenURL(const GURL url, const WindowOpenDisposition disposition);
#if defined(OS_TIZEN_MOBILE)
  void LaunchBrowserWithWebSearch(const std::string& text);
#endif
#if defined(OS_TIZEN)
  void LaunchShareApp(const std::string& text);
#endif
  void DeletePopup();

  static Eina_Bool RestoreTimerCallback(void* data);

  static std::vector<ContextMenuItemEfl> _context_menu_listdata;
  static int _popup_item_height;
  static bool _context_menu_resized;
  EWebView* webview_;
  Evas_Object* native_view_;
  Evas_Object* top_widget_;
  Evas_Object* popup_;
  Evas_Object* list_;
  Eina_List* menu_items_;
  std::unique_ptr<_Ewk_Context_Menu> menu_items_parent_;
  ContextMenuParams params_;
  WebContents& web_contents_;
  base::WeakPtrFactory<ContextMenuControllerEfl> weak_ptr_factory_;
  gfx::Point context_menu_show_pos_;
  bool is_text_selection_;
  std::set<DownloadItem*> clipboard_download_items_;
  std::set<DownloadItem*> disk_download_items_;
  std::unique_ptr<JavaScriptModalDialogEfl> file_saved_dialog_;
  std::unique_ptr<JavaScriptModalDialogEfl> save_fail_dialog_;
  Ecore_Timer* restore_timer_;
  ContextMenuStatusTag context_menu_status_;
};

} // namespace
#endif // context_menu_controller_h
