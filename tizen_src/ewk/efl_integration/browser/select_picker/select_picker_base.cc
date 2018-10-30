// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/select_picker/select_picker_base.h"

#include <Elementary.h>

#include "base/strings/utf_string_conversions.h"
#include "browser/select_picker/select_picker.h"
#include "browser/select_picker/select_picker_tv.h"
#include "browser/select_picker/select_picker_util.h"
#include "browser/select_picker/select_picker_wearable.h"
#include "eweb_view.h"
#include "tizen/system_info.h"

namespace {

const char* const kSelectedCbName = "selected";
const char* const kUnselectedCbName = "unselected";
const char* const kElmText = "elm.text";

template <class T>
void InsertSorted(std::vector<T>& vec, const T value) {
  vec.insert(std::upper_bound(vec.begin(), vec.end(), value), value);
}

template <class T>
size_t SearchValue(const std::vector<T>& vec, const T value) {
  auto it = find(vec.begin(), vec.end(), value);
  if (it == vec.end())
    return std::string::npos;
  return std::distance(vec.begin(), it);
}

template <class T>
void RemoveValue(std::vector<T>& vec, const T value) {
  vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
}

char* LabelGetCallback(void* data, Evas_Object* obj, const char* part) {
  auto callback_data = static_cast<GenlistCallbackData*>(data);
  if (!strncmp(part, kElmText, strlen(kElmText)))
    return elm_entry_utf8_to_markup(
        base::UTF16ToUTF8(callback_data->GetLabel()).c_str());
  return nullptr;
}
}

SelectPickerBase::SelectPickerBase(EWebView* web_view,
                                   int selected_index,
                                   bool is_multiple_selection,
                                   Elm_Genlist_Item_Scrollto_Type scrollto_type)
    : web_view_(web_view),
      popup_list_(nullptr),
      selected_index_(selected_index),
      is_multiple_selection_(is_multiple_selection),
      item_class_(nullptr),
      scrollto_type_(scrollto_type),
      group_class_(nullptr)
#if defined(TIZEN_ATK_SUPPORT)
      ,
      first_item_(nullptr),
      selected_item_(nullptr)
#endif
{
  window_ = elm_object_top_widget_get(
      elm_object_parent_widget_get(web_view->evas_object()));
  layout_ = elm_layout_add(window_);
}

SelectPickerBase::~SelectPickerBase() {
  auto rfh = static_cast<content::RenderFrameHostImpl*>(
      web_view_->web_contents().GetFocusedFrame());
  if (rfh)
    rfh->DidCancelPopupMenu();
  DestroyPopupList();
  elm_genlist_item_class_free(group_class_);
  elm_genlist_item_class_free(item_class_);
  evas_object_del(layout_);
}

SelectPickerBase* SelectPickerBase::Create(
    EWebView* web_view,
    int selected_index,
    const std::vector<content::MenuItem>& items,
    bool is_multiple_selection,
    const gfx::Rect& bounds) {
  SelectPickerBase* picker;
  if (IsTvProfile()) {
      picker = new SelectPickerEflTv(web_view,
                                   selected_index,
                                   is_multiple_selection);
  } else if (IsWearableProfile()) {
      picker = new SelectPickerEflWearable(web_view,
                                   selected_index,
                                   is_multiple_selection);
  } else {
      picker = new SelectPickerEfl(web_view,
                                   selected_index,
                                   is_multiple_selection);
  }
  // Create two separate Elm_Genlist_Item_Class classes, because EFL cannot swap
  // item_style at runtime.
  picker->InitializeItemClass();
  picker->InitializeGroupClass();
  picker->Init(items, bounds);
  return picker;
}

void SelectPickerBase::InitializeItemClass() {
  item_class_ = elm_genlist_item_class_new();
  item_class_->item_style = GetItemStyle();
  item_class_->func.text_get = LabelGetCallback;
  item_class_->func.content_get = nullptr;
  item_class_->func.state_get = nullptr;
  item_class_->func.del = nullptr;
}

