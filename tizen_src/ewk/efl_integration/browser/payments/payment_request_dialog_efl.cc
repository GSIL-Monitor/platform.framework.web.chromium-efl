// Copyright 2017 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/payments/payment_request_dialog_efl.h"

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "browser/payments/cvc_unmask_view_controller_efl.h"
#include "browser/payments/payment_data_efl.h"
#include "common/web_contents_utils.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_type.h"
#include "components/autofill/core/browser/country_data.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/phone_number.h"
#include "components/autofill/core/browser/phone_number_i18n.h"
#include "components/autofill/core/browser/validation.h"
#include "components/payments/content/payment_request.h"
#include "components/payments/core/currency_formatter.h"
#include "components/strings/grit/components_strings.h"
#include "content/common/paths_efl.h"
#include "eweb_view.h"
#include "third_party/WebKit/public/platform/modules/payments/payment_request.mojom.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_ui.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_collator.h"

#include <Elementary.h>
#include <Evas.h>

#include <string.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#if defined(OS_TIZEN)
#include <efl_extension.h>
#endif

static const float DB_SYNC_INTERVAL = 0.1;

using web_contents_utils::WebViewFromWebContents;

namespace payments {

struct LayoutIdData {
  PaymentRequestDialogEfl* dialog;
  LayoutId id;
};

static std::string ConcatCurrenyValue(const std::string& currency,
                                      const std::string& value) {
  std::string currency_value;
  if (!strcmp(currency.c_str(), "USD"))
    currency_value += "$";
  currency_value += value;

  return currency_value;
}

static std::string GetShippingAddressRawInfo(
    const autofill::AutofillProfile* profile) {
  std::string country =
      base::UTF16ToUTF8(profile->GetRawInfo(autofill::ADDRESS_HOME_COUNTRY));
  std::string name =
      base::UTF16ToUTF8(profile->GetRawInfo(autofill::NAME_FIRST));
  std::string organisation =
      base::UTF16ToUTF8(profile->GetRawInfo(autofill::COMPANY_NAME));
  std::string street_address =
      base::UTF16ToUTF8(profile->GetRawInfo(autofill::ADDRESS_HOME_LINE1));
  std::string county =
      base::UTF16ToUTF8(profile->GetRawInfo(autofill::ADDRESS_HOME_STATE));
  std::string postcode =
      base::UTF16ToUTF8(profile->GetRawInfo(autofill::ADDRESS_HOME_ZIP));
  std::string phone =
      base::UTF16ToUTF8(profile->GetRawInfo(autofill::PHONE_HOME_WHOLE_NUMBER));
  return country + " " + name + " " + organisation + " " + "\n" +
         street_address + " " + county + " " + postcode + " " + phone;
}

void PaymentRequestDialogEfl::AppendItemToContextPopup(
    PaymentRequestDialogEfl* dialog,
    Evas_Object* btn,
    Evas_Object* ctxpopup,
    std::string data,
    ContextPopupItemType type,
    std::string guid) {
  CtxpopupItemData* ctx_data = new CtxpopupItemData;
  ctx_data->dialog = dialog;
  ctx_data->btn = btn;
  ctx_data->ctxpopup = ctxpopup;
  ctx_data->data = data;
  ctx_data->type = type;
  ctx_data->guid = guid;
  ctxpopup_data_list_.push_back(ctx_data);
  elm_ctxpopup_item_append(ctxpopup, ctx_data->data.c_str(), NULL,
                           CtxpopupItemCb, ctx_data);
}

void PaymentRequestDialogEfl::ShowCvcUnmaskPrompt(
    const autofill::CreditCard& credit_card,
    base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
        result_delegate,
    content::WebContents* web_contents) {
  cvc_controller_.reset(new CvcUnmaskViewControllerEfl(
      request_->state(), this, credit_card, result_delegate, web_contents));
}

void PaymentRequestDialogEfl::ShippingAppendItemToContextPopup(
    PaymentRequestDialogEfl* dialog,
    Evas_Object* btn,
    Evas_Object* ctxpopup,
    std::string data) {
  CtxpopupItemData* ctx_data = new CtxpopupItemData;
  ctx_data->dialog = dialog;
  ctx_data->btn = btn;
  ctx_data->ctxpopup = ctxpopup;
  ctx_data->data = data;
  ctxpopup_data_list_.push_back(ctx_data);
  elm_ctxpopup_item_append(ctxpopup, data.substr(2).c_str(), NULL,
                           ShippingCtxpopupItemCb, ctx_data);
}

void PaymentRequestDialogEfl::ClearCtxpopupDataList() {
  for (const auto& item : ctxpopup_data_list_)
    delete item;
  ctxpopup_data_list_.clear();
}

void PaymentRequestDialogEfl::ClearAllDataLists() {
  if (!credit_card_number_data_list_.empty()) {
    for (const auto& item : credit_card_number_data_list_)
      delete item;
    credit_card_number_data_list_.clear();
  }

  if (!ctxpopup_data_list_.empty()) {
    for (const auto& item : ctxpopup_data_list_)
      delete item;
    ctxpopup_data_list_.clear();
  }

  if (!phone_number_data_list_.empty()) {
    for (const auto& item : phone_number_data_list_)
      delete item;
    phone_number_data_list_.clear();
  }

  if (!shipping_address_data_list_.empty()) {
    for (const auto& item : shipping_address_data_list_)
      delete item;
    shipping_address_data_list_.clear();
  }

  if (!email_address_data_list_.empty()) {
    for (const auto& item : email_address_data_list_)
      delete item;
    email_address_data_list_.clear();
  }

  if (!shipping_option_data_list_.empty()) {
    for (const auto& item : shipping_option_data_list_)
      delete item;
    shipping_option_data_list_.clear();
  }

  if (!order_summary_data_list_.empty()) {
    for (const auto& item : order_summary_data_list_)
      delete item;
    order_summary_data_list_.clear();
  }

  if (!layout_id_data_list_.empty()) {
    for (const auto& item : layout_id_data_list_)
      delete item;
    layout_id_data_list_.clear();
  }

  if (!item_data_list_.empty()) {
    for (const auto& item : item_data_list_)
      delete item;
    item_data_list_.clear();
  }

  contact_info_profiles_list_.clear();
  shipping_data_profile_list_.clear();
  web_contact_info_profiles_list_.clear();
  web_shipping_data_profile_list_.clear();
  credit_card_list_.clear();
}

PaymentRequestDialogEfl::PaymentRequestDialogEfl(
    PaymentRequest* request,
    content::WebContents* web_contents)
    : request_(request),
      web_view_(WebViewFromWebContents(web_contents)),
      popup_(nullptr),
      conformant_(nullptr),
      container_layout_(nullptr),
      main_layout_(nullptr),
      header_layout_(nullptr),
      card_input_layout_(nullptr),
      cvc_layout_(nullptr),
      cvc_controller_(nullptr),
      progress_layout_(nullptr),
      order_summary_layout_(nullptr),
      shipping_address_layout_(nullptr),
      shipping_address_input_layout_(nullptr),
      shipping_option_layout_(nullptr),
      payment_method_layout_(nullptr),
      contact_phone_number_layout_(nullptr),
      contact_email_address_layout_(nullptr),
      contact_phone_number_input_layout_(nullptr),
      contact_email_address_input_layout_(nullptr),
      shipping_input_genlist_(nullptr),
      credit_card_genlist_(nullptr),
      order_summary_genlist_(nullptr),
      contact_genlist_(nullptr),
      shipping_genlist_(nullptr),
      card_name_entry_(nullptr),
      card_number_entry_(nullptr),
      cvc_entry_(nullptr),
      contact_phone_number_entry_(nullptr),
      contact_email_address_entry_(nullptr),
      shipping_name_entry_(nullptr),
      shipping_street_entry_(nullptr),
      shipping_postaltown_entry_(nullptr),
      shipping_postcode_entry_(nullptr),
      shipping_phone_entry_(nullptr),
      order_summary_radio_group_(nullptr),
      credit_card_radio_group_(nullptr),
      contact_radio_group_(nullptr),
      shipping_radio_group_(nullptr),
      shipping_options_radio_group_(nullptr),
      details_button_(nullptr),
      pay_button_(nullptr),
      db_sync_timer_(nullptr),
      card_number_length_(0),
      card_type_(CARD_UNKNOWN),
      curr_expanded_layout_id_(LAYOUT_NONE),
      main_layout_expanded_(false),
      personal_data_manager_(nullptr) {
#if defined(TIZEN_AUTOFILL_SUPPORT)
  personal_data_manager_ = request_->state()->GetPersonalDataManager();
  if (!personal_data_manager_) {
    LOG(INFO) << "PersonalDataManager is NULL";
    return;
  }
#endif
  InitializeOrResetData();

  base::FilePath edj_dir;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  payment_edj_path_ = edj_dir.Append(FILE_PATH_LITERAL("PaymentMain.edj"));
  elm_theme_extension_add(NULL, payment_edj_path_.AsUTF8Unsafe().c_str());
  SetCountries();
}

PaymentRequestDialogEfl::~PaymentRequestDialogEfl() {
  ClearAllDataLists();
  Close();
}

void PaymentRequestDialogEfl::InitializeOrResetData() {
  card_details_added_.ResetValues();
  payment_data_.reset(new PaymentDataEfl(this));
  contact_info_details_.reset(new ContactInfoDetails);
  shipping_address_data_.reset(new ShippingAddressData);
  contact_info_profile_.reset(new autofill::AutofillProfile());
  shipping_info_profile_.reset(new autofill::AutofillProfile());

#if defined(TIZEN_AUTOFILL_SUPPORT)
  // Credit cards
  credit_card_list_ = GetPersonalDataManager()->GetCreditCards();

  // Shipping autofill profiles
  shipping_data_profile_list_ = GetPersonalDataManager()->GetProfiles();
  web_shipping_data_profile_list_.clear();
  for (const auto& profile : shipping_data_profile_list_) {
    std::string country = base::UTF16ToUTF8(
        profile->GetRawInfo((autofill::ADDRESS_HOME_COUNTRY)));
    std::string name =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::NAME_FIRST)));
    std::string organisation =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::COMPANY_NAME)));
    std::string street_address =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::ADDRESS_HOME_LINE1)));
    std::string county =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::ADDRESS_HOME_STATE)));
    std::string postcode =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::ADDRESS_HOME_ZIP)));
    std::string phone = base::UTF16ToUTF8(
        profile->GetRawInfo((autofill::PHONE_HOME_WHOLE_NUMBER)));
    if (!country.empty() && !name.empty() && !street_address.empty() &&
        !postcode.empty() && !phone.empty()) {
      web_shipping_data_profile_list_.push_back(profile);
    }
  }

  // Contacts autofill profiles
  web_contact_info_profiles_list_.clear();
  contact_info_profiles_list_ = GetPersonalDataManager()->GetProfiles();
  if (request_->spec()->request_payer_phone()) {
    for (const auto& profile : contact_info_profiles_list_) {
      std::string number = base::UTF16ToUTF8(
          profile->GetRawInfo((autofill::PHONE_HOME_WHOLE_NUMBER)));
      if (!number.empty())
        web_contact_info_profiles_list_.push_back(profile);
    }
  }
  if (request_->spec()->request_payer_email()) {
    for (const auto& profile : contact_info_profiles_list_) {
      std::string email_address =
          base::UTF16ToUTF8(profile->GetRawInfo((autofill::EMAIL_ADDRESS)));
      if (!email_address.empty())
        web_contact_info_profiles_list_.push_back(profile);
    }
  }
#endif
}

void PaymentRequestDialogEfl::SetCountries() {
  countries_.clear();

  std::string app_locale = request_->state()->GetApplicationLocale();
  // The sorted list of country codes.
  const std::vector<std::string>* available_countries =
      &autofill::CountryDataMap::GetInstance()->country_codes();

  // Filter out the countries that do not have rules for address input and
  // validation.
  const std::vector<std::string>& addressinput_countries =
      ::i18n::addressinput::GetRegionCodes();
  std::vector<std::string> filtered_countries;
  filtered_countries.reserve(available_countries->size());
  std::set_intersection(
      available_countries->begin(), available_countries->end(),
      addressinput_countries.begin(), addressinput_countries.end(),
      std::back_inserter(filtered_countries));
  available_countries = &filtered_countries;

  CountryVector sorted_countries;
  for (const auto& country_code : *available_countries) {
    sorted_countries.push_back(
        base::MakeUnique<autofill::AutofillCountry>(country_code, app_locale));
  }

  l10n_util::SortStringsUsingMethod(app_locale, &sorted_countries,
                                    &autofill::AutofillCountry::name);
  std::move(sorted_countries.begin(), sorted_countries.end(),
            std::back_inserter(countries_));
}

