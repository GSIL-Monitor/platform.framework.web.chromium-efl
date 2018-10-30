// Copyright 2017 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PAYMENT_REQUEST_DIALOG_EFL_H_
#define PAYMENT_REQUEST_DIALOG_EFL_H_

#include <Elementary.h>
#include <Evas.h>
#include <ecore-1/Ecore.h>
#include <string>

#include "base/guid.h"
#include "base/macros.h"
#include "browser/autofill/personal_data_manager_factory.h"
#include "components/autofill/core/browser/autofill_country.h"
#include "components/autofill/core/browser/autofill_experiments.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/personal_data_manager_observer.h"
#include "components/autofill/core/browser/webdata/autofill_table.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/autofill/core/common/autofill_clock.h"
#include "components/autofill/core/common/autofill_constants.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/autofill/core/common/form_data.h"
#include "components/payments/content/payment_request_dialog.h"

class EWebView;

namespace content {
class WebContents;
}

namespace payments {

class CvcUnmaskViewControllerEfl;
class PaymentRequest;
class PaymentRequestDialogEfl;
struct LayoutIdData;
class PaymentDataEfl;

enum ContextPopupItemType { TYPE_NONE = 0, MONTH, YEAR, BILLING_ADDR };

struct CtxpopupItemData {
  PaymentRequestDialogEfl* dialog;
  Evas_Object* btn;
  Evas_Object* ctxpopup;
  std::string data;
  ContextPopupItemType type;
  std::string guid;  // set only for type BILLING_ADDR
};

struct OrderSummaryItemData {
  PaymentRequestDialogEfl* dialog;
  std::string label;
  std::string currency;
  std::string value;
  bool is_shipping_type;
  int id;
};

struct AutofillGenlistItemData {
  PaymentRequestDialogEfl* dialog;
  Evas_Object* radio;
  std::string data;
  std::string guid;
  int id;
};

struct ShippingOptionItemData {
  PaymentRequestDialogEfl* dialog;
  Evas_Object* radio;
  std::string shipping_type;
  std::string currency;
  std::string value;
  std::string option_id;
  int id;
  bool selected;
};

enum CreditCardType {
  CARD_UNKNOWN = 0,    // sad
  CARD_VISA = 1,       // if (number[0] == '4')
  CARD_AMEX = 2,       // first_two_digits == 34 || first_two_digits == 37
  CARD_MASTERCARD = 3  // first_two_digits >= 51 && first_two_digits <= 55
};

enum LayoutId {
  LAYOUT_NONE = 0,
  LAYOUT_ORDER_SUMMARY = 1,
  LAYOUT_SHIPPING_ADDR = 2,
  LAYOUT_SHIPPING_OPTION = 3,
  LAYOUT_PAYMENT_METHOD = 4,
  LAYOUT_PHONE_NUMBER = 5,
  LAYOUT_EMAIL_ADDR = 6
};

enum ShippingAddressFieldType {
  COUNTRY = 1,
  NAME = 2,
  ORGANISATION = 3,
  STREETADDRESS = 4,
  POSTALTOWN = 5,
  COUNTY = 6,
  POSTCODE = 7,
  PHONENUMBER = 8,
  NONE = 9
};

struct ItemData {
  PaymentRequestDialogEfl* dialog;
  ShippingAddressFieldType type;
  Evas_Object* dropdown;
  Evas_Object* entry;
};

class PaymentRequestDialogEfl : public PaymentRequestDialog {
 public:
  PaymentRequestDialogEfl(PaymentRequest* request,
                          content::WebContents* web_contents);
  ~PaymentRequestDialogEfl() override;

  void ShowDialog() override;
  void CloseDialog() override;
  void ShowErrorMessage() override;
  void ShowCvcUnmaskPrompt(
      const autofill::CreditCard& credit_card,
      base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
          result_delegate,
      content::WebContents* web_contents);

  void InitializeOrResetData();

  // Deletes popup and conformant
  void Close();

  // Layout parts of main layout
  Evas_Object* AddHeaderLayout();
  Evas_Object* AddOrderSummaryLayout();
  Evas_Object* AddShippingAddressLayout();
  Evas_Object* AddShippingOptionLayout();
  Evas_Object* AddPaymentMethodLayout();
  Evas_Object* AddPhoneNumberLayout();
  Evas_Object* AddEmailAddressLayout();

