/*
 * Copyright (C) 2013-2016 Samsung Electronics. All rights reserved.
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

#include "ewk_dispatcher_internal.h"
#include <assert.h>

// // TODO: remove dependency to chromium internal
#include <base/bind.h>
#include <base/location.h>

#include "base/threading/thread_task_runner_handle.h"

using namespace base;

static scoped_refptr<base::SingleThreadTaskRunner> g_task_runner;

void ewk_dispatcher_touch()
{
  g_task_runner = base::ThreadTaskRunnerHandle::Get();
}

void ewk_dispatcher_dispatch(ewk_dispatch_callback cb, void *user_data, unsigned delay)
{
  if (delay) {
    g_task_runner->PostDelayedTask(FROM_HERE,
                                   Bind(cb, user_data),
                                   TimeDelta::FromMilliseconds(delay));
  } else {
    g_task_runner->PostTask(FROM_HERE,
                            Bind(cb, user_data));
  }
}