void PaymentRequestDialogEfl::ShowDialog() {
  Evas_Object* top_window = web_view_->GetElmWindow();
  if (!top_window)
    return;

  conformant_ = elm_conformant_add(top_window);
  if (!conformant_)
    return;

  evas_object_size_hint_weight_set(conformant_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  elm_win_resize_object_add(top_window, conformant_);
  evas_object_show(conformant_);

  Evas_Object* layout = elm_layout_add(conformant_);
  elm_layout_theme_set(layout, "layout", "application", "default");
  evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_show(layout);
  elm_object_content_set(conformant_, layout);

  popup_ = elm_popup_add(layout);
  if (!popup_) {
    LOG(ERROR) << "Popup creation failed!";
    Close();
    return;
  }
  elm_popup_align_set(popup_, ELM_NOTIFY_ALIGN_FILL, 1.0);

  Evas_Object* bg_layout = elm_layout_add(popup_);
  elm_layout_file_set(bg_layout, payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "background");
  evas_object_size_hint_weight_set(bg_layout, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(bg_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_show(bg_layout);
  elm_object_content_set(popup_, bg_layout);

  // Add Container layout, that holds main layout and
  // bottom bar i.e Cancel & Pay buttons.
  container_layout_ = elm_layout_add(bg_layout);
  elm_layout_file_set(container_layout_,
                      payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "payment_layout_container");
  evas_object_size_hint_weight_set(container_layout_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(container_layout_, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);
  evas_object_show(container_layout_);
  elm_object_content_set(bg_layout, container_layout_);

  // Main popup layout
  main_layout_ = elm_layout_add(container_layout_);
  elm_layout_file_set(main_layout_, payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "payment_layout");
  evas_object_size_hint_weight_set(main_layout_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(main_layout_, EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_show(main_layout_);
  elm_object_part_content_set(container_layout_, "swallow_payment_layout",
                              main_layout_);

  evas_object_focus_set(popup_, true);
  evas_object_show(popup_);
#if defined(OS_TIZEN)
  eext_object_event_callback_add(popup_, EEXT_CALLBACK_BACK, CancelButtonCb,
                                 this);
#endif

  header_layout_ = AddHeaderLayout();

  // Add Pay button.
  pay_button_ = AddButton(container_layout_, "PAY", "pay_button", PayButtonCb);
  elm_object_disabled_set(pay_button_, true);

  // Add cancel button.
  Evas_Object* cancel_btn =
      AddButton(container_layout_, "CANCEL", "cancel_button", CancelButtonCb);

  // ShowDialog might be called multiple times for the existing
  // PaymentRequestDialog handle. So, reset the local data lists and
  // reinitialize autofill lists.
  InitializeOrResetData();

  // Add layout parts to main layout.
  order_summary_layout_ = AddOrderSummaryLayout();

  if (request_->spec()->request_shipping())
    shipping_address_layout_ = AddShippingAddressLayout();
  else
    elm_object_signal_emit(main_layout_, "elm,action,hide,shippingAddress", "");

  if (request_->spec()->request_shipping())
    shipping_option_layout_ = AddShippingOptionLayout();
  else
    elm_object_signal_emit(main_layout_, "elm,action,hide,shippingOption", "");

  payment_method_layout_ = AddPaymentMethodLayout();

  if (request_->spec()->request_payer_phone())
    contact_phone_number_layout_ = AddPhoneNumberLayout();
  else
    elm_object_signal_emit(main_layout_, "elm,action,hide,phoneNumber", "");

  if (request_->spec()->request_payer_email())
    contact_email_address_layout_ = AddEmailAddressLayout();
  else
    elm_object_signal_emit(main_layout_, "elm,action,hide,emailAddress", "");

  details_button_ = AddCustomButton(main_layout_, "View details",
                                    "details_button", DetailsButtonCb);
  elm_object_signal_emit(details_button_, "elm,action,set,smallfont", "");

  int width = 0, height = 0;
#if defined(USE_WAYLAND)
  Ecore_Wl2_Display* wl2_display = ecore_wl2_connected_display_get(NULL);
  ecore_wl2_display_screen_size_get(wl2_display, &width, &height);
#else
  ecore_x_window_size_get(ecore_x_window_root_first_get(), &width, &height);
#endif
  evas_object_resize(popup_, width, height);
  evas_object_move(popup_, 0, 0);
}

Evas_Object* PaymentRequestDialogEfl::AddHeaderLayout() {
  Evas_Object* layout = elm_layout_add(main_layout_);
  elm_layout_file_set(layout, payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "header_layout");

  evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_show(layout);

  // Get favicon, page title.
  Evas_Object* favicon = ewk_view_favicon_get(web_view_->evas_object());
  const char* title = web_view_->GetTitle();

  // Get hostname.
  std::string hostname = web_view_->GetURL().host();
  const char* url = hostname.c_str();

  // Set favicon, title and url.
  elm_object_part_content_set(layout, "favicon", favicon);
  elm_object_part_text_set(layout, "title_text", title);
  elm_object_part_text_set(layout, "url_text", url);
  elm_object_part_content_set(main_layout_, "header_swallow", layout);
  return layout;
}

void PaymentRequestDialogEfl::AddCardInputLayout() {
  base::FilePath edj_dir;
  base::FilePath add_card_edj_path;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  add_card_edj_path = edj_dir.Append(FILE_PATH_LITERAL("PaymentAddCard.edj"));
  card_details_added_.ResetValues();

  card_input_layout_ = elm_layout_add(main_layout_);

  elm_layout_file_set(card_input_layout_,
                      add_card_edj_path.AsUTF8Unsafe().c_str(),
                      "Addcard_layout");
  evas_object_size_hint_weight_set(card_input_layout_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(card_input_layout_, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);
  evas_object_show(card_input_layout_);

  elm_object_content_set(main_layout_, card_input_layout_);

  // Add cancel button.
  Evas_Object* cancel_btn = AddButton(
      card_input_layout_, "CANCEL", "cancel_button", CancelCreditCardButtonCb);

  // Add Done button.
  Evas_Object* done_btn = AddButton(card_input_layout_, "DONE", "done_button",
                                    CreditCardDoneButtonCb);

  // Add drop down button for month.
  Evas_Object* drop_down_btn_month = AddDropDownButton("MM", "month");

  // Add drop down button for year.
  Evas_Object* drop_down_btn_year = AddDropDownButton("YY", "year");

  // Add drop down button for billing address.
  Evas_Object* drop_down_btn_bill =
      AddDropDownButton("ADDR", "billaddressdropdown");

  // Create new credit card object to store User Added info
  credit_card_ =
      autofill::CreditCard(base::GenerateGUID(), autofill::kSettingsOrigin);

  //---------CARD NAME---------
  card_name_entry_ = elm_entry_add(card_input_layout_);
  elm_entry_single_line_set(card_name_entry_, EINA_TRUE);
  elm_entry_scrollable_set(card_name_entry_, EINA_FALSE);
  elm_entry_scrollable_set(card_name_entry_, EINA_FALSE);
  evas_object_size_hint_weight_set(card_name_entry_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(card_name_entry_, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);
  evas_object_smart_callback_add(card_name_entry_, "changed",
                                 ValidateCardNameCb, this);
  evas_object_smart_callback_add(card_name_entry_, "activated", AddCardNameCb,
                                 this);
  evas_object_show(card_name_entry_);
  elm_object_part_content_set(card_input_layout_, "card_name_input_entry",
                              card_name_entry_);

  //---------CARD NUMBER--------
  card_number_entry_ = elm_entry_add(card_input_layout_);
  elm_entry_single_line_set(card_number_entry_, EINA_TRUE);
  elm_entry_scrollable_set(card_number_entry_, EINA_FALSE);
  elm_entry_input_panel_layout_set(card_number_entry_,
                                   ELM_INPUT_PANEL_LAYOUT_NUMBERONLY);
  evas_object_size_hint_weight_set(card_number_entry_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(card_number_entry_, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);
  evas_object_smart_callback_add(card_number_entry_, "changed",
                                 ValidateCardNumberCb, this);
  evas_object_smart_callback_add(card_number_entry_, "activated",
                                 AddCardNumberCb, this);
  evas_object_show(card_number_entry_);
  elm_object_part_content_set(card_input_layout_, "card_number_input",
                              card_number_entry_);
}

char* PaymentRequestDialogEfl::ShippingAddressInputTextGetCb(void* data,
                                                             Evas_Object* obj,
                                                             const char* part) {
  ItemData* item_data = static_cast<ItemData*>(data);
  if (!strcmp(part, "item_label")) {
    switch (item_data->type) {
      case ShippingAddressFieldType::COUNTRY:
        return strdup("Country/region");
      case ShippingAddressFieldType::NAME:
        return strdup("Name*");
      case ShippingAddressFieldType::ORGANISATION:
        return strdup("Organisation");
      case ShippingAddressFieldType::STREETADDRESS:
        return strdup("Street address*");
      case ShippingAddressFieldType::POSTALTOWN:
        return strdup("Postal town*");
      case ShippingAddressFieldType::COUNTY:
        return strdup("County");
      case ShippingAddressFieldType::POSTCODE:
        return strdup("Postcode*");
      case ShippingAddressFieldType::PHONENUMBER:
        return strdup("Phone number*");
      case ShippingAddressFieldType::NONE:
        return strdup("");
    }
  } else if (!strcmp(part, "required_text")) {
    switch (item_data->type) {
      case ShippingAddressFieldType::COUNTRY:
        return strdup("");
      case ShippingAddressFieldType::NAME:
        return strdup("*required field");
      case ShippingAddressFieldType::ORGANISATION:
        return strdup("");
      case ShippingAddressFieldType::STREETADDRESS:
        return strdup("*required field");
      case ShippingAddressFieldType::POSTALTOWN:
        return strdup("*required field");
      case ShippingAddressFieldType::COUNTY:
        return strdup("");
      case ShippingAddressFieldType::POSTCODE:
        return strdup("*required field");
      case ShippingAddressFieldType::PHONENUMBER:
        return strdup("*required field");
      case ShippingAddressFieldType::NONE:
        return strdup("");
    }
  }
  return strdup("");
}

Evas_Object* PaymentRequestDialogEfl::ShippingAddressInputContentGetCb(
    void* data,
    Evas_Object* obj,
    const char* part) {
  ItemData* item_data = static_cast<ItemData*>(data);
  if (!strcmp(part, "entry_part")) {
    switch (item_data->type) {
      case ShippingAddressFieldType::COUNTRY: {
        Evas_Object* button =
            elm_button_add(item_data->dialog->shipping_input_genlist_);
        elm_object_style_set(button, "dropdown");
        elm_object_domain_translatable_part_text_set(button, NULL, "WebKit",
                                                     "Select");
        evas_object_smart_callback_add(
            button, "clicked", ShowCtxPopupCountryRegionCb, item_data->dialog);
        item_data->dropdown = button;
        return button;
      }
      case ShippingAddressFieldType::NONE: {
        return NULL;
      }
      case ShippingAddressFieldType::NAME: {
        Evas_Object* entry =
            elm_entry_add(item_data->dialog->shipping_input_genlist_);
        elm_entry_single_line_set(entry, EINA_TRUE);
        evas_object_smart_callback_add(entry, "changed", ShippingNameCb,
                                       item_data);
        item_data->entry = entry;
        item_data->dialog->shipping_name_entry_ = entry;
        return entry;
      }
      case ShippingAddressFieldType::ORGANISATION: {
        Evas_Object* entry =
            elm_entry_add(item_data->dialog->shipping_input_genlist_);
        elm_entry_single_line_set(entry, EINA_TRUE);
        evas_object_smart_callback_add(
            entry, "changed", ShippingAddressFieldValueChangedCb, item_data);
        item_data->entry = entry;
        return entry;
      }
      case ShippingAddressFieldType::STREETADDRESS: {
        Evas_Object* entry =
            elm_entry_add(item_data->dialog->shipping_input_genlist_);
        elm_entry_single_line_set(entry, EINA_TRUE);
        evas_object_smart_callback_add(
            entry, "changed", ShippingAddressFieldValueChangedCb, item_data);
        item_data->entry = entry;
        item_data->dialog->shipping_street_entry_ = entry;
        return entry;
      }
      case ShippingAddressFieldType::POSTALTOWN: {
        Evas_Object* entry =
            elm_entry_add(item_data->dialog->shipping_input_genlist_);
        elm_entry_single_line_set(entry, EINA_TRUE);
        evas_object_smart_callback_add(
            entry, "changed", ShippingAddressFieldValueChangedCb, item_data);
        item_data->entry = entry;
        item_data->dialog->shipping_postaltown_entry_ = entry;
        return entry;
      }
      case ShippingAddressFieldType::COUNTY: {
        Evas_Object* entry =
            elm_entry_add(item_data->dialog->shipping_input_genlist_);
        elm_entry_single_line_set(entry, EINA_TRUE);
        evas_object_smart_callback_add(
            entry, "changed", ShippingAddressFieldValueChangedCb, item_data);
        item_data->entry = entry;
        return entry;
      }
      case ShippingAddressFieldType::POSTCODE: {
        Evas_Object* entry =
            elm_entry_add(item_data->dialog->shipping_input_genlist_);
        elm_entry_single_line_set(entry, EINA_TRUE);
        elm_entry_input_panel_layout_set(entry,
                                         ELM_INPUT_PANEL_LAYOUT_PHONENUMBER);
        evas_object_smart_callback_add(
            entry, "changed", ShippingAddressFieldValueChangedCb, item_data);
        item_data->entry = entry;
        item_data->dialog->shipping_postcode_entry_ = entry;
        return entry;
      }
      case ShippingAddressFieldType::PHONENUMBER: {
        Evas_Object* entry =
            elm_entry_add(item_data->dialog->shipping_input_genlist_);
        elm_entry_single_line_set(entry, EINA_TRUE);
        elm_entry_input_panel_layout_set(entry,
                                         ELM_INPUT_PANEL_LAYOUT_PHONENUMBER);
        evas_object_smart_callback_add(
            entry, "changed", ShippingAddressFieldValueChangedCb, item_data);
        item_data->entry = entry;
        item_data->dialog->shipping_phone_entry_ = entry;
        return entry;
      }
    }
  }
  return nullptr;
}

void PaymentRequestDialogEfl::ShippingAddressInputLayout() {
  base::FilePath edj_dir, shipping_address_edj_path;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  shipping_address_edj_path =
      edj_dir.Append(FILE_PATH_LITERAL("PaymentShippingAddress.edj"));

  shipping_address_data_.reset(new ShippingAddressData);
  evas_object_hide(popup_);
  edje_object_signal_emit(main_layout_, "hide,main,layout", "");

  shipping_address_input_layout_ = elm_layout_add(main_layout_);

  // Set the shipping_address_layout_ file.
  elm_layout_file_set(shipping_address_input_layout_,
                      shipping_address_edj_path.AsUTF8Unsafe().c_str(),
                      "ShippingAddress_layout");
  elm_theme_extension_add(NULL,
                          shipping_address_edj_path.AsUTF8Unsafe().c_str());
  evas_object_size_hint_weight_set(shipping_address_input_layout_,
                                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(shipping_address_input_layout_,
                                  EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_show(shipping_address_input_layout_);

  elm_object_content_set(main_layout_, shipping_address_input_layout_);

  // Add add button to cancel Shipping address.
  Evas_Object* cancel_shipping_address_btn = AddButton(
      shipping_address_input_layout_, "CANCEL",
      "cancel_shipping_address_button", CancelShippingAddressButtonCb);

  // Add add button to Finished Shipping address.
  Evas_Object* finished_shipping_address_btn = AddButton(
      shipping_address_input_layout_, "DONE",
      "finished_shipping_address_button", FinishedShippingAddressButtonCb);

  shipping_input_genlist_ = elm_genlist_add(shipping_address_input_layout_);
  evas_object_show(shipping_input_genlist_);
  elm_object_part_content_set(shipping_address_input_layout_, "genlist_part",
                              shipping_input_genlist_);

  Elm_Genlist_Item_Class* itc1 = elm_genlist_item_class_new();
  itc1->item_style = "shipping_fields";
  itc1->func.text_get = ShippingAddressInputTextGetCb;
  itc1->func.content_get = ShippingAddressInputContentGetCb;
  itc1->func.del = NULL;

  ItemData* data = nullptr;
  for (size_t i = 0; i < ShippingAddressFieldType::NONE; ++i) {
    data = new ItemData;
    data->dialog = this;
    data->type = static_cast<ShippingAddressFieldType>(i + 1);
    item_data_list_.push_back(data);
    elm_genlist_item_append(shipping_input_genlist_, itc1, (void*)data, NULL,
                            ELM_GENLIST_ITEM_NONE, ItemSelectedCb, (void*)data);
  }
}

void PaymentRequestDialogEfl::AddExpandAndShrinkButtons(
    Evas_Object* layout,
    LayoutIdData* layout_data) {
  // Expand button.
  Evas_Object* down_button = elm_button_add(layout);
  elm_object_style_set(down_button, "custom_button");
  evas_object_smart_callback_add(down_button, "clicked",
                                 LayoutExpandButtonClickedCb, layout_data);
  Evas_Object* icon_down = elm_icon_add(down_button);
  elm_image_file_set(icon_down, payment_edj_path_.AsUTF8Unsafe().c_str(),
                     "expand_button_icon.png");
  elm_object_part_content_set(down_button, "button_icon", icon_down);
  elm_object_part_content_set(layout, "down_button_click", down_button);

  // Shrink Button
  Evas_Object* up_button = elm_button_add(layout);
  elm_object_style_set(up_button, "custom_button");
  evas_object_smart_callback_add(up_button, "clicked",
                                 LayoutShrinkButtonClickedCb, this);
  Evas_Object* icon_up = elm_icon_add(up_button);
  elm_image_file_set(icon_up, payment_edj_path_.AsUTF8Unsafe().c_str(),
                     "shrink_button_icon.png");
  elm_object_part_content_set(up_button, "button_icon", icon_up);
  elm_object_part_content_set(layout, "up_button_click", up_button);
}

void PaymentRequestDialogEfl::ItemSelectedCb(void* data,
                                             Evas_Object* obj,
                                             void* event_info) {}

void PaymentRequestDialogEfl::AddOverlayButton(const std::string& part,
                                               LayoutIdData* data) {
  Evas_Object* btn = elm_button_add(main_layout_);
  elm_object_style_set(btn, "custom_button");
  evas_object_smart_callback_add(btn, "clicked", OverlayButtonClickedCb, data);
  elm_object_part_text_set(btn, "button_text", "");
  elm_object_part_content_set(main_layout_, part.c_str(), btn);
}

void PaymentRequestDialogEfl::OverlayButtonClickedCb(void* data,
                                                     Evas_Object* obj,
                                                     void* event_info) {
  LayoutIdData* data_set = static_cast<LayoutIdData*>(data);
  // Expand layout only if it has some data items to show.
  switch (data_set->id) {
    case LAYOUT_SHIPPING_ADDR:
      if (data_set->dialog->web_shipping_data_profile_list_.empty())
        return;
      break;
    case LAYOUT_PAYMENT_METHOD:
      if (data_set->dialog->credit_card_list_.empty())
        return;
      break;
    case LAYOUT_PHONE_NUMBER:
      if (data_set->dialog->web_contact_info_profiles_list_.empty())
        return;
      break;
    case LAYOUT_EMAIL_ADDR:
      if (data_set->dialog->web_contact_info_profiles_list_.empty())
        return;
      break;
  }

  if (data_set->id == data_set->dialog->curr_expanded_layout_id_)
    LayoutShrinkButtonClickedCb(data_set->dialog, nullptr, nullptr);
  else
    LayoutExpandButtonClickedCb(data_set, nullptr, nullptr);
}

Evas_Object* PaymentRequestDialogEfl::AddShippingAddressLayout() {
  Evas_Object* shipping_address = elm_layout_add(main_layout_);
  elm_layout_file_set(shipping_address,
                      payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "shipping_address");

  evas_object_size_hint_weight_set(shipping_address, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(shipping_address, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);

  evas_object_show(shipping_address);
  elm_object_part_content_set(main_layout_, "swallow_shipping_address",
                              shipping_address);

  // Add add button for adding new shipping address.
  Evas_Object* add_shipping_info_btn = AddButton(
      shipping_address, "ADD", "add_shipping_button", AddShippingAddressInfoCb);

  // Button click item data creation
  LayoutIdData* layout_data = new LayoutIdData;
  layout_data->dialog = this;
  layout_data->id = LAYOUT_SHIPPING_ADDR;
  layout_id_data_list_.push_back(layout_data);
  AddExpandAndShrinkButtons(shipping_address, layout_data);
  AddOverlayButton("shipping_address_overlay_button", layout_data);

  // Label showing latest shipping data
  if (web_shipping_data_profile_list_.size()) {
    std::string shipping_address_data =
        GetShippingAddressRawInfo(web_shipping_data_profile_list_[0]);

    // Set lastly added profile as selected profile by default.
    *shipping_info_profile_.get() = *web_shipping_data_profile_list_[0];
    payment_data_->SetSelectedShippingAddress(shipping_info_profile_.get());

    elm_object_part_text_set(shipping_address, "shipping_text",
                             shipping_address_data.c_str());
    elm_object_signal_emit(shipping_address, "elm,action,hide,add,button", "");
    elm_object_signal_emit(shipping_address,
                           "elm,action,enable,expand,shrink,button", "");
  } else {
    elm_object_part_text_set(shipping_address, "shipping_text", "");
    elm_object_signal_emit(shipping_address,
                           "elm,action,disable,expand,shrink,button", "");
  }

  // Add genlist for Shipping Address
  shipping_genlist_ = elm_genlist_add(shipping_address);
  shipping_radio_group_ = elm_radio_add(shipping_genlist_);
  elm_radio_state_value_set(shipping_radio_group_, -1);

  evas_object_show(shipping_genlist_);
  elm_object_part_content_set(shipping_address, "genlist_container",
                              shipping_genlist_);
  Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
  itc->item_style = "one.icon.one.text";
  itc->func.text_get = NULL;
  itc->func.content_get = ShippingContentGetCb;
  itc->func.del = NULL;

  AutofillGenlistItemData* genlist_item_data = nullptr;
  Evas_Object* radio = nullptr;
  int index = 0;
  for (const auto& profile : web_shipping_data_profile_list_) {
    std::string shipping_address_data = GetShippingAddressRawInfo(profile);
    radio = elm_radio_add(shipping_genlist_);
    genlist_item_data = new AutofillGenlistItemData;
    genlist_item_data->radio = radio;
    genlist_item_data->data = shipping_address_data;
    genlist_item_data->dialog = this;
    genlist_item_data->id = index;
    genlist_item_data->guid = profile->guid();
    shipping_address_data_list_.push_back(std::move(genlist_item_data));
    elm_genlist_item_append(shipping_genlist_, itc,
                            (void*)shipping_address_data_list_[index], NULL,
                            ELM_GENLIST_ITEM_NONE, ShippingAddressItemClickCb,
                            (void*)shipping_address_data_list_[index]);
    index++;
  }
  elm_genlist_realized_items_update(shipping_genlist_);

  // Add 'Add New Address' button.
  Evas_Object* add_new_btn =
      AddCustomButton(shipping_address, "+  Add New Address", "add_new_button",
                      AddShippingAddressInfoCb);

  return shipping_address;
}

void PaymentRequestDialogEfl::ShippingAddressItemClickCb(void* data,
                                                         Evas_Object* obj,
                                                         void* event_info) {
  AutofillGenlistItemData* data_set =
      static_cast<AutofillGenlistItemData*>(data);
  PaymentRequestDialogEfl* dialog = data_set->dialog;
  elm_radio_value_set(data_set->radio, data_set->id);
  Evas_Object* shrink_layout =
      dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);

  if (shrink_layout) {
    elm_object_part_text_set(shrink_layout, "shipping_text",
                             data_set->data.c_str());
    elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    dialog->ShrinkLayoutForLayoutId(dialog->curr_expanded_layout_id_);
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
  }
#if defined(TIZEN_AUTOFILL_SUPPORT)
  autofill::AutofillProfile* profile =
      dialog->GetPersonalDataManager()->GetProfileByGUID(data_set->guid);
  *dialog->shipping_info_profile_.get() = *profile;
  dialog->payment_data_->SetSelectedShippingAddress(
      dialog->shipping_info_profile_.get());
#endif
}

char* PaymentRequestDialogEfl::ShippingOptionTextGetCb(void* data,
                                                       Evas_Object* obj,
                                                       const char* part) {
  ShippingOptionItemData* item_data =
      static_cast<ShippingOptionItemData*>(data);
  if (!strcmp(part, "item_text_main"))
    return strdup(item_data->shipping_type.c_str());
  else if (!strcmp(part, "item_text_sub")) {
    return strdup(
        ConcatCurrenyValue(item_data->currency, item_data->value).c_str());
  }
  return nullptr;
}

Evas_Object* PaymentRequestDialogEfl::ShippingOptionContentGetCb(
    void* data,
    Evas_Object* obj,
    const char* part) {
  ShippingOptionItemData* data_set = static_cast<ShippingOptionItemData*>(data);
  Evas_Object* radio = elm_radio_add(obj);
  data_set->radio = radio;
  elm_radio_state_value_set(radio, data_set->id);
  if (data_set->selected)
    elm_radio_value_set(radio, data_set->id);
  elm_radio_group_add(radio, data_set->dialog->shipping_options_radio_group_);
  return data_set->radio;
}

Evas_Object* PaymentRequestDialogEfl::AddShippingOptionLayout() {
  Evas_Object* shipping_option = elm_layout_add(main_layout_);
  elm_layout_file_set(shipping_option, payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "shipping_option");

  evas_object_size_hint_weight_set(shipping_option, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(shipping_option, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);

  evas_object_show(shipping_option);
  elm_object_part_content_set(main_layout_, "swallow_shipping_option",
                              shipping_option);

  // Button click item data creation
  LayoutIdData* layout_data = new LayoutIdData;
  layout_data->dialog = this;
  layout_data->id = LAYOUT_SHIPPING_OPTION;
  layout_id_data_list_.push_back(layout_data);
  AddExpandAndShrinkButtons(shipping_option, layout_data);
  AddOverlayButton("shipping_option_overlay_button", layout_data);

  // Add genlist for shipping options
  Evas_Object* genlist = elm_genlist_add(shipping_option);
  shipping_options_radio_group_ = elm_radio_add(genlist);
  elm_radio_state_value_set(shipping_options_radio_group_, -1);

  evas_object_show(genlist);
  elm_object_part_content_set(shipping_option, "genlist_container", genlist);
  Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
  itc->item_style = "shipping_option_item";
  itc->func.text_get = ShippingOptionTextGetCb;
  itc->func.content_get = ShippingOptionContentGetCb;
  itc->func.del = NULL;

  shipping_option_data_list_.clear();
  int index = 0;
  for (const auto& option : request_->spec()->details().shipping_options) {
    ShippingOptionItemData* shipping_option_data = new ShippingOptionItemData;
    shipping_option_data->shipping_type = option->label;
    shipping_option_data->currency = option->amount->currency;
    shipping_option_data->value = option->amount->value;
    shipping_option_data->dialog = this;
    shipping_option_data->id = index;
    shipping_option_data->option_id = option->id;
    shipping_option_data->selected = option->selected;
    if (option->selected) {
      elm_object_part_text_set(shipping_option, "shipping_option_type",
                               option->label.c_str());
      elm_object_part_text_set(
          shipping_option, "shipping_option_currency",
          ConcatCurrenyValue(option->amount->currency, option->amount->value)
              .c_str());
    }
    shipping_option_data_list_.push_back(shipping_option_data);
    elm_genlist_item_append(
        genlist, itc, (void*)shipping_option_data_list_[index], NULL,
        ELM_GENLIST_ITEM_NONE, ShippingOptionGenlistItemClickCb,
        (void*)shipping_option_data_list_[index]);
    index++;
  }
  elm_genlist_realized_items_update(genlist);

  return shipping_option;
}

void PaymentRequestDialogEfl::ShippingOptionGenlistItemClickCb(
    void* data,
    Evas_Object* obj,
    void* event_info) {
  ShippingOptionItemData* data_set = static_cast<ShippingOptionItemData*>(data);
  PaymentRequestDialogEfl* dialog = data_set->dialog;
  elm_radio_value_set(data_set->radio, data_set->id);

  for (const auto& item : dialog->shipping_option_data_list_)
    item->selected = false;

  data_set->selected = true;
  dialog->payment_data_->SetSelectedShippingOption(data_set->option_id);
  dialog->db_sync_timer_ =
      ecore_timer_add(DB_SYNC_INTERVAL, UpdateOrderSummaryGenlist, dialog);

  Evas_Object* shrink_layout =
      dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);
  if (shrink_layout) {
    elm_object_part_text_set(shrink_layout, "shipping_option_type",
                             data_set->shipping_type.c_str());
    elm_object_part_text_set(
        shrink_layout, "shipping_option_currency",
        ConcatCurrenyValue(data_set->currency, data_set->value).c_str());
    elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    elm_object_signal_emit(dialog->main_layout_, "elm,action,shrink,mainLayout",
                           "");
    elm_object_signal_emit(dialog->main_layout_,
                           "elm,action,shrink,shippingOption", "");
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
  }
}

char* PaymentRequestDialogEfl::OrderSummaryTextGetCb(void* data,
                                                     Evas_Object* obj,
                                                     const char* part) {
  OrderSummaryItemData* item_data = static_cast<OrderSummaryItemData*>(data);
  if (!strcmp(part, "item_text_left")) {
    return strdup(item_data->label.c_str());
  } else if (!strcmp(part, "item_text_right")) {
    return strdup(
        ConcatCurrenyValue(item_data->currency, item_data->value).c_str());
  }
  return nullptr;
}

Evas_Object* PaymentRequestDialogEfl::AddOrderSummaryLayout() {
  Evas_Object* order_summary = elm_layout_add(main_layout_);
  elm_layout_file_set(order_summary, payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "order_summary");

  evas_object_size_hint_weight_set(order_summary, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(order_summary, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);

  evas_object_show(order_summary);
  elm_object_part_content_set(main_layout_, "swallow_order_summary",
                              order_summary);

  LayoutIdData* layout_data = new LayoutIdData;
  layout_data->dialog = this;
  layout_data->id = LAYOUT_ORDER_SUMMARY;
  layout_id_data_list_.push_back(layout_data);
  AddExpandAndShrinkButtons(order_summary, layout_data);
  AddOverlayButton("order_summary_overlay_button", layout_data);

  // Get the label for total order.
  const char* total_label = (request_->spec()->details().total->label).c_str();

  // Get total amount and total summary label.
  std::string total_label_value_amount =
      request_->spec()->details().total->amount->value;
  std::string total_label_value_currency =
      request_->spec()->details().total->amount->currency;
  std::string total_label_value_string =
      total_label_value_currency + " " + total_label_value_amount;
  const char* total_label_value = (total_label_value_string).c_str();

  base::string16 payment_summary =
      l10n_util::GetStringUTF16(IDS_PAYMENTS_ORDER_SUMMARY_LABEL);

  std::string payment_summary_label_string = base::UTF16ToUTF8(payment_summary);
  const char* payment_summary_label = (payment_summary_label_string).c_str();

  // Add layout parts.
  elm_object_part_text_set(order_summary, "summary_label",
                           payment_summary_label);
  elm_object_part_text_set(order_summary, "total_label", total_label);
  elm_object_part_text_set(order_summary, "total_value", total_label_value);

  // Add genlist for shipping options
  Evas_Object* genlist = elm_genlist_add(order_summary);
  order_summary_radio_group_ = elm_radio_add(genlist);
  elm_radio_state_value_set(order_summary_radio_group_, -1);

  evas_object_show(genlist);
  elm_object_part_content_set(order_summary, "genlist_container", genlist);
  Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
  itc->item_style = "order_summary_item";
  itc->func.text_get = OrderSummaryTextGetCb;
  itc->func.content_get = NULL;
  itc->func.del = NULL;

  order_summary_data_list_.clear();
  int index = 0;
  for (const auto& item : request_->spec()->details().display_items) {
    OrderSummaryItemData* order_summary_data = new OrderSummaryItemData;
    order_summary_data->label = item->label;
    order_summary_data->currency = item->amount->currency;
    order_summary_data->value = item->amount->value;
    order_summary_data->dialog = this;
    order_summary_data->id = index;
    order_summary_data->is_shipping_type = false;
    order_summary_data_list_.push_back(order_summary_data);
    elm_genlist_item_append(
        genlist, itc, (void*)order_summary_data_list_[index], NULL,
        ELM_GENLIST_ITEM_NONE, OrderSummaryGenlistItemClickCb,
        (void*)order_summary_data_list_[index]);
    index++;
  }
  elm_genlist_realized_items_update(genlist);
  order_summary_genlist_ = genlist;

  return order_summary;
}

Eina_Bool PaymentRequestDialogEfl::UpdateOrderSummaryGenlist(void* data) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  ecore_timer_del(dialog->db_sync_timer_);

  // Add genlist for shipping options
  Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
  itc->item_style = "order_summary_item";
  itc->func.text_get = OrderSummaryTextGetCb;
  itc->func.content_get = NULL;
  itc->func.del = NULL;

  elm_genlist_clear(dialog->order_summary_genlist_);
  dialog->order_summary_data_list_.clear();

  int index = 0;
  for (const auto& item : dialog->request_->spec()->details().display_items) {
    OrderSummaryItemData* order_summary_data = new OrderSummaryItemData;
    order_summary_data->label = item->label;
    order_summary_data->currency = item->amount->currency;
    order_summary_data->value = item->amount->value;
    order_summary_data->dialog = dialog;
    order_summary_data->id = index;
    dialog->order_summary_data_list_.push_back(order_summary_data);
    elm_genlist_item_append(dialog->order_summary_genlist_, itc,
                            (void*)dialog->order_summary_data_list_[index],
                            NULL, ELM_GENLIST_ITEM_NONE,
                            OrderSummaryGenlistItemClickCb,
                            (void*)dialog->order_summary_data_list_[index]);
    index++;
  }

  elm_genlist_realized_items_update(dialog->order_summary_genlist_);

  std::string total_currency =
      dialog->request_->spec()->details().total->amount->currency;

  double total_value_with_shipping =
      atof(dialog->request_->spec()->details().total->amount->value.c_str());
  char value[20];
  memset(value, '\0', 20);
  snprintf(value, 20, "%0.2f", total_value_with_shipping);

  std::string final_value = total_currency + " " + value;
  elm_object_part_text_set(dialog->order_summary_layout_, "total_value",
                           final_value.c_str());
  return ECORE_CALLBACK_CANCEL;
}

void PaymentRequestDialogEfl::OrderSummaryGenlistItemClickCb(void* data,
                                                             Evas_Object* obj,
                                                             void* event_info) {
  NOTIMPLEMENTED();
}

Evas_Object* PaymentRequestDialogEfl::AddPaymentMethodLayout() {
  Evas_Object* payment_method = elm_layout_add(main_layout_);
  elm_layout_file_set(payment_method, payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "generic_child_layout");

  evas_object_size_hint_weight_set(payment_method, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(payment_method, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);

  evas_object_show(payment_method);
  elm_object_part_content_set(main_layout_, "swallow_payment_method",
                              payment_method);
  elm_object_part_text_set(payment_method, "label_text", "Payment");

  // Add add button for adding new contacts.
  Evas_Object* add_card_btn =
      AddButton(payment_method, "ADD CARD", "add_button", AddCardCb);

  // Button click item data creation
  LayoutIdData* layout_data = new LayoutIdData;
  layout_data->dialog = this;
  layout_data->id = LAYOUT_PAYMENT_METHOD;
  layout_id_data_list_.push_back(layout_data);
  AddExpandAndShrinkButtons(payment_method, layout_data);
  AddOverlayButton("payment_method_overlay_button", layout_data);

  credit_card_list_.clear();
#if defined(TIZEN_AUTOFILL_SUPPORT)
  credit_card_list_ = GetPersonalDataManager()->GetCreditCards();
#endif
  // Label showing latest credit card number added
  if (credit_card_list_.size()) {
    std::string number = base::UTF16ToASCII(
        credit_card_list_[0]->GetRawInfo((autofill::CREDIT_CARD_NUMBER)));

    // Set lastly added credit card as selected card by default.
    credit_card_ = *credit_card_list_[0];
    payment_data_->SetSelectedCreditCard(&credit_card_);

    std::string card_number_to_display;
    GetCardNumberFromSelectedItem(number, card_number_to_display);
    elm_object_part_text_set(payment_method, "selected_item_text",
                             card_number_to_display.c_str());
    elm_object_signal_emit(payment_method, "elm,action,hide,add,button", "");
    elm_object_signal_emit(payment_method,
                           "elm,action,enable,expand,shrink,button", "");
  } else {
    elm_object_part_text_set(payment_method, "selected_item_text", "");
    elm_object_signal_emit(payment_method,
                           "elm,action,disable,expand,shrink,button", "");
  }

  // Add genlist for Credit Card Number
  credit_card_genlist_ = elm_genlist_add(payment_method);
  credit_card_radio_group_ = elm_radio_add(credit_card_genlist_);
  elm_radio_state_value_set(credit_card_radio_group_, -1);

  evas_object_show(credit_card_genlist_);
  elm_object_part_content_set(payment_method, "genlist_container",
                              credit_card_genlist_);
  Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
  itc->item_style = "two.icon.one.text";
  itc->func.text_get = NULL;
  itc->func.content_get = CreditCardContentGetCb;
  itc->func.del = NULL;

  AutofillGenlistItemData* genlist_item_data = nullptr;
  Evas_Object* radio = nullptr;
  int index = 0;
  for (const auto& card : credit_card_list_) {
    std::string credit_card_number =
        base::UTF16ToASCII(card->GetRawInfo((autofill::CREDIT_CARD_NUMBER)));
    std::string card_number_to_display;
    GetCardNumberFromSelectedItem(credit_card_number, card_number_to_display);
    radio = elm_radio_add(credit_card_genlist_);
    genlist_item_data = new AutofillGenlistItemData;
    genlist_item_data->radio = radio;
    genlist_item_data->data = card_number_to_display;
    genlist_item_data->dialog = this;
    genlist_item_data->id = index;
    genlist_item_data->guid = card->guid();
    credit_card_number_data_list_.push_back(std::move(genlist_item_data));
    elm_genlist_item_append(credit_card_genlist_, itc,
                            (void*)credit_card_number_data_list_[index], NULL,
                            ELM_GENLIST_ITEM_NONE, CreditCardGenlistItemClickCb,
                            (void*)credit_card_number_data_list_[index]);
    index++;
  }
  elm_genlist_realized_items_update(credit_card_genlist_);

  AddCustomButton(payment_method, "+  Add New Card", "add_new_button",
                  AddCardCb);
  return payment_method;
}

void PaymentRequestDialogEfl::CreditCardGenlistItemClickCb(void* data,
                                                           Evas_Object* obj,
                                                           void* event_info) {
  AutofillGenlistItemData* data_set =
      static_cast<AutofillGenlistItemData*>(data);
  PaymentRequestDialogEfl* dialog = data_set->dialog;
  elm_radio_value_set(data_set->radio, data_set->id);
  Evas_Object* shrink_layout =
      dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);

  if (shrink_layout) {
    elm_object_part_text_set(shrink_layout, "selected_item_text",
                             data_set->data.c_str());
    elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    dialog->ShrinkLayoutForLayoutId(dialog->curr_expanded_layout_id_);
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
  }
#if defined(TIZEN_AUTOFILL_SUPPORT)
  autofill::CreditCard* card =
      dialog->GetPersonalDataManager()->GetCreditCardByGUID(data_set->guid);
  dialog->credit_card_ = *card;
  dialog->payment_data_->SetSelectedCreditCard(&dialog->credit_card_);
#endif
}

void PaymentRequestDialogEfl::PhoneGenlistItemClickCb(void* data,
                                                      Evas_Object* obj,
                                                      void* event_info) {
  AutofillGenlistItemData* data_set =
      static_cast<AutofillGenlistItemData*>(data);
  PaymentRequestDialogEfl* dialog = data_set->dialog;
  elm_radio_value_set(data_set->radio, data_set->id);
  Evas_Object* shrink_layout =
      dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);

  if (shrink_layout) {
    elm_object_part_text_set(shrink_layout, "selected_item_text",
                             data_set->data.c_str());
    elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    dialog->ShrinkLayoutForLayoutId(dialog->curr_expanded_layout_id_);
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
  }
#if defined(TIZEN_AUTOFILL_SUPPORT)
  autofill::AutofillProfile* profile =
      dialog->GetPersonalDataManager()->GetProfileByGUID(data_set->guid);
  *dialog->contact_info_profile_.get() = *profile;
  dialog->payment_data_->SetSelectedPhone(dialog->contact_info_profile_.get());
#endif
}

void PaymentRequestDialogEfl::EmailGenlistItemClickCb(void* data,
                                                      Evas_Object* obj,
                                                      void* event_info) {
  AutofillGenlistItemData* data_set =
      static_cast<AutofillGenlistItemData*>(data);
  PaymentRequestDialogEfl* dialog = data_set->dialog;
  elm_radio_value_set(data_set->radio, data_set->id);
  Evas_Object* shrink_layout =
      dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);

  if (shrink_layout) {
    elm_object_part_text_set(shrink_layout, "selected_item_text",
                             data_set->data.c_str());
    elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    dialog->ShrinkLayoutForLayoutId(dialog->curr_expanded_layout_id_);
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
  }
#if defined(TIZEN_AUTOFILL_SUPPORT)
  autofill::AutofillProfile* profile =
      dialog->GetPersonalDataManager()->GetProfileByGUID(data_set->guid);
  *dialog->contact_info_profile_.get() = *profile;
  dialog->payment_data_->SetSelectedEmail(dialog->contact_info_profile_.get());
#endif
}

Evas_Object* PaymentRequestDialogEfl::ContactsContentGetCb(void* data,
                                                           Evas_Object* obj,
                                                           const char* part) {
  AutofillGenlistItemData* data_set =
      static_cast<AutofillGenlistItemData*>(data);
  Evas_Object* radio = elm_radio_add(obj);
  data_set->radio = radio;
  elm_radio_state_value_set(radio, data_set->id);
  elm_radio_group_add(radio, data_set->dialog->contact_radio_group_);
  elm_object_text_set(radio, data_set->data.c_str());
  return data_set->radio;
}

Evas_Object* PaymentRequestDialogEfl::ShippingContentGetCb(void* data,
                                                           Evas_Object* obj,
                                                           const char* part) {
  AutofillGenlistItemData* data_set =
      static_cast<AutofillGenlistItemData*>(data);
  Evas_Object* radio = elm_radio_add(obj);
  data_set->radio = radio;
  elm_radio_state_value_set(radio, data_set->id);
  elm_radio_group_add(radio, data_set->dialog->shipping_radio_group_);
  elm_object_text_set(radio, data_set->data.c_str());
  return data_set->radio;
}

Evas_Object* PaymentRequestDialogEfl::CreditCardContentGetCb(void* data,
                                                             Evas_Object* obj,
                                                             const char* part) {
  AutofillGenlistItemData* data_set =
      static_cast<AutofillGenlistItemData*>(data);
  if (!strcmp(part, "radio_icon")) {
    Evas_Object* radio = elm_radio_add(obj);
    data_set->radio = radio;
    elm_radio_state_value_set(radio, data_set->id);
    elm_radio_group_add(radio, data_set->dialog->credit_card_radio_group_);
    elm_object_text_set(radio, data_set->data.c_str());
    return data_set->radio;
  } else if (!strcmp(part, "card_icon")) {
    CreditCardType card_type =
        data_set->dialog->CheckCreditCardTypeFromDisplayStr(
            data_set->data.c_str());
    Evas_Object* card_icon = elm_icon_add(obj);
    if (card_type == CARD_VISA) {
      elm_image_file_set(
          card_icon, data_set->dialog->payment_edj_path_.AsUTF8Unsafe().c_str(),
          "visa.png");
    } else if (card_type == CARD_AMEX) {
      elm_image_file_set(
          card_icon, data_set->dialog->payment_edj_path_.AsUTF8Unsafe().c_str(),
          "americanexpress.png");
    } else if (card_type == CARD_MASTERCARD) {
      elm_image_file_set(
          card_icon, data_set->dialog->payment_edj_path_.AsUTF8Unsafe().c_str(),
          "mastercard.png");
    }
    return card_icon;
  }
  return nullptr;
}

Evas_Object* PaymentRequestDialogEfl::AddPhoneNumberLayout() {
  Evas_Object* phone_number = elm_layout_add(main_layout_);
  elm_layout_file_set(phone_number, payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "generic_child_layout");

  evas_object_size_hint_weight_set(phone_number, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(phone_number, EVAS_HINT_FILL, EVAS_HINT_FILL);

  evas_object_show(phone_number);
  elm_object_part_content_set(main_layout_, "swallow_phone_number",
                              phone_number);

  // Add add button for adding new contacts.
  Evas_Object* add_contact_info_btn =
      AddButton(phone_number, "ADD", "add_button", AddContactInfoCb);

  // Button click item data creation
  LayoutIdData* layout_data = new LayoutIdData;
  layout_data->dialog = this;
  layout_data->id = LAYOUT_PHONE_NUMBER;
  layout_id_data_list_.push_back(layout_data);
  AddExpandAndShrinkButtons(phone_number, layout_data);
  AddOverlayButton("phone_number_overlay_button", layout_data);

  // Label showing latest phone number
  if (web_contact_info_profiles_list_.size()) {
    std::string number =
        base::UTF16ToUTF8(web_contact_info_profiles_list_[0]->GetRawInfo(
            (autofill::PHONE_HOME_WHOLE_NUMBER)));

    // Set lastly added profile as selected profile by default.
    *contact_info_profile_.get() = *web_contact_info_profiles_list_[0];
    payment_data_->SetSelectedPhone(contact_info_profile_.get());

    elm_object_part_text_set(phone_number, "selected_item_text",
                             number.c_str());
    elm_object_signal_emit(phone_number, "elm,action,hide,add,button", "");
    elm_object_signal_emit(phone_number,
                           "elm,action,enable,expand,shrink,button", "");
  } else {
    elm_object_part_text_set(phone_number, "selected_item_text", "");
    elm_object_signal_emit(phone_number,
                           "elm,action,disable,expand,shrink,button", "");
  }

  // Add genlist for Contact Number
  contact_genlist_ = elm_genlist_add(phone_number);
  contact_radio_group_ = elm_radio_add(contact_genlist_);
  elm_radio_state_value_set(contact_radio_group_, -1);

  evas_object_show(contact_genlist_);
  elm_object_part_content_set(phone_number, "genlist_container",
                              contact_genlist_);
  Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
  itc->item_style = "one.icon.one.text";
  itc->func.text_get = NULL;
  itc->func.content_get = ContactsContentGetCb;
  itc->func.del = NULL;

  AutofillGenlistItemData* genlist_item_data = nullptr;
  Evas_Object* radio = nullptr;
  int index = 0;
  for (const auto& profile : web_contact_info_profiles_list_) {
    std::string phone_number = base::UTF16ToUTF8(
        profile->GetRawInfo((autofill::PHONE_HOME_WHOLE_NUMBER)));
    radio = elm_radio_add(contact_genlist_);
    genlist_item_data = new AutofillGenlistItemData;
    genlist_item_data->radio = radio;
    genlist_item_data->data = phone_number;
    genlist_item_data->dialog = this;
    genlist_item_data->id = index;
    genlist_item_data->guid = profile->guid();
    phone_number_data_list_.push_back(std::move(genlist_item_data));
    elm_genlist_item_append(contact_genlist_, itc,
                            (void*)phone_number_data_list_[index], NULL,
                            ELM_GENLIST_ITEM_NONE, PhoneGenlistItemClickCb,
                            (void*)phone_number_data_list_[index]);
    index++;
  }
  elm_genlist_realized_items_update(contact_genlist_);

  AddCustomButton(phone_number, "+  Add New Contact", "add_new_button",
                  AddContactInfoCb);

  return phone_number;
}

Evas_Object* PaymentRequestDialogEfl::AddEmailAddressLayout() {
  Evas_Object* email_address = elm_layout_add(main_layout_);
  elm_layout_file_set(email_address, payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "generic_child_layout");

  evas_object_size_hint_weight_set(email_address, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(email_address, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);

  evas_object_show(email_address);
  elm_object_part_content_set(main_layout_, "swallow_email_address",
                              email_address);

  // Add add button for adding new contacts.
  Evas_Object* add_email_address_btn =
      AddButton(email_address, "ADD", "add_button", AddContactInfoCb);

  // Button click item data creation
  LayoutIdData* layout_data = new LayoutIdData;
  layout_data->dialog = this;
  layout_data->id = LAYOUT_EMAIL_ADDR;
  layout_id_data_list_.push_back(layout_data);
  AddExpandAndShrinkButtons(email_address, layout_data);
  AddOverlayButton("email_address_overlay_button", layout_data);

  // Label showing latest Email
  if (web_contact_info_profiles_list_.size()) {
    std::string email =
        base::UTF16ToUTF8(web_contact_info_profiles_list_[0]->GetRawInfo(
            (autofill::EMAIL_ADDRESS)));

    // Set lastly added profile as selected profile by default.
    *contact_info_profile_.get() = *web_contact_info_profiles_list_[0];
    payment_data_->SetSelectedEmail(contact_info_profile_.get());

    elm_object_part_text_set(email_address, "selected_item_text",
                             email.c_str());
    elm_object_signal_emit(email_address, "elm,action,hide,add,button", "");
    elm_object_signal_emit(email_address,
                           "elm,action,enable,expand,shrink,button", "");
  } else {
    elm_object_part_text_set(email_address, "selected_item_text", "");
    elm_object_signal_emit(email_address,
                           "elm,action,disable,expand,shrink,button", "");
  }

  // Add genlist for Contact Number
  contact_genlist_ = elm_genlist_add(email_address);
  contact_radio_group_ = elm_radio_add(contact_genlist_);
  elm_radio_state_value_set(contact_radio_group_, -1);

  evas_object_show(contact_genlist_);
  elm_object_part_content_set(email_address, "genlist_container",
                              contact_genlist_);
  Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
  itc->item_style = "one.icon.one.text";
  itc->func.text_get = NULL;
  itc->func.content_get = ContactsContentGetCb;
  itc->func.del = NULL;

  AutofillGenlistItemData* genlist_item_data = nullptr;
  Evas_Object* radio = nullptr;
  int index = 0;
  for (const auto& profile : web_contact_info_profiles_list_) {
    std::string email =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::EMAIL_ADDRESS)));
    radio = elm_radio_add(contact_genlist_);
    genlist_item_data = new AutofillGenlistItemData;
    genlist_item_data->radio = radio;
    genlist_item_data->data = email;
    genlist_item_data->dialog = this;
    genlist_item_data->id = index;
    genlist_item_data->guid = profile->guid();
    email_address_data_list_.push_back(std::move(genlist_item_data));
    elm_genlist_item_append(contact_genlist_, itc,
                            (void*)email_address_data_list_[index], NULL,
                            ELM_GENLIST_ITEM_NONE, EmailGenlistItemClickCb,
                            (void*)email_address_data_list_[index]);
    index++;
  }
  elm_genlist_realized_items_update(contact_genlist_);

  AddCustomButton(email_address, "+  Add New Contact", "add_new_button",
                  AddContactInfoCb);

  return email_address;
}

void PaymentRequestDialogEfl::ContactPhoneNumberInputLayout() {
  base::FilePath edj_dir;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  add_phone_number_path_ =
      edj_dir.Append(FILE_PATH_LITERAL("PaymentAddContact.edj"));

  evas_object_hide(popup_);
  edje_object_signal_emit(main_layout_, "hide,main,layout", "");

  contact_phone_number_input_layout_ = elm_layout_add(main_layout_);
  // Set the layout file.
  elm_layout_file_set(contact_phone_number_input_layout_,
                      add_phone_number_path_.AsUTF8Unsafe().c_str(),
                      "Add_Contact_layout");
  elm_object_signal_emit(contact_phone_number_input_layout_,
                         "elm,action,phone,number", "");

  evas_object_size_hint_weight_set(contact_phone_number_input_layout_,
                                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(contact_phone_number_input_layout_,
                                  EVAS_HINT_FILL, EVAS_HINT_FILL);

  // Add button to cancel Phone Number
  Evas_Object* cancel_add_phone_number_btn =
      AddButton(contact_phone_number_input_layout_, "CANCEL",
                "cancel_add_contact_button", CancelAddPhoneNumberButtonCb);

  // Add button to Finished Phone Number.
  Evas_Object* finished_add_phone_number_btn =
      AddButton(contact_phone_number_input_layout_, "DONE",
                "finished_add_contact_button", FinishedAddPhoneNumberButtonCb);

  Evas_Object* contact_phone_number_entry_ =
      elm_entry_add(contact_phone_number_input_layout_);
  elm_entry_single_line_set(contact_phone_number_entry_, EINA_TRUE);
  elm_entry_scrollable_set(contact_phone_number_entry_, EINA_FALSE);
  evas_object_size_hint_weight_set(contact_phone_number_entry_,
                                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(contact_phone_number_entry_, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);
  elm_entry_input_panel_layout_set(contact_phone_number_entry_,
                                   ELM_INPUT_PANEL_LAYOUT_PHONENUMBER);
  elm_entry_prediction_allow_set(contact_phone_number_entry_, EINA_FALSE);

  Elm_Entry_Filter_Limit_Size entry_limit_size;
  entry_limit_size.max_char_count = 13;
  elm_entry_markup_filter_append(contact_phone_number_entry_,
                                 elm_entry_filter_limit_size,
                                 &entry_limit_size);
  evas_object_smart_callback_add(contact_phone_number_entry_, "changed",
                                 AddPhoneNumberCb, this);
  elm_object_part_content_set(contact_phone_number_input_layout_,
                              "contact_input_entry",
                              contact_phone_number_entry_);
  elm_entry_input_panel_return_key_type_set(
      contact_phone_number_entry_, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
  evas_object_show(contact_phone_number_entry_);
  evas_object_show(contact_phone_number_input_layout_);
}

void PaymentRequestDialogEfl::ContactEmailAddressInputLayout() {
  base::FilePath edj_dir;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  add_email_address_path_ =
      edj_dir.Append(FILE_PATH_LITERAL("PaymentAddContact.edj"));

  evas_object_hide(popup_);
  edje_object_signal_emit(main_layout_, "hide,main,layout", "");

  contact_email_address_input_layout_ = elm_layout_add(main_layout_);
  // Set the layout file.
  elm_layout_file_set(contact_email_address_input_layout_,
                      add_email_address_path_.AsUTF8Unsafe().c_str(),
                      "Add_Contact_layout");
  elm_object_signal_emit(contact_email_address_input_layout_,
                         "elm,action,email,address", "");

  evas_object_size_hint_weight_set(contact_email_address_input_layout_,
                                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(contact_email_address_input_layout_,
                                  EVAS_HINT_FILL, EVAS_HINT_FILL);

  // Add button to cancel email_address
  Evas_Object* cancel_add_email_address_btn =
      AddButton(contact_email_address_input_layout_, "CANCEL",
                "cancel_add_contact_button", CancelAddEmailAddressButtonCb);

  // Add button to Finished email_address.
  Evas_Object* finished_add_email_address_btn =
      AddButton(contact_email_address_input_layout_, "DONE",
                "finished_add_contact_button", FinishedAddEmailAddressButtonCb);

  Evas_Object* contact_email_address_entry_ =
      elm_entry_add(contact_email_address_input_layout_);
  elm_entry_single_line_set(contact_email_address_entry_, EINA_TRUE);
  elm_entry_scrollable_set(contact_email_address_entry_, EINA_FALSE);
  evas_object_size_hint_weight_set(contact_email_address_entry_,
                                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(contact_phone_number_entry_, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);
  elm_entry_input_panel_layout_set(contact_email_address_entry_,
                                   ELM_INPUT_PANEL_LAYOUT_EMAIL);
  elm_entry_prediction_allow_set(contact_phone_number_entry_, EINA_FALSE);

  evas_object_smart_callback_add(contact_email_address_entry_, "changed",
                                 AddEmailAddressCb, this);
  elm_object_part_content_set(contact_email_address_input_layout_,
                              "contact_input_entry",
                              contact_email_address_entry_);
  elm_entry_input_panel_return_key_type_set(
      contact_email_address_entry_, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
  evas_object_show(contact_email_address_entry_);
  evas_object_show(contact_email_address_input_layout_);
}

void PaymentRequestDialogEfl::CancelAddEmailAddressButtonCb(void* data,
                                                            Evas_Object* obj,
                                                            void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  if (dialog->contact_email_address_input_layout_) {
    evas_object_del(dialog->contact_email_address_input_layout_);
    dialog->contact_email_address_input_layout_ = nullptr;
    evas_object_show(dialog->popup_);
  }
}

void PaymentRequestDialogEfl::CancelAddPhoneNumberButtonCb(void* data,
                                                           Evas_Object* obj,
                                                           void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  if (dialog->contact_phone_number_input_layout_) {
    evas_object_del(dialog->contact_phone_number_input_layout_);
    dialog->contact_phone_number_input_layout_ = nullptr;
    evas_object_show(dialog->popup_);
  }
}

void PaymentRequestDialogEfl::GetCardNumberFromSelectedItem(
    const std::string& number,
    std::string& number_to_display) {
  int len = number.length();
  if (!len) {
    number_to_display = "Unknown Card";
    return;
  }
  if (number[0] == '4') {
    number_to_display = "Visa***";
  }
  if (len > 1) {
    int first_two_digits = (number[0] - '0') * 10 + (number[1] - '0');
    if (first_two_digits == 34 || first_two_digits == 37) {
      number_to_display = "Amex***";
    } else if (first_two_digits >= 51 && first_two_digits <= 55) {
      number_to_display = "Master***";
    }
  }
  number_to_display += number.substr(len - 4);
}

Eina_Bool PaymentRequestDialogEfl::CreditCardDbSyncCb(void* data) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  ecore_timer_del(dialog->db_sync_timer_);
  dialog->credit_card_list_.clear();
  dialog->credit_card_list_ =
      dialog->GetPersonalDataManager()->GetCreditCards();
  dialog->credit_card_radio_group_ =
      elm_radio_add(dialog->credit_card_genlist_);
  elm_radio_state_value_set(dialog->credit_card_radio_group_, -1);

  // Show the recently added credit card number and shrink the drop down list
  if (dialog->credit_card_list_.size()) {
    std::string number =
        base::UTF16ToUTF8(dialog->credit_card_list_[0]->GetRawInfo(
            (autofill::CREDIT_CARD_NUMBER)));
    std::string card_number_to_display;

    dialog->GetCardNumberFromSelectedItem(number, card_number_to_display);
    elm_object_part_text_set(dialog->payment_method_layout_,
                             "selected_item_text",
                             card_number_to_display.c_str());
    elm_object_signal_emit(dialog->payment_method_layout_,
                           "elm,action,hide,add,button", "");
    elm_object_signal_emit(dialog->payment_method_layout_,
                           "elm,action,enable,expand,shrink,button", "");
  } else {
    elm_object_part_text_set(dialog->payment_method_layout_,
                             "selected_item_text", "");
    elm_object_signal_emit(dialog->payment_method_layout_,
                           "elm,action,disable,expand,shrink,button", "");
  }
  Evas_Object* shrink_layout =
      dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);

  // Shrink Curr expanded Layout
  if (shrink_layout) {
    elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    dialog->ShrinkLayoutForLayoutId(dialog->curr_expanded_layout_id_);
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
  }

  Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
  itc->item_style = "two.icon.one.text";
  itc->func.text_get = NULL;
  itc->func.content_get = CreditCardContentGetCb;
  itc->func.del = NULL;
  elm_genlist_clear(dialog->credit_card_genlist_);

  AutofillGenlistItemData* genlist_item_data = nullptr;
  Evas_Object* radio = nullptr;
  int index = 0;
  for (const auto& card_number_list : dialog->credit_card_number_data_list_)
    delete card_number_list;

  dialog->credit_card_number_data_list_.clear();
  for (const auto& card : dialog->credit_card_list_) {
    std::string credit_card_number =
        base::UTF16ToASCII(card->GetRawInfo((autofill::CREDIT_CARD_NUMBER)));
    std::string card_number_to_display;
    dialog->GetCardNumberFromSelectedItem(credit_card_number,
                                          card_number_to_display);
    radio = elm_radio_add(dialog->credit_card_genlist_);
    genlist_item_data = new AutofillGenlistItemData;
    genlist_item_data->radio = radio;
    genlist_item_data->data = card_number_to_display;
    genlist_item_data->dialog = dialog;
    genlist_item_data->id = index;
    genlist_item_data->guid = card->guid();
    dialog->credit_card_number_data_list_.push_back(
        std::move(genlist_item_data));
    elm_genlist_item_append(
        dialog->credit_card_genlist_, itc,
        (void*)dialog->credit_card_number_data_list_[index], NULL,
        ELM_GENLIST_ITEM_NONE, CreditCardGenlistItemClickCb,
        (void*)dialog->credit_card_number_data_list_[index]);
    index++;
  }
  elm_genlist_realized_items_update(dialog->credit_card_genlist_);
  dialog->card_details_added_.ResetValues();
  return ECORE_CALLBACK_CANCEL;
}

Eina_Bool PaymentRequestDialogEfl::ShippingDbSyncCb(void* data) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  ecore_timer_del(dialog->db_sync_timer_);
  dialog->web_shipping_data_profile_list_.clear();
  dialog->shipping_data_profile_list_ =
      dialog->GetPersonalDataManager()->GetProfiles();
  for (const auto& profile : dialog->shipping_data_profile_list_) {
    std::string country = base::UTF16ToUTF8(
        profile->GetRawInfo((autofill::ADDRESS_HOME_COUNTRY)));
    std::string name =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::NAME_FIRST)));
    std::string organisation =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::COMPANY_NAME)));
    std::string street_address =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::ADDRESS_HOME_LINE1)));
    std::string county =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::ADDRESS_HOME_STATE)));
    std::string postcode =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::ADDRESS_HOME_ZIP)));
    std::string phone = base::UTF16ToUTF8(
        profile->GetRawInfo((autofill::PHONE_HOME_WHOLE_NUMBER)));
    if (!country.empty() && !name.empty() && !street_address.empty() &&
        !postcode.empty() && !phone.empty()) {
      dialog->web_shipping_data_profile_list_.push_back(profile);
    }
  }

  dialog->shipping_radio_group_ = elm_radio_add(dialog->shipping_genlist_);
  elm_radio_state_value_set(dialog->shipping_radio_group_, -1);

  // Show the recently added shipping address and shrink the drop down list
  if (dialog->web_shipping_data_profile_list_.size()) {
    std::string shipping_address_data =
        GetShippingAddressRawInfo(dialog->web_shipping_data_profile_list_[0]);
    elm_object_part_text_set(dialog->shipping_address_layout_, "shipping_text",
                             shipping_address_data.c_str());
    elm_object_signal_emit(dialog->shipping_address_layout_,
                           "elm,action,hide,add,button", "");
    elm_object_signal_emit(dialog->shipping_address_layout_,
                           "elm,action,enable,expand,shrink,button", "");
  } else {
    elm_object_part_text_set(dialog->shipping_address_layout_, "shipping_text",
                             "");
    elm_object_signal_emit(dialog->shipping_address_layout_,
                           "elm,action,disable,expand,shrink,button", "");
  }

  Evas_Object* shrink_layout =
      dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);

  // Shrink Curr expanded Layout
  if (shrink_layout) {
    elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    dialog->ShrinkLayoutForLayoutId(dialog->curr_expanded_layout_id_);
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
  }

  Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
  itc->item_style = "one.icon.one.text";
  itc->func.text_get = NULL;
  itc->func.content_get = ShippingContentGetCb;
  itc->func.del = NULL;
  elm_genlist_clear(dialog->shipping_genlist_);

  AutofillGenlistItemData* genlist_item_data = nullptr;
  Evas_Object* radio = nullptr;
  int index = 0;
  for (const auto& address_list : dialog->shipping_address_data_list_)
    delete address_list;

  dialog->shipping_address_data_list_.clear();
  for (const auto& profile : dialog->web_shipping_data_profile_list_) {
    std::string shipping_address_data = GetShippingAddressRawInfo(profile);
    radio = elm_radio_add(dialog->shipping_genlist_);
    genlist_item_data = new AutofillGenlistItemData;
    genlist_item_data->radio = radio;
    genlist_item_data->data = shipping_address_data;
    genlist_item_data->dialog = dialog;
    genlist_item_data->id = index;
    genlist_item_data->guid = profile->guid();
    dialog->shipping_address_data_list_.push_back(std::move(genlist_item_data));
    elm_genlist_item_append(dialog->shipping_genlist_, itc,
                            (void*)dialog->shipping_address_data_list_[index],
                            NULL, ELM_GENLIST_ITEM_NONE,
                            ShippingAddressItemClickCb,
                            (void*)dialog->shipping_address_data_list_[index]);
    index++;
  }

  elm_genlist_realized_items_update(dialog->shipping_genlist_);
  return ECORE_CALLBACK_CANCEL;
}