  // Child layouts of main layouts
  void AddCardInputLayout();
  void ShippingAddressInputLayout();
  void ContactPhoneNumberInputLayout();
  void ContactEmailAddressInputLayout();
  void EnablePayButton();
  void ShowCvcInputLayout();
  void ShowPaymentProcessingPopup();

  void GetCardNumberFromSelectedItem(const std::string& number,
                                     std::string& phone_number_to_display);
  void AppendItemToContextPopup(PaymentRequestDialogEfl* dialog,
                                Evas_Object* btn,
                                Evas_Object* ctxpopup,
                                std::string data,
                                ContextPopupItemType type = TYPE_NONE,
                                std::string guid = std::string());
  void ShippingAppendItemToContextPopup(PaymentRequestDialogEfl* dialog,
                                        Evas_Object* btn,
                                        Evas_Object* ctxpopup,
                                        std::string data);
  void ClearCtxpopupDataList();
  void ClearAllDataLists();
  CreditCardType CheckCreditCardType(const char* number);
  CreditCardType CheckCreditCardTypeFromDisplayStr(const char* display_str);
  Evas_Object* GetLayoutObjForLayoutId(LayoutId id);
  void ShrinkLayoutForLayoutId(LayoutId id);
  void ExpandLayoutForLayoutId(LayoutId id);
  void AddExpandAndShrinkButtons(Evas_Object* layout,
                                 LayoutIdData* layout_data);
  static Eina_Bool UpdateOrderSummaryGenlist(void* data);
  void SetCountries();

  Evas_Object* AddButton(Evas_Object* layout,
                         const std::string& text,
                         const std::string& part,
                         Evas_Smart_Cb callback);
  Evas_Object* AddCustomButton(Evas_Object* layout,
                               const std::string& text,
                               const std::string& part,
                               Evas_Smart_Cb callback);
  void AddOverlayButton(const std::string& part, LayoutIdData* data);
  Evas_Object* AddDropDownButton(const std::string& text,
                                 const std::string& part);
  Evas_Object* AddCloseButton(Evas_Object* layout,
                              const std::string& part,
                              Evas_Smart_Cb callback);
  autofill::PersonalDataManager* GetPersonalDataManager() const {
    return personal_data_manager_;
  }
  void PaymentProcessingResult(bool success);

  // Callbacks
  static void CancelShippingAddressButtonCb(void* data,
                                            Evas_Object* obj,
                                            void* event_info);
  static void FinishedShippingAddressButtonCb(void* data,
                                              Evas_Object* obj,
                                              void* event_info);
  static void CancelButtonCb(void* data, Evas_Object* obj, void* event_info);
  static void CancelCreditCardButtonCb(void* data,
                                       Evas_Object* obj,
                                       void* event_info);
  static void DetailsButtonCb(void* data, Evas_Object* obj, void* event_info);
  static void AddCardCb(void* data, Evas_Object* obj, void* event_info);
  static void LayoutExpandButtonClickedCb(void* data,
                                          Evas_Object* obj,
                                          void* event_info);
  static void LayoutShrinkButtonClickedCb(void* data,
                                          Evas_Object* obj,
                                          void* event_info);
  static void OverlayButtonClickedCb(void* data,
                                     Evas_Object* obj,
                                     void* event_info);
  static void PayButtonCb(void* data, Evas_Object* obj, void* event_info);
  static void OkButtonCb(void* data, Evas_Object* obj, void* event_info);
  static void CreditCardDoneButtonCb(void* data,
                                     Evas_Object* obj,
                                     void* event_info);
  static void ShowCtxPopupMonthCb(void* data,
                                  Evas_Object* obj,
                                  void* event_info);
  static void ShowCtxPopupYearCb(void* data,
                                 Evas_Object* obj,
                                 void* event_info);
  static void ShowCtxPopupBillCb(void* data,
                                 Evas_Object* obj,
                                 void* event_info);
  static void ShowCtxPopupCountryRegionCb(void* data,
                                          Evas_Object* obj,
                                          void* event_info);
  static void ValidateCardNumberCb(void* data,
                                   Evas_Object* obj,
                                   void* event_info);
  static void ShippingNameCb(void* data, Evas_Object* obj, void* event_info);
  static void ShippingAddressFieldValueChangedCb(void* data,
                                                 Evas_Object* obj,
                                                 void* event_info);
  static void ValidateCardNameCb(void* data,
                                 Evas_Object* obj,
                                 void* event_info);
  static void AddCardNameCb(void* data, Evas_Object* obj, void* event_info);
  static void AddCardNumberCb(void* data, Evas_Object* obj, void* event_info);
  static void CtxpopupItemCb(void* data, Evas_Object* obj, void* event_info);
  static void ShippingCtxpopupItemCb(void* data,
                                     Evas_Object* obj,
                                     void* event_info);
  static char* ShippingAddressInputTextGetCb(void* data,
                                             Evas_Object* obj,
                                             const char* part);
  static Evas_Object* ShippingAddressInputContentGetCb(void* data,
                                                       Evas_Object* obj,
                                                       const char* part);
  static void ItemSelectedCb(void* data, Evas_Object* obj, void* event_info);
  static void DismissedCb(void* data, Evas_Object* obj, void* event_info);
  static void AddPhoneNumberCb(void* data, Evas_Object* obj, void* event_info);
  static void AddEmailAddressCb(void* data, Evas_Object* obj, void* event_info);
  static void AddContactInfoCb(void* data, Evas_Object* obj, void* event_info);
  static void AddShippingAddressInfoCb(void* data,
                                       Evas_Object* obj,
                                       void* event_info);
  static void CancelAddPhoneNumberButtonCb(void* data,
                                           Evas_Object* obj,
                                           void* event_info);
  static void FinishedAddPhoneNumberButtonCb(void* data,
                                             Evas_Object* obj,
                                             void* event_info);
  static void CancelAddEmailAddressButtonCb(void* data,
                                            Evas_Object* obj,
                                            void* event_info);
  static void FinishedAddEmailAddressButtonCb(void* data,
                                              Evas_Object* obj,
                                              void* event_info);

