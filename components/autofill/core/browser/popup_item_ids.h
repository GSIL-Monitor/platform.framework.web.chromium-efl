// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This enum defines item identifiers for Autofill popup controller.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_POPUP_ITEM_ID_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_POPUP_ITEM_ID_H_

namespace autofill {

enum PopupItemId {
  POPUP_ITEM_ID_AUTOCOMPLETE_ENTRY = 0,
  POPUP_ITEM_ID_INSECURE_CONTEXT_PAYMENT_DISABLED_MESSAGE = -1,
  POPUP_ITEM_ID_PASSWORD_ENTRY = -2,
  POPUP_ITEM_ID_SEPARATOR = -3,
  POPUP_ITEM_ID_CLEAR_FORM = -4,
  POPUP_ITEM_ID_AUTOFILL_OPTIONS = -5,
  POPUP_ITEM_ID_DATALIST_ENTRY = -6,
  POPUP_ITEM_ID_SCAN_CREDIT_CARD = -7,
  POPUP_ITEM_ID_TITLE = -8,
  POPUP_ITEM_ID_CREDIT_CARD_SIGNIN_PROMO = -9,
  POPUP_ITEM_ID_HTTP_NOT_SECURE_WARNING_MESSAGE = -10,
  POPUP_ITEM_ID_USERNAME_ENTRY = -11,
#if defined(S_TERRACE_SUPPORT)
  // when this value was changed,
  // please check ITEM_ID_HAVE_CREDIT_CARD
  // in TinAutofillPopupBridge.java and AutofillPopup.java
  // must be sync with value of ITEM_ID_HAVE_CREDIT_CARD.
  // we are decided to set value to -100.
  // because to avoid overlap value with others when rebase.
  POPUP_ITEM_ID_HAS_CREDIT_CARD = -99,
  POPUP_ITEM_ID_POPUP_HELPER = -100,
#endif
  POPUP_ITEM_ID_CREATE_HINT = -12,
  POPUP_ITEM_ID_ALL_SAVED_PASSWORDS_ENTRY = -13,
  POPUP_ITEM_ID_GENERATE_PASSWORD_ENTRY = -14,
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_POPUP_ITEM_ID_H_