Eina_Bool PaymentRequestDialogEfl::ContactsDbSyncCb(void* data) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  ecore_timer_del(dialog->db_sync_timer_);
  dialog->web_contact_info_profiles_list_.clear();
  dialog->contact_info_profiles_list_ =
      dialog->GetPersonalDataManager()->GetProfiles();

  if (dialog->request_->spec()->request_payer_phone()) {
    for (const auto& profile : dialog->contact_info_profiles_list_) {
      std::string number = base::UTF16ToUTF8(
          profile->GetRawInfo((autofill::PHONE_HOME_WHOLE_NUMBER)));
      if (!number.empty()) {
        dialog->web_contact_info_profiles_list_.push_back(profile);
      }
    }
  } else if (dialog->request_->spec()->request_payer_email()) {
    for (const auto& profile : dialog->contact_info_profiles_list_) {
      std::string email_address =
          base::UTF16ToUTF8(profile->GetRawInfo((autofill::EMAIL_ADDRESS)));
      if (!email_address.empty()) {
        dialog->web_contact_info_profiles_list_.push_back(profile);
      }
    }
  }

  dialog->contact_radio_group_ = elm_radio_add(dialog->contact_genlist_);
  elm_radio_state_value_set(dialog->contact_radio_group_, -1);

  // Show the recently added contact number and shrink the drop down list
  if (dialog->web_contact_info_profiles_list_.size()) {
    if (dialog->request_->spec()->request_payer_phone()) {
      std::string number = base::UTF16ToUTF8(
          dialog->web_contact_info_profiles_list_[0]->GetRawInfo(
              (autofill::PHONE_HOME_WHOLE_NUMBER)));
      elm_object_part_text_set(dialog->contact_phone_number_layout_,
                               "selected_item_text", number.c_str());
      elm_object_signal_emit(dialog->contact_phone_number_layout_,
                             "elm,action,hide,add,button", "");
      elm_object_signal_emit(dialog->contact_phone_number_layout_,
                             "elm,action,enable,expand,shrink,button", "");
    } else if (dialog->request_->spec()->request_payer_email()) {
      std::string email = base::UTF16ToUTF8(
          dialog->web_contact_info_profiles_list_[0]->GetRawInfo(
              (autofill::EMAIL_ADDRESS)));
      elm_object_part_text_set(dialog->contact_email_address_layout_,
                               "selected_item_text", email.c_str());
      elm_object_signal_emit(dialog->contact_email_address_layout_,
                             "elm,action,hide,add,button", "");
      elm_object_signal_emit(dialog->contact_email_address_layout_,
                             "elm,action,enable,expand,shrink,button", "");
    }
  } else {
    if (dialog->request_->spec()->request_payer_phone()) {
      elm_object_part_text_set(dialog->contact_phone_number_layout_,
                               "selected_item_text", "");
    } else if (dialog->request_->spec()->request_payer_email()) {
      elm_object_part_text_set(dialog->contact_email_address_layout_,
                               "selected_item_text", "");
    }
  }
  Evas_Object* shrink_layout =
      dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);

  // Shrink Curr expanded Layout
  if (shrink_layout) {
    elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    dialog->ShrinkLayoutForLayoutId(dialog->curr_expanded_layout_id_);
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
  }

  Elm_Genlist_Item_Class* itc = elm_genlist_item_class_new();
  itc->item_style = "one.icon.one.text";
  itc->func.text_get = NULL;
  itc->func.content_get = ContactsContentGetCb;
  itc->func.del = NULL;
  elm_genlist_clear(dialog->contact_genlist_);

  AutofillGenlistItemData* genlist_item_data = nullptr;
  Evas_Object* radio = nullptr;
  int index = 0;
  if (dialog->request_->spec()->request_payer_phone()) {
    for (const auto& phone_list : dialog->phone_number_data_list_)
      delete phone_list;

    dialog->phone_number_data_list_.clear();
  } else if (dialog->request_->spec()->request_payer_email()) {
    for (const auto& email_list : dialog->email_address_data_list_)
      delete email_list;

    dialog->email_address_data_list_.clear();
  }

  if (dialog->request_->spec()->request_payer_phone()) {
    for (const auto& profile : dialog->web_contact_info_profiles_list_) {
      std::string phone_number = base::UTF16ToUTF8(
          profile->GetRawInfo((autofill::PHONE_HOME_WHOLE_NUMBER)));
      radio = elm_radio_add(dialog->contact_genlist_);
      genlist_item_data = new AutofillGenlistItemData;
      genlist_item_data->radio = radio;
      genlist_item_data->data = phone_number;
      genlist_item_data->dialog = dialog;
      genlist_item_data->id = index;
      genlist_item_data->guid = profile->guid();
      dialog->phone_number_data_list_.push_back(std::move(genlist_item_data));
      elm_genlist_item_append(dialog->contact_genlist_, itc,
                              (void*)dialog->phone_number_data_list_[index],
                              NULL, ELM_GENLIST_ITEM_NONE,
                              PhoneGenlistItemClickCb,
                              (void*)dialog->phone_number_data_list_[index]);
      index++;
    }
  } else if (dialog->request_->spec()->request_payer_email()) {
    for (const auto& profile : dialog->web_contact_info_profiles_list_) {
      std::string email_address =
          base::UTF16ToUTF8(profile->GetRawInfo((autofill::EMAIL_ADDRESS)));
      radio = elm_radio_add(dialog->contact_genlist_);
      genlist_item_data = new AutofillGenlistItemData;
      genlist_item_data->radio = radio;
      genlist_item_data->data = email_address;
      genlist_item_data->dialog = dialog;
      genlist_item_data->id = index;
      genlist_item_data->guid = profile->guid();
      dialog->email_address_data_list_.push_back(std::move(genlist_item_data));
      elm_genlist_item_append(dialog->contact_genlist_, itc,
                              (void*)dialog->email_address_data_list_[index],
                              NULL, ELM_GENLIST_ITEM_NONE,
                              EmailGenlistItemClickCb,
                              (void*)dialog->email_address_data_list_[index]);
      index++;
    }
  }
  elm_genlist_realized_items_update(dialog->contact_genlist_);
  return ECORE_CALLBACK_CANCEL;
}

