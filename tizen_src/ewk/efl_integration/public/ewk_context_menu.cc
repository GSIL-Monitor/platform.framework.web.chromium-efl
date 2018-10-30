// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk_context_menu_product.h"

#include <Eina.h>
#include "private/ewk_private.h"
#include "private/ewk_context_menu_private.h"
#include "context_menu_controller_efl.h"

unsigned ewk_context_menu_item_count(Ewk_Context_Menu* menu)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(menu, 0);
  return eina_list_count(menu->GetMenuList());  // LCOV_EXCL_LINE
}

Ewk_Context_Menu_Item* ewk_context_menu_nth_item_get(Ewk_Context_Menu* menu, unsigned int n)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(menu, NULL);
  return static_cast<Ewk_Context_Menu_Item*>(
      eina_list_nth(menu->GetMenuList(), n));  // LCOV_EXCL_LINE;
}

/* LCOV_EXCL_START */
Eina_Bool ewk_context_menu_item_remove(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(menu, false);
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);

  menu->RemoveItem(item);
  return true;
}

Eina_Bool ewk_context_menu_item_append_as_action(Ewk_Context_Menu* menu,
    Ewk_Context_Menu_Item_Tag tag, const char* title, Eina_Bool enabled)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(menu, false);
  EINA_SAFETY_ON_NULL_RETURN_VAL(title, false);
  content::ContextMenuItemEfl *new_item = new content::ContextMenuItemEfl(
      EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
      tag,
      std::string(title));
  new_item->SetEnabled(enabled);
  _Ewk_Context_Menu_Item *item = new _Ewk_Context_Menu_Item(new_item, menu);
  menu->AppendItem(item);
  return true;
}


Eina_Bool ewk_context_menu_item_append(Ewk_Context_Menu* menu,
    Ewk_Context_Menu_Item_Tag tag, const char* title, const char* icon_file,
    Eina_Bool enabled)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(menu, false);
  EINA_SAFETY_ON_NULL_RETURN_VAL(title, false);
  EINA_SAFETY_ON_NULL_RETURN_VAL(icon_file, false);

  content::ContextMenuItemEfl *new_item = new content::ContextMenuItemEfl(
      EWK_CONTEXT_MENU_ITEM_TYPE_ACTION,
      tag,
      std::string(title),
      std::string(),
      std::string(),
      std::string(icon_file));
  new_item->SetEnabled(enabled);
  _Ewk_Context_Menu_Item *item = new _Ewk_Context_Menu_Item(new_item, menu);
  menu->AppendItem(item);
  return true;
}

Ewk_Context_Menu_Item_Tag ewk_context_menu_item_tag_get(Ewk_Context_Menu_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, 0);
  content::ContextMenuItemEfl *info_item = item->GetMenuItem();
  return static_cast<Ewk_Context_Menu_Item_Tag>(info_item->GetContextMenuOption());
}

Ewk_Context_Menu_Item_Type ewk_context_menu_item_type_get(Ewk_Context_Menu_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, EWK_CONTEXT_MENU_ITEM_TYPE_ACTION);
  content::ContextMenuItemEfl *info_item = item->GetMenuItem();
  return static_cast<Ewk_Context_Menu_Item_Type>(info_item->GetContextMenuOptionType());
}

Eina_Bool ewk_context_menu_item_enabled_get(const Ewk_Context_Menu_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, false);
  content::ContextMenuItemEfl *info_item = item->GetMenuItem();
  return info_item->IsEnabled();
}
/* LCOV_EXCL_STOP */

const char* ewk_context_menu_item_link_url_get(Ewk_Context_Menu_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);
  /* LCOV_EXCL_START */
  content::ContextMenuItemEfl *info_item = item->GetMenuItem();
  return info_item->LinkURL().c_str();
  /* LCOV_EXCL_STOP */
}

const char* ewk_context_menu_item_image_url_get(Ewk_Context_Menu_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);
  /* LCOV_EXCL_START */
  content::ContextMenuItemEfl *info_item = item->GetMenuItem();
  return info_item->ImageURL().c_str();
  /* LCOV_EXCL_STOP */
}

/* LCOV_EXCL_START */
const char* ewk_context_menu_item_title_get(const Ewk_Context_Menu_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);
  if (item->GetMenuItem())
    return item->GetMenuItem()->Title().c_str();

  return NULL;
}

Eina_Bool ewk_context_menu_item_checked_get(const Ewk_Context_Menu_Item* item)
{
  LOG_EWK_API_MOCKUP();
  return false;
}

Ewk_Context_Menu* ewk_context_menu_item_submenu_get(const Ewk_Context_Menu_Item* item)
{
  LOG_EWK_API_MOCKUP();
  return NULL;
}

const Eina_List* ewk_context_menu_items_get(const Ewk_Context_Menu* menu)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(menu, NULL);
  return menu->GetMenuList();
}

Ewk_Context_Menu* ewk_context_menu_item_parent_menu_get(const Ewk_Context_Menu_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, nullptr);
  return item->ParentMenu();
}

Eina_Bool ewk_context_menu_item_select(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(menu, EINA_FALSE);
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, EINA_FALSE);
  menu->MenuItemSelected(item);
  return EINA_TRUE;
}

Eina_Bool ewk_context_menu_hide(Ewk_Context_Menu* menu)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(menu, EINA_FALSE);
  menu->HideContextMenu();
  return EINA_TRUE;
}

int ewk_context_menu_pos_x_get(Ewk_Context_Menu* menu)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(menu, 0);
  return menu->GetPosition().x();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
  return 0;
#endif
}

int ewk_context_menu_pos_y_get(Ewk_Context_Menu* menu)
{
#if defined(OS_TIZEN_TV_PRODUCT)
  EINA_SAFETY_ON_NULL_RETURN_VAL(menu, 0);
  return menu->GetPosition().y();
#else
  LOG_EWK_API_MOCKUP("Only for Tizen TV.");
  return 0;
#endif
}
/* LCOV_EXCL_STOP */