void SelectPickerBase::InitializeGroupClass() {
  group_class_ = elm_genlist_item_class_new();
  group_class_->item_style = "group_index";
  group_class_->func.text_get = LabelGetCallback;
  group_class_->func.content_get = nullptr;
  group_class_->func.state_get = nullptr;
  group_class_->func.del = nullptr;
}

const char* SelectPickerBase::GetItemStyle() const {
  return "default";
}

void SelectPickerBase::UpdatePickerData(
    int selected_index,
    const std::vector<content::MenuItem>& items,
    bool is_multiple_selection) {
  ClearData();
  selected_index_ = selected_index;
  is_multiple_selection_ = is_multiple_selection;
  CreateAndPopulatePopupList(items);
}

gfx::Rect SelectPickerBase::GetGeometryDIP() const{
  return gfx::Rect();
}

void SelectPickerBase::Resize(void* data, Evas*, Evas_Object*, void*) {
  auto thiz = static_cast<SelectPickerBase*>(data);
  thiz->ScrollSelectedItemTo();
}

void SelectPickerBase::ScrollSelectedItemTo() {
  if (is_multiple_selection_ && !selected_indexes_.empty())
    select_picker_data_[selected_indexes_.front()]->ScrollTo(scrollto_type_);
  else if (selected_index_ >= 0)
    select_picker_data_[selected_index_]->ScrollTo(scrollto_type_);
}

void SelectPickerBase::DestroyPopupList() {
  if (popup_list_) {
    if (is_multiple_selection_) {
      evas_object_smart_callback_del(popup_list_, kSelectedCbName,
                                     MenuItemActivatedCallback);
      evas_object_smart_callback_del(popup_list_, kUnselectedCbName,
                                     MenuItemDeactivatedCallback);
    }
    evas_object_event_callback_del(popup_list_, EVAS_CALLBACK_RESIZE, Resize);
    elm_genlist_clear(popup_list_);
    evas_object_del(popup_list_);
  }
  for (const auto& item : select_picker_data_)
    delete item;
}

void SelectPickerBase::ClearData() {
  DestroyPopupList();
  select_picker_data_.clear();
  selected_indexes_.clear();
}

void SelectPickerBase::ItemSelectedCallback(void* data,
                                            Evas_Object* obj,
                                            void* event_info) {
  auto callback_data = static_cast<GenlistCallbackData*>(data);
  callback_data->GetSelectPicker()->ItemSelected(callback_data, event_info);
}

void SelectPickerBase::ItemSelected(GenlistCallbackData* data,
                                    void* event_info) {
  if (data->IsEnabled()) {
    selected_index_ = data->GetIndex();
    DidSelectPopupMenuItem();
  }
}

void SelectPickerBase::DidSelectPopupMenuItem() {
  auto render_frame_host = static_cast<content::RenderFrameHostImpl*>(
      web_view_->web_contents().GetFocusedFrame());
  if (!render_frame_host || select_picker_data_.empty())
    return;
  // When user select empty space then no index is selected, so selectedIndex
  // value is -1. In that case we should call valueChanged() with -1 index.
  // That in turn call popupDidHide() in didChangeSelectedIndex() for reseting
  // the value of m_popupIsVisible in RenderMenuList.
  if (selected_index_ != -1 &&
      selected_index_ >= static_cast<int>(select_picker_data_.size()))
    return;
  // In order to reuse RenderFrameHostImpl::DidSelectPopupMenuItems() method
  // in Android, put selectedIndex into std::vector<int>.
  std::vector<int> selectedIndices;
  selectedIndices.push_back(selected_index_);
  render_frame_host->DidSelectPopupMenuItems(selectedIndices);
}

void SelectPickerBase::MenuItemActivatedCallback(void* data,
                                                 Evas_Object* obj,
                                                 void* event_info) {
  auto picker = static_cast<SelectPickerBase*>(data);
  auto selected = static_cast<Elm_Object_Item*>(event_info);
  // Subtract 1 from value returned by elm_genlist_item_index_get
  // to match webkit expectation (index starting from 0).
  int index = elm_genlist_item_index_get(selected) - 1;
  if (picker->is_multiple_selection_) {
    size_t pos = SearchValue(picker->selected_indexes_, index);
    if (pos == std::string::npos)
      InsertSorted(picker->selected_indexes_, index);
    else
      RemoveValue(picker->selected_indexes_, index);
  }
  picker->DidMultipleSelectPopupMenuItem();
}