void PaymentRequestDialogEfl::FinishedAddPhoneNumberButtonCb(void* data,
                                                             Evas_Object* obj,
                                                             void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  if (dialog->contact_info_details_->contact_phone_number_.empty()) {
    elm_entry_cursor_end_set(dialog->contact_phone_number_entry_);
    elm_object_focus_set(dialog->contact_phone_number_entry_, EINA_TRUE);
  } else {
    const base::string16& phone_number = base::UTF8ToUTF16(
        dialog->contact_info_details_->contact_phone_number_.c_str());
    dialog->contact_info_profile_.reset(new autofill::AutofillProfile());
    dialog->contact_info_profile_->SetRawInfo(
        (autofill::PHONE_HOME_WHOLE_NUMBER), phone_number);
#if defined(TIZEN_AUTOFILL_SUPPORT)
    dialog->GetPersonalDataManager()->AddProfile(
        *(dialog->contact_info_profile_));
    dialog->db_sync_timer_ =
        ecore_timer_add(DB_SYNC_INTERVAL, ContactsDbSyncCb, data);
#endif
    dialog->payment_data_->SetSelectedPhone(
        dialog->contact_info_profile_.get());
    if (dialog->contact_phone_number_input_layout_) {
      evas_object_del(dialog->contact_phone_number_input_layout_);
      dialog->contact_phone_number_input_layout_ = nullptr;
      evas_object_show(dialog->popup_);
    }
  }
}

