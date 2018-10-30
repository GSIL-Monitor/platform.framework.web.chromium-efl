/*
 * Copyright (C) 2017 Samsung Electronics.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "ewk_form_repost_decision_product.h"
#include "private/ewk_form_repost_decision_private.h"
#include "private/ewk_private.h"

void ewk_form_repost_decision_request_reply(Ewk_Form_Repost_Decision_Request* request, Eina_Bool allow)
{
  EINA_SAFETY_ON_NULL_RETURN(request);
  if (allow)
    request->OnAccepted();
  else
    request->OnCanceled();
}
