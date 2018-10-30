// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/select_picker/select_picker_util.h"

GenlistCallbackData::GenlistCallbackData(
    int index,
    SelectPickerBase* select_picker,
    content::MenuItem menu_item,
    Evas_Object* genlist,
    const Elm_Genlist_Item_Class* item_class,
    const Elm_Genlist_Item_Class* group_class,
    bool is_multiple_selection,
    Evas_Smart_Cb item_selected_cb
#if defined(TIZEN_ATK_SUPPORT)
    ,
    bool atk_enabled
#endif
    )
    : index_(index),
      select_picker_(select_picker),
      menu_item_(menu_item)
#if defined(TIZEN_ATK_SUPPORT)
      ,
      atk_enabled_(atk_enabled)
#endif
{
  bool is_option_type = (menu_item.type == content::MenuItem::OPTION);
  elm_item_ = elm_genlist_item_append(
      genlist, is_option_type ? item_class : group_class, this, nullptr,
      ELM_GENLIST_ITEM_NONE, is_multiple_selection ? nullptr : item_selected_cb,
      is_multiple_selection ? nullptr : this);
  if (!is_option_type)
    elm_genlist_item_select_mode_set(elm_item_,
                                     ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
  if (is_multiple_selection && menu_item.checked)
    elm_genlist_item_selected_set(elm_item_, EINA_TRUE);
  elm_object_item_disabled_set(elm_item_, !menu_item.enabled);
}

GenlistCallbackData::~GenlistCallbackData() {
#if defined(TIZEN_ATK_SUPPORT)
  // TODO(bj1987.kim): This code prevents the elm_genlist from crashing
  // when ATK is turned on. After fixing genlist crash, need to remove.
  if (!atk_enabled_ && elm_item_)
#else
  if (elm_item_)
#endif
    elm_object_item_del(elm_item_);
}

void GenlistCallbackData::SetSelection(bool selected) {
  elm_genlist_item_selected_set(elm_item_, selected ? EINA_TRUE : EINA_FALSE);
}

void GenlistCallbackData::ScrollTo(Elm_Genlist_Item_Scrollto_Type type) {
  elm_genlist_item_bring_in(elm_item_, type);
}

int GenlistCallbackData::GetIndex() const {
  return index_;
}

const base::string16& GenlistCallbackData::GetLabel() const {
  return menu_item_.label;
}

#if defined(TIZEN_ATK_SUPPORT)
Elm_Object_Item* GenlistCallbackData::GetElmItem() const {
  return elm_item_;
}
#endif

SelectPickerBase* GenlistCallbackData::GetSelectPicker() const {
  return select_picker_;
}

bool GenlistCallbackData::IsEnabled() const {
  return menu_item_.enabled;
}