void PaymentRequestDialogEfl::AddPhoneNumberCb(void* data,
                                               Evas_Object* obj,
                                               void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->contact_info_details_->contact_phone_number_ =
      elm_entry_entry_get(obj);
}

void PaymentRequestDialogEfl::FinishedAddEmailAddressButtonCb(
    void* data,
    Evas_Object* obj,
    void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  if (dialog->contact_info_details_->contact_email_address_.empty()) {
    elm_entry_cursor_end_set(dialog->contact_email_address_entry_);
    elm_object_focus_set(dialog->contact_email_address_entry_, EINA_TRUE);
  } else {
    const base::string16& email_address = base::UTF8ToUTF16(
        dialog->contact_info_details_->contact_email_address_.c_str());
    dialog->contact_info_profile_.reset(new autofill::AutofillProfile());
    dialog->contact_info_profile_->SetRawInfo((autofill::EMAIL_ADDRESS),
                                              email_address);
#if defined(TIZEN_AUTOFILL_SUPPORT)
    dialog->GetPersonalDataManager()->AddProfile(
        *(dialog->contact_info_profile_));
    dialog->db_sync_timer_ =
        ecore_timer_add(DB_SYNC_INTERVAL, ContactsDbSyncCb, data);
#endif
    dialog->payment_data_->SetSelectedEmail(
        dialog->contact_info_profile_.get());
    if (dialog->contact_email_address_input_layout_) {
      evas_object_del(dialog->contact_email_address_input_layout_);
      dialog->contact_email_address_input_layout_ = nullptr;
      evas_object_show(dialog->popup_);
    }
  }
}