  static void CvcEntryChangedCb(void* data, Evas_Object* obj, void* event_info);
  static void CancelCvcButtonCb(void* data, Evas_Object* obj, void* event_info);
  static void ConfirmCvcButtonCb(void* data,
                                 Evas_Object* obj,
                                 void* event_info);

  static void CreditCardGenlistItemClickCb(void* data,
                                           Evas_Object* obj,
                                           void* event_info);
  static void PhoneGenlistItemClickCb(void* data,
                                      Evas_Object* obj,
                                      void* event_info);
  static void EmailGenlistItemClickCb(void* data,
                                      Evas_Object* obj,
                                      void* event_info);
  static Evas_Object* ContactsContentGetCb(void* data,
                                           Evas_Object* obj,
                                           const char* part);
  static Evas_Object* ShippingContentGetCb(void* data,
                                           Evas_Object* obj,
                                           const char* part);
  static Eina_Bool ContactsDbSyncCb(void* data);
  static Eina_Bool ShippingDbSyncCb(void* data);
  static Evas_Object* CreditCardContentGetCb(void* data,
                                             Evas_Object* obj,
                                             const char* part);
  static Eina_Bool CreditCardDbSyncCb(void* data);
  static char* OrderSummaryTextGetCb(void* data,
                                     Evas_Object* obj,
                                     const char* part);
  static void OrderSummaryGenlistItemClickCb(void* data,
                                             Evas_Object* obj,
                                             void* event_info);
  static char* ShippingOptionTextGetCb(void* data,
                                       Evas_Object* obj,
                                       const char* part);
  static void ShippingOptionGenlistItemClickCb(void* data,
                                               Evas_Object* obj,
                                               void* event_info);
  static void ShippingAddressItemClickCb(void* data,
                                         Evas_Object* obj,
                                         void* event_info);
  static Evas_Object* ShippingOptionContentGetCb(void* data,
                                                 Evas_Object* obj,
                                                 const char* part);

 private:
  PaymentRequest* request_;

  EWebView* web_view_;
  Evas_Object* conformant_;
  Evas_Object* popup_;

  friend class PaymentDataEfl;

  typedef struct CardDetailsAdded {
    CardDetailsAdded() { ResetValues(); }
    void ResetValues() {
      card_name_added_ = card_number_added_ = card_month_added_ =
          card_year_added_ = card_billaddr_added_ = false;
    }
    bool IsAllValuesAdded() {
      return card_name_added_ && card_number_added_ && card_month_added_ &&
             card_year_added_ && card_billaddr_added_;
    }
    bool card_name_added_;
    bool card_number_added_;
    bool card_month_added_;
    bool card_year_added_;
    bool card_billaddr_added_;
  };

  typedef struct ContactInfoDetails {
    std::string contact_phone_number_;
    std::string contact_email_address_;
    std::string contact_name_;
  };

