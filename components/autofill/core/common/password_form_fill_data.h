// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_COMMON_PASSWORD_FORM_FILL_DATA_H_
#define COMPONENTS_AUTOFILL_CORE_COMMON_PASSWORD_FORM_FILL_DATA_H_

#include <map>

#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/password_form.h"

namespace autofill {

// Helper struct for PasswordFormFillData
struct UsernamesCollectionKey {
  UsernamesCollectionKey();
  ~UsernamesCollectionKey();

  // Defined so that this struct can be used as a key in a std::map.
  bool operator<(const UsernamesCollectionKey& other) const;

  base::string16 username;
  base::string16 password;
  std::string realm;
};

struct PasswordAndRealm {
  base::string16 password;
  std::string realm;
};

// Structure used for autofilling password forms. Note that the realms in this
// struct are only set when the password's realm differs from the realm of the
// form that we are filling.
struct PasswordFormFillData {
  typedef std::map<base::string16, PasswordAndRealm> LoginCollection;
  typedef std::map<UsernamesCollectionKey,
                   std::vector<base::string16> > UsernamesCollection;

  // The name of the form.
  base::string16 name;

  // An origin URL consists of the scheme, host, port and path; the rest is
  // stripped.
  GURL origin;

  // The action target of the form; like |origin| URL consists of the scheme,
  // host, port and path; the rest is stripped.
  GURL action;

  // Username and password input fields in the form.
  FormFieldData username_field;
  FormFieldData password_field;

  // The signon realm of the preferred user/pass pair.
  std::string preferred_realm;

  // A list of other matching username->PasswordAndRealm pairs for the form.
  LoginCollection additional_logins;

  // A list of possible usernames in the case where we aren't completely sure
  // that the original saved username is correct. This data is keyed by the
  // saved username/password to ensure uniqueness, though the username is not
  // used.
  // TODO(crbug/188908). Remove |other_possible_usernames| or launch.
  UsernamesCollection other_possible_usernames;

  // Tells us whether we need to wait for the user to enter a valid username
  // before we autofill the password. By default, this is off unless the
  // PasswordManager determined there is an additional risk associated with this
  // form. This can happen, for example, if action URI's of the observed form
  // and our saved representation don't match up.
  bool wait_for_username;

  // Tells autofillagent to use the selected user for autofill and autologin
  // when we have
  // multiple fingerprint accounts register for the same form.
  base::string16 selected_user;

  // Tells the client if the usernames are readonly.
  bool username_element_readonly;

  // Tells us whether the form is Non-FingerPrint remembered. It is set to
  // true for those credentials which are remebered without fingerprint.
  bool manual_autofill;

  // Usernames list which requires autologin through Authentication
  std::vector<base::string16> username_list;

  // True if this form is a change password form.
  bool is_possible_change_password_form;

  // True if a "form not secure" warning should be shown when the form is
  // autofilled.
  bool show_form_not_secure_warning_on_autofill;

  // Tells us whether autologin is required for this password form. It will be
  // true for Password forms which user has already added to fingerprint
  // database.
  bool autologin_required;

  PasswordFormFillData();
  PasswordFormFillData(const PasswordFormFillData& other);
  ~PasswordFormFillData();
};

// Create a FillData structure in preparation for autofilling a form,
// from basic_data identifying which form to fill, and a collection of
// matching stored logins to use as username/password values.
// |preferred_match| should equal (address) one of matches.
// |wait_for_username_before_autofill| is true if we should not autofill
// anything until the user typed in a valid username and blurred the field.
// If |enable_possible_usernames| is true, we will populate possible_usernames
// in |result|.
void InitPasswordFormFillData(
    const PasswordForm& form_on_page,
    const std::map<base::string16, const PasswordForm*>& matches,
    const PasswordForm* const preferred_match,
    bool wait_for_username_before_autofill,
#if defined(S_TERRACE_SUPPORT)
    bool manual_autofill,
#endif
    bool enable_other_possible_usernames,
    PasswordFormFillData* result);

// Renderer needs to have only a password that should be autofilled, all other
// passwords might be safety erased.
PasswordFormFillData ClearPasswordValues(const PasswordFormFillData& data);

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_COMMON_PASSWORD_FORM_FILL_DATA_H__