void PaymentRequestDialogEfl::AddEmailAddressCb(void* data,
                                                Evas_Object* obj,
                                                void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->contact_info_details_->contact_email_address_ =
      elm_entry_entry_get(obj);
}

void PaymentRequestDialogEfl::ShowCvcInputLayout() {
  base::FilePath edj_dir;
  PathService::Get(PathsEfl::EDJE_RESOURCE_DIR, &edj_dir);
  cvc_edj_path_ = edj_dir.Append(FILE_PATH_LITERAL("PaymentCvc.edj"));

  evas_object_hide(popup_);
  edje_object_signal_emit(main_layout_, "hide,main,layout", "");

  cvc_layout_ = elm_layout_add(main_layout_);
  // Set the layout file.
  elm_layout_file_set(cvc_layout_, cvc_edj_path_.AsUTF8Unsafe().c_str(),
                      "CVC_layout");

  evas_object_size_hint_weight_set(cvc_layout_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(cvc_layout_, EVAS_HINT_FILL, EVAS_HINT_FILL);

  cvc_entry_ = elm_entry_add(cvc_layout_);
  elm_entry_single_line_set(cvc_entry_, EINA_TRUE);
  elm_entry_scrollable_set(cvc_entry_, EINA_FALSE);
  evas_object_size_hint_weight_set(cvc_entry_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(cvc_entry_, EVAS_HINT_FILL, EVAS_HINT_FILL);
  elm_entry_input_panel_layout_set(cvc_entry_,
                                   ELM_INPUT_PANEL_LAYOUT_NUMBERONLY);
  elm_entry_prediction_allow_set(cvc_entry_, EINA_FALSE);

  Elm_Entry_Filter_Limit_Size entry_limit_size;
  entry_limit_size.max_char_count = 3;
  elm_entry_markup_filter_append(cvc_entry_, elm_entry_filter_limit_size,
                                 &entry_limit_size);
  elm_object_part_content_set(cvc_layout_, "cvc_number_input_entry",
                              cvc_entry_);
  elm_entry_password_set(cvc_entry_, EINA_TRUE);
  elm_entry_input_panel_return_key_type_set(
      cvc_entry_, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
  elm_object_part_text_set(cvc_entry_, "elm.guide", "CVC");

  // Add button to cancel CVC.
  Evas_Object* cancel_cvc_btn =
      AddButton(cvc_layout_, "CANCEL", "cancel_cvc_button", CancelCvcButtonCb);

  // Add button to confirm CVC.
  Evas_Object* confirm_cvc_btn = AddButton(
      cvc_layout_, "CONFIRM", "confirm_cvc_button", ConfirmCvcButtonCb);
  elm_object_disabled_set(confirm_cvc_btn, EINA_TRUE);

  evas_object_smart_callback_add(cvc_entry_, "changed", CvcEntryChangedCb,
                                 confirm_cvc_btn);

  std::string number = base::UTF16ToASCII(
      credit_card_.GetRawInfo((autofill::CREDIT_CARD_NUMBER)));
  std::string phone_number_to_display;
  GetCardNumberFromSelectedItem(number, phone_number_to_display);
  std::string card_number = "Enter the CVC for " + phone_number_to_display;
  elm_object_part_text_set(cvc_layout_, "cvc_title", card_number.c_str());
#if defined(OS_TIZEN)
  eext_object_event_callback_add(cvc_layout_, EEXT_CALLBACK_BACK,
                                 CancelCvcButtonCb, this);
#endif
  evas_object_show(cvc_entry_);
  evas_object_show(cvc_layout_);
}

CreditCardType PaymentRequestDialogEfl::CheckCreditCardTypeFromDisplayStr(
    const char* display_str) {
  std::string card_display_str = display_str;
  if (!card_display_str.compare(0, strlen("Visa"), "Visa"))
    return CARD_VISA;
  else if (!card_display_str.compare(0, strlen("Amex"), "Amex"))
    return CARD_AMEX;
  else if (!card_display_str.compare(0, strlen("Master"), "Master"))
    return CARD_MASTERCARD;

  return CARD_UNKNOWN;
}

CreditCardType PaymentRequestDialogEfl::CheckCreditCardType(
    const char* number) {
  int len = strlen(number);
  if (!len) {
    elm_object_signal_emit(card_input_layout_, "elm,action,show,nothing", "");
    return CARD_UNKNOWN;
  }
  if (number[0] == '4') {
    elm_object_signal_emit(card_input_layout_, "elm,action,show,visa", "");
    return CARD_VISA;
  }
  if (len > 1) {
    int first_two_digits = (number[0] - '0') * 10 + (number[1] - '0');
    if (first_two_digits == 34 || first_two_digits == 37) {
      elm_object_signal_emit(card_input_layout_, "elm,action,show,amex", "");
      return CARD_AMEX;
    } else if (first_two_digits >= 51 && first_two_digits <= 55) {
      elm_object_signal_emit(card_input_layout_, "elm,action,show,mastercard",
                             "");
      return CARD_MASTERCARD;
    }
  }
  elm_object_signal_emit(card_input_layout_, "elm,action,show,nothing", "");
  return CARD_UNKNOWN;
}

void PaymentRequestDialogEfl::AddCardNumberCb(void* data,
                                              Evas_Object* obj,
                                              void* event_info) {
  const char* number = elm_entry_entry_get(obj);
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  std::string numStr = std::string(number);
  numStr.erase(std::remove(numStr.begin(), numStr.end(), '-'), numStr.end());
  dialog->credit_card_.SetRawInfo(autofill::CREDIT_CARD_NUMBER,
                                  base::ASCIIToUTF16(numStr.c_str()));
  dialog->card_details_added_.card_number_added_ = true;
}

void PaymentRequestDialogEfl::ValidateCardNameCb(void* data,
                                                 Evas_Object* obj,
                                                 void* event_info) {
  const char* name = elm_entry_entry_get(obj);
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->credit_card_.SetRawInfo(autofill::CREDIT_CARD_NAME_FULL,
                                  base::ASCIIToUTF16(name));
  dialog->card_details_added_.card_name_added_ = true;
}

void PaymentRequestDialogEfl::AddCardNameCb(void* data,
                                            Evas_Object* obj,
                                            void* event_info) {
  const char* name = elm_entry_entry_get(obj);
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->credit_card_.SetRawInfo(autofill::CREDIT_CARD_NAME_FULL,
                                  base::ASCIIToUTF16(name));
  dialog->card_details_added_.card_name_added_ = true;
  elm_object_focus_set(dialog->card_number_entry_, EINA_TRUE);
  elm_entry_cursor_end_set(dialog->card_number_entry_);
}

void PaymentRequestDialogEfl::ValidateCardNumberCb(void* data,
                                                   Evas_Object* obj,
                                                   void* event_info) {
  const char* number = elm_entry_entry_get(obj);
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);

  // Check and set credit card type
  dialog->card_type_ = dialog->CheckCreditCardType(number);

  int len = strlen(number);
  bool greater = (len > dialog->card_number_length_) ? true : false;
  dialog->card_number_length_ = len;
  if (len > 22) {
    std::string numStr = std::string(number);
    elm_entry_entry_set(obj, numStr.substr(0, numStr.size() - 1).c_str());
  }
  // Validate XXXX-XXXX-XXXX-XXXX
  if (len > 19)
    elm_object_signal_emit(dialog->card_input_layout_, "elm,action,show,error",
                           "");
  else
    elm_object_signal_emit(dialog->card_input_layout_, "elm,action,hide,error",
                           "");

  // Add '-' after every 4 digits
  if (len % 5 == 4 && len < 19) {
    if (greater) {
      elm_entry_entry_set(obj, (number + std::string("-")).c_str());
      elm_entry_cursor_end_set(obj);
    } else {
      std::string numStr = std::string(number);
      elm_entry_entry_set(obj, numStr.substr(0, numStr.size() - 1).c_str());
    }
  }
  // remove - from number
  std::string numStr = std::string(number);
  numStr.erase(std::remove(numStr.begin(), numStr.end(), '-'), numStr.end());
  dialog->credit_card_.SetRawInfo(autofill::CREDIT_CARD_NUMBER,
                                  base::ASCIIToUTF16(numStr.c_str()));
  dialog->card_details_added_.card_number_added_ = true;
}

void PaymentRequestDialogEfl::ShippingNameCb(void* data,
                                             Evas_Object* obj,
                                             void* event_info) {
  ItemData* item_data = static_cast<ItemData*>(data);
  const char* name = elm_entry_entry_get(obj);
  if (strlen(name) > 0) {
    elm_object_signal_emit(item_data->dialog->shipping_address_layout_,
                           "elm,action,not,show,requiredFieldError,name", "");
    item_data->dialog->shipping_address_data_->shippingName = name;
  }
}

void PaymentRequestDialogEfl::ShippingAddressFieldValueChangedCb(
    void* data,
    Evas_Object* obj,
    void* event_info) {
  ItemData* item_data = static_cast<ItemData*>(data);
  PaymentRequestDialogEfl* dialog = item_data->dialog;
  switch (item_data->type) {
    case ShippingAddressFieldType::ORGANISATION:
      dialog->shipping_address_data_->shippingOrganisation =
          elm_entry_entry_get(obj);
      break;
    case ShippingAddressFieldType::STREETADDRESS:
      dialog->shipping_address_data_->shippingStreetAddress =
          elm_entry_entry_get(obj);
      break;
    case ShippingAddressFieldType::POSTALTOWN:
      dialog->shipping_address_data_->shippingPostalTown =
          elm_entry_entry_get(obj);
      break;
    case ShippingAddressFieldType::COUNTY:
      dialog->shipping_address_data_->shippingCounty = elm_entry_entry_get(obj);
      break;
    case ShippingAddressFieldType::POSTCODE:
      dialog->shipping_address_data_->shippingPostcode =
          elm_entry_entry_get(obj);
      break;
    case ShippingAddressFieldType::PHONENUMBER:
      dialog->shipping_address_data_->shippingPhonenumber =
          elm_entry_entry_get(obj);
      break;
  }
}

void PaymentRequestDialogEfl::ShippingCtxpopupItemCb(void* data,
                                                     Evas_Object* obj,
                                                     void* event_info) {
  CtxpopupItemData* data_set = static_cast<CtxpopupItemData*>(data);
  PaymentRequestDialogEfl* dialog = data_set->dialog;
  Evas_Object* layout = dialog->shipping_address_input_layout_;
  Evas_Object* btn = data_set->btn;
  Evas_Object* ctxpopup = data_set->ctxpopup;
  dialog->shipping_address_data_->shippingCountry = data_set->data.substr(0, 2);
  elm_object_domain_translatable_part_text_set(
      btn, NULL, "WebKit", data_set->data.substr(2).c_str());
  evas_object_del(ctxpopup);
}

void PaymentRequestDialogEfl::ShowCtxPopupCountryRegionCb(void* data,
                                                          Evas_Object* obj,
                                                          void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  Evas_Object* layout = dialog->shipping_address_input_layout_;
  Evas_Object* ctxpopup = elm_ctxpopup_add(dialog->web_view_->GetElmWindow());

  elm_object_style_set(ctxpopup, "default");
#if defined(OS_TIZEN)
  eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK,
                                 eext_ctxpopup_back_cb, NULL);
#endif
  evas_object_smart_callback_add(ctxpopup, "dismissed", DismissedCb, NULL);

// TODO: Adding more (> ~30) items to ctx_popup via elm_ctxpopup_item_append
// API is taking long time. So, currently we are adding just 5 items. Need to
// discuss with platform team about the delay caused by the API and fix
// accordingly.
#if 1
  const char* country_codes[6] = {"IN", "US", "GB", "KR", "JP", "CN"};
  const char* country_names[6] = {"India",          "United States of America",
                                  "United Kingdom", "South Korea",
                                  "Japan",          "China"};
  for (size_t i = 0; i < 6; i++) {
    std::string code_name;
    code_name += country_codes[i];
    code_name += country_names[i];
    dialog->ShippingAppendItemToContextPopup(dialog, obj, ctxpopup, code_name);
  }
#else
  // Add country list to menu items
  for (const auto& item : dialog->countries_) {
    std::string code_name;
    code_name += item->country_code();
    code_name += base::UTF16ToUTF8(item->name());
    dialog->ShippingAppendItemToContextPopup(dialog, obj, ctxpopup, code_name);
  }
#endif

  // Change position of the popup
  Evas_Coord x, y, w, h;
  evas_object_geometry_get(obj, &x, &y, &w, &h);
  evas_object_move(ctxpopup, x + (w / 2), y + (h / 2));
  evas_object_show(ctxpopup);
}

void PaymentRequestDialogEfl::CancelShippingAddressButtonCb(void* data,
                                                            Evas_Object* obj,
                                                            void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->ClearCtxpopupDataList();
  if (dialog->shipping_address_input_layout_) {
    evas_object_del(dialog->shipping_address_input_layout_);
    dialog->shipping_address_input_layout_ = nullptr;
    evas_object_show(dialog->popup_);
  }
}

void PaymentRequestDialogEfl::FinishedShippingAddressButtonCb(
    void* data,
    Evas_Object* obj,
    void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  const base::string16& country =
      base::UTF8ToUTF16(dialog->shipping_address_data_->shippingCountry);
  const base::string16& name =
      base::UTF8ToUTF16(dialog->shipping_address_data_->shippingName);
  const base::string16& organisation =
      base::UTF8ToUTF16(dialog->shipping_address_data_->shippingOrganisation);
  const base::string16& streetAddress =
      base::UTF8ToUTF16(dialog->shipping_address_data_->shippingStreetAddress);
  const base::string16& postalTown =
      base::UTF8ToUTF16(dialog->shipping_address_data_->shippingPostalTown);
  const base::string16& county =
      base::UTF8ToUTF16(dialog->shipping_address_data_->shippingCounty);
  const base::string16& postcode =
      base::UTF8ToUTF16(dialog->shipping_address_data_->shippingPostcode);
  const base::string16& phoneNumber =
      base::UTF8ToUTF16(dialog->shipping_address_data_->shippingPhonenumber);

  if ((dialog->shipping_address_data_->shippingName).size() == 0) {
    elm_entry_cursor_end_set(dialog->shipping_name_entry_);
    elm_object_focus_set(dialog->shipping_name_entry_, EINA_TRUE);
  } else if ((dialog->shipping_address_data_->shippingStreetAddress).size() ==
             0) {
    elm_entry_cursor_end_set(dialog->shipping_street_entry_);
    elm_object_focus_set(dialog->shipping_street_entry_, EINA_TRUE);
  } else if ((dialog->shipping_address_data_->shippingPostalTown).size() == 0) {
    elm_entry_cursor_end_set(dialog->shipping_postaltown_entry_);
    elm_object_focus_set(dialog->shipping_postaltown_entry_, EINA_TRUE);
  } else if ((dialog->shipping_address_data_->shippingPostcode).size() == 0) {
    elm_entry_cursor_end_set(dialog->shipping_postcode_entry_);
    elm_object_focus_set(dialog->shipping_postcode_entry_, EINA_TRUE);
  } else if ((dialog->shipping_address_data_->shippingPhonenumber).size() ==
             0) {
    elm_entry_cursor_end_set(dialog->shipping_phone_entry_);
    elm_object_focus_set(dialog->shipping_phone_entry_, EINA_TRUE);
  } else {
    dialog->shipping_info_profile_.reset(new autofill::AutofillProfile());
    dialog->shipping_info_profile_->SetRawInfo((autofill::NAME_FIRST), name);
    dialog->shipping_info_profile_->SetRawInfo((autofill::COMPANY_NAME),
                                               organisation);
    dialog->shipping_info_profile_->SetRawInfo((autofill::ADDRESS_HOME_LINE1),
                                               streetAddress);
    dialog->shipping_info_profile_->SetRawInfo((autofill::ADDRESS_HOME_CITY),
                                               postalTown);
    dialog->shipping_info_profile_->SetRawInfo((autofill::ADDRESS_HOME_STATE),
                                               county);
    dialog->shipping_info_profile_->SetRawInfo((autofill::ADDRESS_HOME_ZIP),
                                               postcode);
    dialog->shipping_info_profile_->SetRawInfo((autofill::ADDRESS_HOME_COUNTRY),
                                               country);
    dialog->shipping_info_profile_->SetRawInfo(
        (autofill::PHONE_HOME_WHOLE_NUMBER), phoneNumber);
#if defined(TIZEN_AUTOFILL_SUPPORT)
    dialog->personal_data_manager_->AddProfile(
        *(dialog->shipping_info_profile_));
    dialog->db_sync_timer_ =
        ecore_timer_add(DB_SYNC_INTERVAL, ShippingDbSyncCb, data);
#endif
    dialog->payment_data_->SetSelectedShippingAddress(
        dialog->shipping_info_profile_.get());
    if (dialog->shipping_address_input_layout_) {
      evas_object_del(dialog->shipping_address_input_layout_);
      dialog->shipping_address_input_layout_ = nullptr;
      evas_object_show(dialog->popup_);
    }
  }
}

void PaymentRequestDialogEfl::DismissedCb(void* data,
                                          Evas_Object* obj,
                                          void* event_info) {}

void PaymentRequestDialogEfl::CtxpopupItemCb(void* data,
                                             Evas_Object* obj,
                                             void* event_info) {
  CtxpopupItemData* data_set = static_cast<CtxpopupItemData*>(data);
  PaymentRequestDialogEfl* dialog = data_set->dialog;
  Evas_Object* layout = dialog->card_input_layout_;
  Evas_Object* btn = data_set->btn;
  Evas_Object* ctxpopup = data_set->ctxpopup;
  const char* text = data_set->data.c_str();
  elm_object_domain_translatable_part_text_set(btn, NULL, "WebKit", text);

  if (data_set->data == "+Add New Address") {
    dialog->ShippingAddressInputLayout();
    dialog->AddDropDownButton("ADDR", "billaddressdropdown");
  }

  if (data_set->type == MONTH) {
    dialog->credit_card_.SetRawInfo(autofill::CREDIT_CARD_EXP_MONTH,
                                    base::ASCIIToUTF16(text));
    dialog->card_details_added_.card_month_added_ = true;
  } else if (data_set->type == YEAR) {
    dialog->credit_card_.SetRawInfo(autofill::CREDIT_CARD_EXP_4_DIGIT_YEAR,
                                    base::ASCIIToUTF16(text));
    dialog->card_details_added_.card_year_added_ = true;
  } else if (data_set->type == BILLING_ADDR) {
    dialog->credit_card_.set_billing_address_id(data_set->guid);
    dialog->card_details_added_.card_billaddr_added_ = true;
  }
  if (dialog->card_details_added_.card_month_added_ &&
      dialog->card_details_added_.card_year_added_) {
    int input_month = stoi(base::UTF16ToUTF8(
        dialog->credit_card_.GetRawInfo(autofill::CREDIT_CARD_EXP_MONTH)));
    int input_year = stoi(base::UTF16ToUTF8(dialog->credit_card_.GetRawInfo(
        autofill::CREDIT_CARD_EXP_4_DIGIT_YEAR)));
    base::Time::Exploded exploded_time;
    base::Time::Now().UTCExplode(&exploded_time);

    if (input_year == exploded_time.year && input_month < exploded_time.month)
      elm_object_signal_emit(layout, "elm,action,show,expDateError", "");
    else
      elm_object_signal_emit(layout, "elm,action,hide,expDateError", "");
  }

  evas_object_del(ctxpopup);
}

Evas_Object* PaymentRequestDialogEfl::AddDropDownButton(
    const std::string& text,
    const std::string& part) {
  Evas_Object* btn = elm_button_add(card_input_layout_);
  elm_object_style_set(btn, "dropdown");
  elm_object_domain_translatable_part_text_set(
      btn, NULL, "WebKit", text == "ADDR" ? "Select" : text.c_str());
  elm_object_part_content_set(card_input_layout_, part.c_str(), btn);

  if (text == "MM")
    evas_object_smart_callback_add(btn, "clicked", ShowCtxPopupMonthCb, this);
  else if (text == "YY")
    evas_object_smart_callback_add(btn, "clicked", ShowCtxPopupYearCb, this);
  else if (text == "ADDR")
    evas_object_smart_callback_add(btn, "clicked", ShowCtxPopupBillCb, this);

  return btn;
}

void PaymentRequestDialogEfl::ShowCtxPopupMonthCb(void* data,
                                                  Evas_Object* obj,
                                                  void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  Evas_Object* layout = dialog->card_input_layout_;
  Evas_Object* ctxpopup = elm_ctxpopup_add(dialog->web_view_->GetElmWindow());

  int width;
  evas_object_geometry_get(obj, 0, 0, &width, 0);
  evas_object_size_hint_min_set(ctxpopup, width, 0);

  elm_object_style_set(ctxpopup, "default");

#if defined(OS_TIZEN)
  eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK,
                                 eext_ctxpopup_back_cb, NULL);
#endif

  // Add menu items
  for (int month_counter = 1; month_counter <= 12; month_counter++) {
    std::string data = std::to_string(month_counter);
    data = (data.length() == 1) ? ("0" + data) : data;
    dialog->AppendItemToContextPopup(dialog, obj, ctxpopup, data, MONTH);
  }

  // Change position of the popup
  Evas_Coord x, y, w, h;
  evas_object_geometry_get(obj, &x, &y, &w, &h);
  evas_object_move(ctxpopup, x + (w / 2), y + (h / 2));
  evas_object_show(ctxpopup);
}

