// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "utc_blink_ewk_base.h"

class utc_blink_ewk_context_form_candidate_data_delete_all: public utc_blink_ewk_base
{
};

/**
* @brief Checking whether function doesn't explode.
* In order to check if autocomplete candidates were removed one should perform manual test.
*/
TEST_F(utc_blink_ewk_context_form_candidate_data_delete_all, POS_TEST)
{
    ewk_context_form_candidate_data_delete_all(ewk_view_context_get(GetEwkWebView()));
}

/**
 * @brief Checking whether function works properly in case of NULL of a context.
 */
TEST_F(utc_blink_ewk_context_form_candidate_data_delete_all, NEG_TEST)
{
    ewk_context_form_candidate_data_delete_all(NULL);
}
