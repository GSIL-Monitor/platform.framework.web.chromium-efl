// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_context_form_autofill_credit_card_private_h
#define ewk_context_form_autofill_credit_card_private_h

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include <Evas.h>
#include <tizen.h>

struct Ewk_Context;

typedef struct _Ewk_Autofill_CreditCard Ewk_Autofill_CreditCard;
typedef void (*Ewk_Context_Form_Autofill_CreditCard_Changed_Callback)(
    void* data);

class EwkContextFormAutofillCreditCardManager {
 public:
  /**
   * Gets a list of all existing credit_cards
   *
   * The obtained credit_card must be deleted by
   * ewk_autofill_credit_card_delete.
   * @param context context object
   *
   * @return @c Eina_List of Ewk_Autofill_CreditCard @c NULL otherwise
   * @see ewk_autofill_credit_card_delete
   */
  static Eina_List* priv_form_autofill_credit_card_get_all(
      Ewk_Context* context);

  /**
   * Gets the existing credit_card for given index
   *
   * The obtained credit_card must be deleted by
   * ewk_autofill_credit_card_delete.
   *
   * @param context context object
   * @param id credit_card id
   *
   * @return @c Ewk_Autofill_CreditCard if credit_card exists, @c NULL otherwise
   * @see ewk_autofill_credit_card_delete
   */
  static Ewk_Autofill_CreditCard* priv_form_autofill_credit_card_get(
      Ewk_Context* context,
      unsigned id);

  /**
   * Sets the given credit_card for the given id
   *
   * Data can be added to the created credit_card by
   * ewk_autofill_credit_card_data_set.
   *
   * @param context context object
   * @param credit_card id
   * @param credit_card Ewk_Autofill_CreditCard
   *
   * @return @c EINA_TRUE if the credit_card data is set successfully, @c
   * EINA_FALSE otherwise
   * @see ewk_autofill_credit_card_new
   * @see ewk_context_form_autofill_credit_card_add
   */
  static Eina_Bool priv_form_autofill_credit_card_set(
      Ewk_Context* context,
      unsigned id,
      Ewk_Autofill_CreditCard* credit_card);

  /**
   * Saves the created credit_card into permenant storage
   *
   * The credit_card used to save must be created by
   * ewk_autofill_credit_card_new.
   * Data can be added to the created credit_card by
   * ewk_autofill_credit_card_data_set.
   *
   * @param context context object
   * @param credit_card Ewk_Autofill_CreditCard
   *
   * @return @c EINA_TRUE if the credit_card data is saved successfully, @c
   * EINA_FALSE otherwise
   * @see ewk_autofill_credit_card_new
   */
  static Eina_Bool priv_form_autofill_credit_card_add(
      Ewk_Context* context,
      Ewk_Autofill_CreditCard* credit_card);

  /**
   * Removes Autofill Form credit_card completely
   *
   * @param context context object
   * @param id credit_card id
   *
   * @return @c EINA_TRUE if the credit_card data is removed successfully, @c
   * EINA_FALSE otherwise
   * @see ewk_context_form_autofill_credit_card_get_all
   */
  static Eina_Bool priv_form_autofill_credit_card_remove(Ewk_Context* context,
                                                         unsigned id);

  /**
   * Sets callback function on credit_cards changed
   *
   * @param callback pointer to callback function
   * @param user_data user data returned on callback
   *
   * @see ewk_context_form_autofill_credit_card_changed_callback_set
   */
  static void priv_form_autofill_credit_card_changed_callback_set(
      Ewk_Context_Form_Autofill_CreditCard_Changed_Callback callback,
      void* user_data);
};

#endif  // TIZEN_AUTOFILL_SUPPORT

#endif  // ewk_context_form_autofill_credit_card_private_h