void PaymentRequestDialogEfl::ShowCtxPopupYearCb(void* data,
                                                 Evas_Object* obj,
                                                 void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  Evas_Object* layout = dialog->card_input_layout_;
  Evas_Object* ctxpopup = elm_ctxpopup_add(dialog->web_view_->GetElmWindow());

  elm_object_style_set(ctxpopup, "default");
#if defined(OS_TIZEN)
  eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK,
                                 eext_ctxpopup_back_cb, NULL);
#endif

  base::Time::Exploded exploded_time;
  base::Time::Now().UTCExplode(&exploded_time);
  int start_year = exploded_time.year;
  int endYear = start_year + 13;

  // Add menu items
  for (int year_counter = start_year; year_counter <= endYear; year_counter++) {
    std::string data = std::to_string(year_counter);
    dialog->AppendItemToContextPopup(dialog, obj, ctxpopup, data, YEAR);
  }

  // Change position of the popup
  Evas_Coord x, y, w, h;
  evas_object_geometry_get(obj, &x, &y, &w, &h);
  evas_object_move(ctxpopup, x + (w / 2), y + (h / 2));
  evas_object_show(ctxpopup);
}

void PaymentRequestDialogEfl::ShowCtxPopupBillCb(void* data,
                                                 Evas_Object* obj,
                                                 void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  Evas_Object* layout = dialog->card_input_layout_;
  Evas_Object* ctxpopup = elm_ctxpopup_add(dialog->web_view_->GetElmWindow());

  elm_object_style_set(ctxpopup, "default");
#if defined(OS_TIZEN)
  eext_object_event_callback_add(ctxpopup, EEXT_CALLBACK_BACK,
                                 eext_ctxpopup_back_cb, NULL);
#endif

#if defined(TIZEN_AUTOFILL_SUPPORT)
  dialog->web_shipping_data_profile_list_.clear();
  dialog->shipping_data_profile_list_ =
      dialog->GetPersonalDataManager()->GetProfiles();
  for (const auto& profile : dialog->shipping_data_profile_list_) {
    std::string country = base::UTF16ToUTF8(
        profile->GetRawInfo((autofill::ADDRESS_HOME_COUNTRY)));
    std::string name =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::NAME_FIRST)));
    std::string organisation =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::COMPANY_NAME)));
    std::string street_address =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::ADDRESS_HOME_LINE1)));
    std::string county =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::ADDRESS_HOME_STATE)));
    std::string postcode =
        base::UTF16ToUTF8(profile->GetRawInfo((autofill::ADDRESS_HOME_ZIP)));
    std::string phone = base::UTF16ToUTF8(
        profile->GetRawInfo((autofill::PHONE_HOME_WHOLE_NUMBER)));
    if (!country.empty() && !name.empty() && !street_address.empty() &&
        !postcode.empty() && !phone.empty()) {
      dialog->web_shipping_data_profile_list_.push_back(profile);
    }
  }
