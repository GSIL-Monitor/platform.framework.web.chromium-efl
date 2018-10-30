// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_UTIL_H_
#define EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_UTIL_H_

#include <Elementary.h>

#include "base/strings/string16.h"
#include "content/public/common/menu_item.h"

class SelectPickerBase;

class GenlistCallbackData {
 public:
  GenlistCallbackData(int index,
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
                      );
  ~GenlistCallbackData();

  void SetSelection(bool selected);
  void ScrollTo(Elm_Genlist_Item_Scrollto_Type type);
  int GetIndex() const;
  const base::string16& GetLabel() const;
#if defined(TIZEN_ATK_SUPPORT)
  Elm_Object_Item* GetElmItem() const;
#endif
  SelectPickerBase* GetSelectPicker() const;
  bool IsEnabled() const;

 private:
  int index_;
  SelectPickerBase* select_picker_;
  Elm_Object_Item* elm_item_;
  content::MenuItem menu_item_;
#if defined(TIZEN_ATK_SUPPORT)
  bool atk_enabled_;
#endif
};

#endif  // EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_UTIL_H_
