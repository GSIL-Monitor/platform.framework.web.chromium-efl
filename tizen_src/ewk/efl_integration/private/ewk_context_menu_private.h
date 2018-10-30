// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_context_menu_private_h
#define ewk_context_menu_private_h

#include <Eina.h>
#include <stdio.h>

#include "context_menu_controller_efl.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "ui/display/screen.h"
#endif

struct _Ewk_Context_Menu_Item {
 public:
  _Ewk_Context_Menu_Item(content::ContextMenuItemEfl* menu_item,
                         _Ewk_Context_Menu* parent_menu)
      : menu_item_(menu_item), parent_menu_(parent_menu) {}

  ~_Ewk_Context_Menu_Item() {
    delete menu_item_;
    menu_item_ = nullptr;
  }

  content::ContextMenuItemEfl* GetMenuItem() const {
    return menu_item_;
  }

  _Ewk_Context_Menu* ParentMenu() const { return parent_menu_; }

 private:
  content::ContextMenuItemEfl* menu_item_;
  _Ewk_Context_Menu* parent_menu_;
};


// Wrapper for context_menu_controller items
struct _Ewk_Context_Menu {
 public:
  _Ewk_Context_Menu(content::ContextMenuControllerEfl* controller)
#if defined(OS_TIZEN_TV_PRODUCT)
      : controller_(controller), position_(0, 0) {
#else
      : controller_(controller) {
#endif
  }

  const Eina_List* GetMenuList() const {
    if (controller_)
      return controller_->GetMenuItems();
    else
      return nullptr;
  }

  content::ContextMenuControllerEfl* GetController() const {
    return controller_;
  }

  void AppendItem(const _Ewk_Context_Menu_Item* item) {
    if (controller_)
      controller_->AppendItem(item);
  }

  void RemoveItem(const _Ewk_Context_Menu_Item* item) {
    if (controller_)
      controller_->RemoveItem(item);
  }

  void MenuItemSelected(const _Ewk_Context_Menu_Item* item) {
    content::ContextMenuItemEfl* info_item = item->GetMenuItem();
    if (controller_)
      controller_->MenuItemSelected(info_item);
  }

  void HideContextMenu() {
    if (controller_)
      controller_->HideContextMenu();
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  void SetPosition(int x, int y) {
    position_.set_x(x);
    position_.set_y(y);
  }

  gfx::Point GetPosition() { return position_; }
#endif

 private:
  content::ContextMenuControllerEfl* controller_;
#if defined(OS_TIZEN_TV_PRODUCT)
  gfx::Point position_;
#endif
};

#endif // ewk_context_menu_private_h
