// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_context_form_autofill_profile_private_h
#define ewk_context_form_autofill_profile_private_h

#if defined(TIZEN_AUTOFILL_SUPPORT)

#include <Evas.h>
#include <tizen.h>

class Ewk_Context;

typedef struct _Ewk_Autofill_Profile Ewk_Autofill_Profile;
typedef void (*Ewk_Context_Form_Autofill_Profile_Changed_Callback)(void *data);

class EwkContextFormAutofillProfileManager
{
 public:
  /**
   * Gets a list of all existing profiles
   *
   * The obtained profile must be deleted by ewk_autofill_profile_delete.
   * @param context context object
   *
   * @return @c Eina_List of Ewk_Autofill_Profile @c NULL otherwise
   * @see ewk_autofill_profile_delete
   */
  static Eina_List* priv_form_autofill_profile_get_all(Ewk_Context* context);

  /**
   * Gets the existing profile for given index
   *
   * The obtained profile must be deleted by ewk_autofill_profile_delete.
   *
   * @param context context object
   * @param profile id
   *
   * @return @c Ewk_Autofill_Profile if profile exists, @c NULL otherwise
   * @see ewk_autofill_profile_delete
   */
  static Ewk_Autofill_Profile* priv_form_autofill_profile_get(Ewk_Context* context,
      unsigned id);

  /**
   * Sets the given profile for the given id
   *
   * Data can be added to the created profile by ewk_autofill_profile_data_set.
   *
   * @param context context object
   * @param profile id
   * @param profile Ewk_Autofill_Profile
   *
   * @return @c EINA_TRUE if the profile data is set successfully, @c EINA_FALSE otherwise
   * @see ewk_autofill_profile_new
   * @see ewk_context_form_autofill_profile_add
   */
  static Eina_Bool priv_form_autofill_profile_set(Ewk_Context* context,
      unsigned id, Ewk_Autofill_Profile* profile);

  /**
   * Saves the created profile into permenant storage
   *
   * The profile used to save must be created by ewk_autofill_profile_new.
   * Data can be added to the created profile by ewk_autofill_profile_data_set.
   *
   * @param context context object
   * @param profile Ewk_Autofill_Profile
   *
   * @return @c EINA_TRUE if the profile data is saved successfully, @c EINA_FALSE otherwise
   * @see ewk_autofill_profile_new
   */
  static Eina_Bool priv_form_autofill_profile_add(Ewk_Context* context,
      Ewk_Autofill_Profile* profile);

  /**
   * Removes Autofill Form profile completely
   *
   * @param context context object
   * @param index profile id
   *
   * @return @c EINA_TRUE if the profile data is removed successfully, @c EINA_FALSE otherwise
   * @see ewk_context_form_autofill_profile_get_all
   */
    static EXPORT_API Eina_Bool priv_form_autofill_profile_remove(Ewk_Context* context,
        unsigned id);

  /**
   * Sets callback function on profiles changed
   *
   * @param callback pointer to callback function
   * @param user_data user data returned on callback
   *
   * @see ewk_context_form_autofill_profile_changed_callback_set
   */
    static EXPORT_API void priv_form_autofill_profile_changed_callback_set(
        Ewk_Context_Form_Autofill_Profile_Changed_Callback callback,
        void* user_data);
};

#endif // TIZEN_AUTOFILL_SUPPORT

#endif // ewk_context_form_autofill_profile_private_h