  struct ShippingAddressData {
    std::string shippingCountry;
    std::string shippingName;
    std::string shippingOrganisation;
    std::string shippingStreetAddress;
    std::string shippingPostalTown;
    std::string shippingCounty;
    std::string shippingPostcode;
    std::string shippingPhonenumber;
  };

  CreditCardType card_type_;
  CardDetailsAdded card_details_added_;
  autofill::CreditCard credit_card_;
  std::unique_ptr<CvcUnmaskViewControllerEfl> cvc_controller_;
  std::unique_ptr<PaymentDataEfl> payment_data_;
  std::unique_ptr<ContactInfoDetails> contact_info_details_;
  std::unique_ptr<ShippingAddressData> shipping_address_data_;
  std::unique_ptr<autofill::AutofillProfile> contact_info_profile_;
  std::unique_ptr<autofill::AutofillProfile> shipping_info_profile_;

  using CountryVector = std::vector<std::unique_ptr<autofill::AutofillCountry>>;
  CountryVector countries_;

  // Data lists
  std::vector<AutofillGenlistItemData*> credit_card_number_data_list_;
  std::vector<AutofillGenlistItemData*> phone_number_data_list_;
  std::vector<AutofillGenlistItemData*> shipping_address_data_list_;
  std::vector<AutofillGenlistItemData*> email_address_data_list_;
  std::vector<ShippingOptionItemData*> shipping_option_data_list_;
  std::vector<OrderSummaryItemData*> order_summary_data_list_;
  std::vector<CtxpopupItemData*> ctxpopup_data_list_;
  std::vector<ItemData*> item_data_list_;
  std::vector<LayoutIdData*> layout_id_data_list_;

  // Autofill Profiles and Credit Card list
  std::vector<autofill::CreditCard*> credit_card_list_;
  std::vector<autofill::AutofillProfile*> web_contact_info_profiles_list_;
  std::vector<autofill::AutofillProfile*> contact_info_profiles_list_;
  std::vector<autofill::AutofillProfile*> web_shipping_data_profile_list_;
  std::vector<autofill::AutofillProfile*> shipping_data_profile_list_;

  // Layouts
  Evas_Object* main_layout_;
  Evas_Object* header_layout_;
  Evas_Object* container_layout_;
  Evas_Object* order_summary_layout_;
  Evas_Object* shipping_address_layout_;
  Evas_Object* shipping_address_input_layout_;
  Evas_Object* shipping_option_layout_;
  Evas_Object* payment_method_layout_;
  Evas_Object* contact_phone_number_input_layout_;
  Evas_Object* contact_email_address_input_layout_;
  Evas_Object* contact_phone_number_layout_;
  Evas_Object* contact_email_address_layout_;
  Evas_Object* card_input_layout_;
  Evas_Object* cvc_layout_;
  Evas_Object* progress_layout_;

  // Entries
  Evas_Object* card_name_entry_;
  Evas_Object* card_number_entry_;
  Evas_Object* contact_phone_number_entry_;
  Evas_Object* contact_email_address_entry_;
  Evas_Object* shipping_name_entry_;
  Evas_Object* shipping_street_entry_;
  Evas_Object* shipping_postaltown_entry_;
  Evas_Object* shipping_postcode_entry_;
  Evas_Object* shipping_phone_entry_;
  Evas_Object* cvc_entry_;

  // Genlists
  Evas_Object* credit_card_genlist_;
  Evas_Object* order_summary_genlist_;
  Evas_Object* contact_genlist_;
  Evas_Object* shipping_genlist_;
  Evas_Object* shipping_input_genlist_;

  // Radio group
  Evas_Object* credit_card_radio_group_;
  Evas_Object* contact_radio_group_;
  Evas_Object* shipping_radio_group_;
  Evas_Object* shipping_options_radio_group_;
  Evas_Object* order_summary_radio_group_;

  // Buttons
  Evas_Object* details_button_;
  Evas_Object* pay_button_;

  LayoutId curr_expanded_layout_id_;
  autofill::PersonalDataManager* personal_data_manager_;
  int card_number_length_;
  bool main_layout_expanded_;

  // edj paths
  base::FilePath payment_edj_path_;
  base::FilePath add_email_address_path_;
  base::FilePath add_phone_number_path_;
  base::FilePath cvc_edj_path_;

  // Ecore timers
  Ecore_Timer* db_sync_timer_;

  DISALLOW_COPY_AND_ASSIGN(PaymentRequestDialogEfl);
};

}  // namespace payments

#endif  // PAYMENT_REQUEST_DIALOG_EFL_H_
