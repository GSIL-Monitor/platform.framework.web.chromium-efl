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
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SAMSUNG ELECTRONICS. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ewk_console_message_internal.h"

#include "private/ewk_console_message_private.h"

Ewk_Console_Message_Level ewk_console_message_level_get(const Ewk_Console_Message *message)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(message, EWK_CONSOLE_MESSAGE_LEVEL_NULL);
  return static_cast<Ewk_Console_Message_Level>(message->level);
}

Eina_Stringshare* ewk_console_message_text_get(const Ewk_Console_Message *message)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(message, 0);
  return message->message;
}

unsigned ewk_console_message_line_get(const Ewk_Console_Message *message)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(message, 0);
  return message->line;
}

Eina_Stringshare* ewk_console_message_source_get(const Ewk_Console_Message *message)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(message, 0);
  return message->source;
}
