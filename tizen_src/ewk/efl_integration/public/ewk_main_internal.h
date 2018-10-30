/*
 * Copyright (C) 2016 Samsung Electronics. All rights reserved.
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

/**
 * @file    ewk_main_internal.h
 * @brief   The general initialization of Chromium-efl,
 *          not tied to any view object.
 */

#ifndef ewk_main_internal_h
#define ewk_main_internal_h

#include "ewk_main.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set argument count and argument vector.
 *
 * Must be called before the following APIs:\n
 * ewk_view_add\n
 * ewk_view_add_in_incognito_mode\n
 * ewk_view_add_with_context\n
 * ewk_view_add_with_session_data\n
 * ewk_context_default_get\n
 * ewk_context_new\n
 * ewk_context_new_with_injected_bundle_path
 *
 * This API allows passing application arguments to the engine.
 * Also there is possible to add custom parameters. However,
 * the engine expects *original* application arguments
 * to remain unchanged. Passing copy of them using strdup and friends
 * is prohibited. The recommended way to call this API is:
 *
 * int main(int argc, char* argv[]) {
 *   ewk_set_arguments(argc, argv);
 *   ...
 * }
 *
 * or
 *
 * int main(int argc, char* argv[]) {
 *     char* arg_options[] = {
 *       argv[0],
 *       "my_custom_param_1",
 *       "my_custom_param_2",
 *     };
 *     int arg_cnt = sizeof(arg_options) / sizeof(arg_options[0]);
 *     ewk_set_arguments(arg_cnt, arg_options);
 * }
 *
 * @note Calling the function for the second time has no user-visible effect.
 *
 * @param argc argument count
 * @param argv argument array
 */
EXPORT_API void ewk_set_arguments(int argc, char** argv);

/**
* Deprecated.
* Set home directory.
*
* If new path is NULL or empty string, home directory is considered as not set.
*
*/
EINA_DEPRECATED EXPORT_API void ewk_home_directory_set(const char* path);

#ifdef __cplusplus
}
#endif
#endif // ewk_main_internal_h
