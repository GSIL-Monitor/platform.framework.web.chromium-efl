// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_BASE_H_
#define EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_BASE_H_

#include <Elementary.h>
#include <vector>

#include "content/public/common/menu_item.h"
#include "ui/gfx/geometry/rect.h"

class EWebView;
class GenlistCallbackData;

class SelectPickerBase {
 public:
  virtual ~SelectPickerBase();
  static SelectPickerBase* Create(EWebView* web_view,
                                  int selected_index,
                                  const std::vector<content::MenuItem>& items,
                                  bool is_multiple_selection,
                                  const gfx::Rect& bounds);
  virtual void UpdateFormNavigation(int form_element_count,
                                    int current_node_index) {}
  virtual void UpdatePickerData(int selected_index,
                                const std::vector<content::MenuItem>& items,
                                bool is_multiple_selection);
  virtual gfx::Rect GetGeometryDIP() const;

  void Show();
  void Hide();
  bool IsVisible() const;

 protected:
  explicit SelectPickerBase(EWebView* web_view,
                            int selected_index,
                            bool is_multiple_selection,
                            Elm_Genlist_Item_Scrollto_Type scrollto_type);
  virtual const char* GetItemStyle() const;
  virtual void Init(const std::vector<content::MenuItem>& items,
                    const gfx::Rect& bounds) = 0;
  virtual void ClearData();
  virtual void CreateAndPopulatePopupList(
      const std::vector<content::MenuItem>& items) = 0;
  virtual void ItemSelected(GenlistCallbackData* data, void* event_info);
  void InitializeSelectedPickerData(
      const std::vector<content::MenuItem>& items);
  void DidSelectPopupMenuItem();
  void DidMultipleSelectPopupMenuItem();
  void ScrollSelectedItemTo();

  EWebView* web_view_;
  Evas_Object* popup_list_;
  Evas_Object* layout_;
  Evas_Object* window_;
  int selected_index_;
  bool is_multiple_selection_;
  Elm_Genlist_Item_Class* item_class_;
  Elm_Genlist_Item_Scrollto_Type scrollto_type_;
#if defined(TIZEN_ATK_SUPPORT)
  Elm_Object_Item* first_item_;
  Elm_Object_Item* selected_item_;
#endif

 private:
  static void ItemSelectedCallback(void*, Evas_Object*, void*);
  static void MenuItemActivatedCallback(void*, Evas_Object*, void*);
  static void MenuItemDeactivatedCallback(void*, Evas_Object*, void*);
  static void Resize(void* data, Evas* e, Evas_Object* obj, void* event_info);
  void InitializeItemClass();
  void InitializeGroupClass();
  void DestroyPopupList();

  Elm_Genlist_Item_Class* group_class_;
  std::vector<GenlistCallbackData*> select_picker_data_;
  std::vector<int> selected_indexes_;
};

#endif  // EWK_EFL_INTEGRATION_BROWSER_SELECT_PICKER_SELECT_PICKER_BASE_H_
