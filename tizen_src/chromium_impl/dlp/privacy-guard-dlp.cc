// Copyright (c) 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <dlfcn.h>
#include "privacy-guard-dlp.h"
#include "base/logging.h"
#include "base/posix/safe_strerror.h"

#define LIBRARY_PATH "/usr/lib/libprivacy-guard-client.so"

typedef void (*init_func)(void);
typedef void (*check_leak_func)(const char *, char * const, size_t);

static init_func my_privacy_guard_dlp_init = 0;
static check_leak_func my_privacy_guard_dlp_check_leak = 0;

/**
 * @fn void tizen_dlp_init(void)
 * @brief Initialize the DLP creating the Load Rules and Logging threads
 * @callgraph
 */
void privacy_guard_dlp_init(void)
{
    if (!my_privacy_guard_dlp_init && !my_privacy_guard_dlp_check_leak) {
        void *handle = dlopen(LIBRARY_PATH, RTLD_LAZY|RTLD_NODELETE);
        if (handle) {
            my_privacy_guard_dlp_init = (init_func) dlsym(handle, "privacy_guard_dlp_init");
            my_privacy_guard_dlp_check_leak = (check_leak_func) dlsym(handle, "privacy_guard_dlp_check_leak");
            dlclose(handle);
        } else {
            LOG(ERROR) << "handle: " << handle << ", errno: " << base::safe_strerror(errno);
        }
    }

    if (my_privacy_guard_dlp_init)
        my_privacy_guard_dlp_init();
}

/**
 * @fn void tizen_dlp_check_leak(const char *hostname, char * const mem, size_t len)
 * @brief Checks for information leak on a given request string
 *
 * @param[in] hostname          The hostname of the server to which the request will be sent
 * @param[in] mem           Text that we are going to validate for info leak
 * @param[in] len           Size of len in bytes
 *
 * @return  either PRIV_GUARD_DLP_RESULT_ALLOW or PRIV_GUARD_DLP_RESULT_DENY
 * @callgraph
 */
void privacy_guard_dlp_check_leak(const char *hostname, char * const mem, size_t len)
{
    /**
    * Send data to Tizen DLP verification
    */
    if(my_privacy_guard_dlp_check_leak)
        my_privacy_guard_dlp_check_leak(hostname, mem, len);
}