#endif

  // Adding shipping data to billing address dropdown button.
  if (dialog->web_shipping_data_profile_list_.size()) {
    for (const auto& profile : dialog->web_shipping_data_profile_list_) {
      std::string shipping_address_data = GetShippingAddressRawInfo(profile);
      dialog->AppendItemToContextPopup(dialog, obj, ctxpopup,
                                       shipping_address_data, BILLING_ADDR,
                                       profile->guid());
    }
  }
  dialog->AppendItemToContextPopup(dialog, obj, ctxpopup, "+Add New Address");

  // Change position of the popup
  Evas_Coord x, y, w, h;
  evas_object_geometry_get(obj, &x, &y, &w, &h);
  evas_object_move(ctxpopup, x + (w / 2), y + (h / 2));
  evas_object_show(ctxpopup);
}

Evas_Object* PaymentRequestDialogEfl::AddButton(Evas_Object* layout,
                                                const std::string& text,
                                                const std::string& part,
                                                Evas_Smart_Cb callback) {
  Evas_Object* btn = elm_button_add(layout);
  elm_object_style_set(btn, "default");
  elm_object_domain_translatable_part_text_set(btn, NULL, "WebKit",
                                               text.c_str());
  elm_object_part_content_set(layout, part.c_str(), btn);
  evas_object_smart_callback_add(btn, "clicked", callback, this);
  return btn;
}

Evas_Object* PaymentRequestDialogEfl::AddCustomButton(Evas_Object* layout,
                                                      const std::string& text,
                                                      const std::string& part,
                                                      Evas_Smart_Cb callback) {
  Evas_Object* btn = elm_button_add(layout);
  elm_object_style_set(btn, "custom_button");
  evas_object_smart_callback_add(btn, "clicked", callback, this);
  elm_object_part_text_set(btn, "button_text", text.c_str());
  elm_object_part_content_set(layout, part.c_str(), btn);
  return btn;
}

void PaymentRequestDialogEfl::CancelCreditCardButtonCb(void* data,
                                                       Evas_Object* obj,
                                                       void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->ClearCtxpopupDataList();
  if (dialog->card_input_layout_) {
    evas_object_del(dialog->card_input_layout_);
    dialog->card_input_layout_ = nullptr;
    evas_object_show(dialog->popup_);
  }
}

void PaymentRequestDialogEfl::CancelButtonCb(void* data,
                                             Evas_Object* obj,
                                             void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->ClearCtxpopupDataList();
  dialog->ClearAllDataLists();
  dialog->Close();
  dialog->request_->UserCancelled();
}

void PaymentRequestDialogEfl::CreditCardDoneButtonCb(void* data,
                                                     Evas_Object* obj,
                                                     void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  base::string16 local_error_message;
  std::string numStr =
      std::string(elm_entry_entry_get(dialog->card_number_entry_));
  numStr.erase(std::remove(numStr.begin(), numStr.end(), '-'), numStr.end());
  const base::string16& card_number = base::ASCIIToUTF16(numStr.c_str());
  if (!autofill::IsValidCreditCardNumberForBasicCardNetworks(
          card_number, dialog->request_->spec()->supported_card_networks_set(),
          &local_error_message)) {
    elm_object_signal_emit(dialog->card_input_layout_, "elm,action,show,error",
                           "");
    return;
  }
  if (dialog->card_details_added_.IsAllValuesAdded()) {
#if defined(TIZEN_AUTOFILL_SUPPORT)
    dialog->personal_data_manager_->AddCreditCard(dialog->credit_card_);
    dialog->db_sync_timer_ =
        ecore_timer_add(DB_SYNC_INTERVAL, CreditCardDbSyncCb, data);
#endif
    dialog->payment_data_->SetSelectedCreditCard(&dialog->credit_card_);
  }

  dialog->ClearCtxpopupDataList();
  if (dialog->card_input_layout_) {
    evas_object_del(dialog->card_input_layout_);
    dialog->card_input_layout_ = nullptr;
    evas_object_show(dialog->popup_);
  }
}

void PaymentRequestDialogEfl::DetailsButtonCb(void* data,
                                              Evas_Object* obj,
                                              void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);

  if (dialog->main_layout_expanded_) {
    Evas_Object* shrink_layout =
        dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);
    if (shrink_layout)
      elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    dialog->ShrinkLayoutForLayoutId(dialog->curr_expanded_layout_id_);
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
    elm_object_signal_emit(dialog->container_layout_,
                           "elm,action,shrink,mainLayout", "");
    dialog->main_layout_expanded_ = false;
    elm_object_part_text_set(obj, "button_text", "View details");
  } else {
    elm_object_signal_emit(
        dialog->GetLayoutObjForLayoutId(LAYOUT_ORDER_SUMMARY),
        "elm,action,expand,layout", "");
    dialog->ExpandLayoutForLayoutId(LAYOUT_ORDER_SUMMARY);
    dialog->curr_expanded_layout_id_ = LAYOUT_ORDER_SUMMARY;
    elm_object_signal_emit(dialog->container_layout_,
                           "elm,action,expand,mainLayout", "");
    dialog->main_layout_expanded_ = true;
    elm_object_part_text_set(obj, "button_text", "Hide details");
  }
}

void PaymentRequestDialogEfl::AddCardCb(void* data,
                                        Evas_Object* obj,
                                        void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->AddCardInputLayout();
}

void PaymentRequestDialogEfl::ExpandLayoutForLayoutId(LayoutId id) {
  switch (id) {
    case LAYOUT_ORDER_SUMMARY:
      elm_object_signal_emit(main_layout_, "elm,action,expand,orderSummary",
                             "");
      return;
    case LAYOUT_SHIPPING_ADDR: {
      std::string str("elm,action,expand,shippingAddress");
      elm_object_signal_emit(main_layout_, str.c_str(), "");
      int size = shipping_address_data_list_.size();
      if (size <= 2)
        str += std::to_string(size);
      else if (size >= 3)
        str += std::to_string(3);

      elm_object_signal_emit(main_layout_, str.c_str(), "");
      return;
    }
    case LAYOUT_SHIPPING_OPTION:
      elm_object_signal_emit(main_layout_, "elm,action,expand,shippingOption",
                             "");
      return;
    case LAYOUT_PAYMENT_METHOD: {
      std::string str("elm,action,expand,paymentMethod");
      elm_object_signal_emit(main_layout_, str.c_str(), "");
      int size = credit_card_number_data_list_.size();
      if (size <= 2)
        str += std::to_string(size);
      else if (size >= 3)
        str += std::to_string(3);

      elm_object_signal_emit(main_layout_, str.c_str(), "");
      return;
    }
    case LAYOUT_PHONE_NUMBER: {
      std::string str("elm,action,expand,phoneNumber");
      elm_object_signal_emit(main_layout_, str.c_str(), "");
      int size = phone_number_data_list_.size();
      if (size <= 2)
        str += std::to_string(size);
      else if (size >= 3)
        str += std::to_string(3);

      elm_object_signal_emit(main_layout_, str.c_str(), "");
      return;
    }
    case LAYOUT_EMAIL_ADDR: {
      std::string str("elm,action,expand,emailAddress");
      elm_object_signal_emit(main_layout_, str.c_str(), "");
      int size = email_address_data_list_.size();
      if (size <= 2)
        str += std::to_string(size);
      else if (size >= 3)
        str += std::to_string(3);

      elm_object_signal_emit(main_layout_, str.c_str(), "");
      return;
    }
    case LAYOUT_NONE:
    default:
      return;
  }
}

void PaymentRequestDialogEfl::ShrinkLayoutForLayoutId(LayoutId id) {
  switch (id) {
    case LAYOUT_ORDER_SUMMARY:
      elm_object_signal_emit(main_layout_, "elm,action,shrink,orderSummary",
                             "");
      return;
    case LAYOUT_SHIPPING_ADDR:
      elm_object_signal_emit(main_layout_, "elm,action,shrink,shippingAddress",
                             "");
      return;
    case LAYOUT_SHIPPING_OPTION:
      elm_object_signal_emit(main_layout_, "elm,action,shrink,shippingOption",
                             "");
      return;
    case LAYOUT_PAYMENT_METHOD:
      elm_object_signal_emit(main_layout_, "elm,action,shrink,paymentMethod",
                             "");
      return;
    case LAYOUT_PHONE_NUMBER:
      elm_object_signal_emit(main_layout_, "elm,action,shrink,phoneNumber", "");
      return;
    case LAYOUT_EMAIL_ADDR:
      elm_object_signal_emit(main_layout_, "elm,action,shrink,emailAddress",
                             "");
      return;
    case LAYOUT_NONE:
    default:
      return;
  }
}

Evas_Object* PaymentRequestDialogEfl::GetLayoutObjForLayoutId(LayoutId id) {
  switch (id) {
    case LAYOUT_ORDER_SUMMARY:
      return order_summary_layout_;
    case LAYOUT_SHIPPING_ADDR:
      return shipping_address_layout_;
    case LAYOUT_SHIPPING_OPTION:
      return shipping_option_layout_;
    case LAYOUT_PAYMENT_METHOD:
      return payment_method_layout_;
    case LAYOUT_PHONE_NUMBER:
      return contact_phone_number_layout_;
    case LAYOUT_EMAIL_ADDR:
      return contact_email_address_layout_;
    case LAYOUT_NONE:
    default:
      return NULL;
  }
}
void PaymentRequestDialogEfl::LayoutExpandButtonClickedCb(void* data,
                                                          Evas_Object* obj,
                                                          void* event_info) {
  LayoutIdData* layout_data = static_cast<LayoutIdData*>(data);
  PaymentRequestDialogEfl* dialog = layout_data->dialog;
  LayoutId new_layout_expand_id = layout_data->id;

  Evas_Object* expand_layout =
      dialog->GetLayoutObjForLayoutId(new_layout_expand_id);
  Evas_Object* shrink_layout =
      dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);

  Evas_Object* payment_bg_layout = (Evas_Object*)edje_object_part_object_get(
      elm_layout_edje_get(dialog->main_layout_), "payment_bg");

  // Behavior: When User clicks on a down button
  // 1. Shrink Curr expanded Layout
  if (shrink_layout) {
    elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    dialog->ShrinkLayoutForLayoutId(dialog->curr_expanded_layout_id_);
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
  }
  // 2. Expand New layout
  if (expand_layout) {
    elm_object_signal_emit(expand_layout, "elm,action,expand,layout", "");
    dialog->ExpandLayoutForLayoutId(new_layout_expand_id);
    dialog->curr_expanded_layout_id_ = new_layout_expand_id;
  }

  if (!dialog->main_layout_expanded_) {
    elm_object_signal_emit(dialog->container_layout_,
                           "elm,action,expand,mainLayout", "");
    dialog->main_layout_expanded_ = true;
    elm_object_part_text_set(dialog->details_button_, "button_text",
                             "Hide details");
  }
}

void PaymentRequestDialogEfl::LayoutShrinkButtonClickedCb(void* data,
                                                          Evas_Object* obj,
                                                          void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  Evas_Object* shrink_layout =
      dialog->GetLayoutObjForLayoutId(dialog->curr_expanded_layout_id_);

  // Behavior: When User clicks on a up button
  // Shrink Curr expanded Layout
  if (shrink_layout) {
    elm_object_signal_emit(shrink_layout, "elm,action,shrink,layout", "");
    dialog->ShrinkLayoutForLayoutId(dialog->curr_expanded_layout_id_);
    dialog->curr_expanded_layout_id_ = LAYOUT_NONE;
  }
}

void PaymentRequestDialogEfl::AddContactInfoCb(void* data,
                                               Evas_Object* obj,
                                               void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  if (dialog->request_->spec()->request_payer_phone())
    dialog->ContactPhoneNumberInputLayout();
  if (dialog->request_->spec()->request_payer_email())
    dialog->ContactEmailAddressInputLayout();
}

void PaymentRequestDialogEfl::AddShippingAddressInfoCb(void* data,
                                                       Evas_Object* obj,
                                                       void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->ShippingAddressInputLayout();
}

void PaymentRequestDialogEfl::PayButtonCb(void* data,
                                          Evas_Object* obj,
                                          void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->request_->Pay();
}

void PaymentRequestDialogEfl::OkButtonCb(void* data,
                                         Evas_Object* obj,
                                         void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  dialog->Close();
}

void PaymentRequestDialogEfl::CvcEntryChangedCb(void* data,
                                                Evas_Object* obj,
                                                void* event_info) {
  Evas_Object* cvc_confirm_btn = static_cast<Evas_Object*>(data);
  const char* cvc_text = elm_entry_entry_get(obj);
  // Check if input cvc number has 3 digits, only then enable confirm button.
  if (strlen(cvc_text) == 3)
    elm_object_disabled_set(cvc_confirm_btn, EINA_FALSE);
  else
    elm_object_disabled_set(cvc_confirm_btn, EINA_TRUE);
}

void PaymentRequestDialogEfl::CancelCvcButtonCb(void* data,
                                                Evas_Object* obj,
                                                void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  if (dialog->cvc_layout_) {
    evas_object_del(dialog->cvc_layout_);
    dialog->cvc_layout_ = nullptr;
    evas_object_show(dialog->popup_);
  }
}

void PaymentRequestDialogEfl::ConfirmCvcButtonCb(void* data,
                                                 Evas_Object* obj,
                                                 void* event_info) {
  PaymentRequestDialogEfl* dialog = static_cast<PaymentRequestDialogEfl*>(data);
  base::string16 cvc_text =
      base::UTF8ToUTF16(elm_entry_entry_get(dialog->cvc_entry_));
  dialog->cvc_controller_->CvcConfirmed(cvc_text);
  dialog->ShowPaymentProcessingPopup();
}

void PaymentRequestDialogEfl::ShowPaymentProcessingPopup() {
  evas_object_hide(cvc_layout_);
  evas_object_hide(popup_);
  evas_object_hide(main_layout_);
  evas_object_hide(container_layout_);
  edje_object_signal_emit(main_layout_, "hide,main,layout", "");

  progress_layout_ = elm_layout_add(main_layout_);
  // Set the layout file.
  elm_layout_file_set(progress_layout_,
                      payment_edj_path_.AsUTF8Unsafe().c_str(),
                      "payment_progress_layout");

  evas_object_size_hint_weight_set(progress_layout_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(progress_layout_, EVAS_HINT_FILL,
                                  EVAS_HINT_FILL);
  evas_object_show(progress_layout_);
  elm_object_part_content_set(progress_layout_, "header_swallow",
                              header_layout_);

  Evas_Object* progress = elm_progressbar_add(progress_layout_);
  elm_object_part_content_set(progress_layout_, "progress_swallow", progress);
  elm_object_style_set(progress, "process_medium");
  elm_progressbar_unit_format_set(progress, "");
  elm_progressbar_horizontal_set(progress, EINA_TRUE);
  elm_progressbar_pulse_set(progress, EINA_TRUE);
  elm_progressbar_pulse(progress, EINA_TRUE);
  evas_object_show(progress);
}

void PaymentRequestDialogEfl::PaymentProcessingResult(bool success) {
  if (!progress_layout_)
    return;

  Evas_Object* progress =
      elm_object_part_content_get(progress_layout_, "progress_swallow");
  evas_object_hide(progress);
  elm_object_part_content_unset(progress_layout_, "progress_swallow");

  Evas_Object* icon = elm_icon_add(progress_layout_);
  if (success) {
    elm_image_file_set(icon, payment_edj_path_.AsUTF8Unsafe().c_str(),
                       "success.png");
    elm_object_signal_emit(progress_layout_, "elm,action,payment,completed",
                           "");
  } else {
    elm_image_file_set(icon, payment_edj_path_.AsUTF8Unsafe().c_str(),
                       "fail.png");
    elm_object_signal_emit(progress_layout_, "elm,action,payment,failed", "");
  }

  elm_object_part_content_set(progress_layout_, "progress_swallow", icon);
}

void PaymentRequestDialogEfl::Close() {
  if (popup_) {
    evas_object_del(popup_);
    popup_ = nullptr;
  }
  if (conformant_) {
    evas_object_del(conformant_);
    conformant_ = nullptr;
  }
}

void PaymentRequestDialogEfl::CloseDialog() {
  Close();
}

void PaymentRequestDialogEfl::ShowErrorMessage() {
  PaymentProcessingResult(false);
}

}  // namespace payments