void SelectPickerBase::MenuItemDeactivatedCallback(void* data,
                                                   Evas_Object* obj,
                                                   void* event_info) {
  auto picker = static_cast<SelectPickerBase*>(data);
  auto deselectedItem = static_cast<Elm_Object_Item*>(event_info);
  // Subtract 1 from value returned by elm_genlist_item_index_get
  // to match webkit expectation (index starting from 0).
  int deselectedIndex = elm_genlist_item_index_get(deselectedItem) - 1;
  size_t pos = SearchValue(picker->selected_indexes_, deselectedIndex);
  if (pos == std::string::npos)
    InsertSorted(picker->selected_indexes_, deselectedIndex);
  else
    RemoveValue(picker->selected_indexes_, deselectedIndex);
  picker->DidMultipleSelectPopupMenuItem();
}

void SelectPickerBase::DidMultipleSelectPopupMenuItem() {
  auto render_frame_host = static_cast<content::RenderFrameHostImpl*>(
      web_view_->web_contents().GetFocusedFrame());
  if (!render_frame_host || select_picker_data_.empty())
    return;
  render_frame_host->DidSelectPopupMenuItems(selected_indexes_);
}

void SelectPickerBase::InitializeSelectedPickerData(
    const std::vector<content::MenuItem>& items) {
  bool need_scroll_to_top = true;
  for (size_t index = 0; index < items.size(); index++) {
#if defined(TIZEN_ATK_SUPPORT)
    auto data = new GenlistCallbackData(
        index, this, items[index], popup_list_, item_class_, group_class_,
        is_multiple_selection_, ItemSelectedCallback,
        web_view_->GetAtkStatus());

    // Save first item for atk
    if (index == 0) {
      first_item_ = data->GetElmItem();
      selected_item_ = data->GetElmItem();
    }
#else
    auto data = new GenlistCallbackData(
        index, this, items[index], popup_list_, item_class_, group_class_,
        is_multiple_selection_, ItemSelectedCallback);
#endif
    if (is_multiple_selection_ && items[index].checked) {
      selected_indexes_.push_back(index);
      if (need_scroll_to_top) {
        data->ScrollTo(scrollto_type_);
        need_scroll_to_top = false;
#if defined(TIZEN_ATK_SUPPORT)
        // Save selected item for atk with multiple selection
        selected_item_ = data->GetElmItem();
#endif
      }
    }
    select_picker_data_.push_back(data);

#if defined(TIZEN_ATK_SUPPORT)
    // Save selected item for atk
    if (base::checked_cast<int>(index) == selected_index_ &&
        !is_multiple_selection_)
      selected_item_ = data->GetElmItem();
#endif
  }

  if (is_multiple_selection_) {
    evas_object_smart_callback_add(popup_list_, kSelectedCbName,
                                   MenuItemActivatedCallback, this);
    evas_object_smart_callback_add(popup_list_, kUnselectedCbName,
                                   MenuItemDeactivatedCallback, this);
  } else if (selected_index_ >= 0) {
    select_picker_data_[selected_index_]->ScrollTo(scrollto_type_);
  }
  evas_object_event_callback_add(popup_list_, EVAS_CALLBACK_RESIZE, Resize,
                                 this);
}

void SelectPickerBase::Show() {
  evas_object_show(layout_);
#if defined(TIZEN_ATK_SUPPORT)
  if (web_view_->GetAtkStatus())
    elm_atspi_component_highlight_grab(selected_item_);
#endif
}

void SelectPickerBase::Hide() {
  evas_object_hide(layout_);
}

bool SelectPickerBase::IsVisible() const {
  return evas_object_visible_get(layout_);
}
