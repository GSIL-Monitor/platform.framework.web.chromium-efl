/*
 * Copyright (C) 2014-2016 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SAMSUNG ELECTRONICS. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SAMSUNG ELECTRONICS. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ewk_history_internal.h"
#include "private/ewk_private.h"
#include "private/ewk_history_private.h"

int ewk_history_back_list_length_get(Ewk_History* history)
{
  // returning -1 here would be better to indicate the error
  // but we need to stick to the WebKit-EFL implementation
  EINA_SAFETY_ON_NULL_RETURN_VAL(history, 0);
  return history->GetBackListLength();
}

int ewk_history_forward_list_length_get(Ewk_History* history)
{
  // returning -1 here would be better to indicate the error
  // but we need to stick to the WebKit-EFL implementation
  EINA_SAFETY_ON_NULL_RETURN_VAL(history, 0);
  return history->GetForwardListLength();
}

Ewk_History_Item* ewk_history_nth_item_get(Ewk_History* history, int index)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(history, NULL);
  return static_cast<_Ewk_History_Item *>(history->GetItemAtIndex(index));
}

const char* ewk_history_item_uri_get(Ewk_History_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);
  // GetURL().spec() returns const reference
  return item->GetURL().spec().c_str();
}

const char* ewk_history_item_title_get(Ewk_History_Item* item)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(item, NULL);
  // GetTitle() return const reference
  return item->GetTitle().c_str();
}

void ewk_history_free(Ewk_History* history)
{
  EINA_SAFETY_ON_NULL_RETURN(history);
  delete history;
}
