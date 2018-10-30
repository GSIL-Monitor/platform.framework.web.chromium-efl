// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SANDBOX_LINUX_SANDBOX_SECCOMP_BPF_LINUX_H_
#define CONTENT_COMMON_SANDBOX_LINUX_SANDBOX_SECCOMP_BPF_LINUX_H_

#include <memory>
#include <string>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "sandbox/linux/bpf_dsl/policy.h"
#include "services/service_manager/sandbox/sandbox_type.h"

namespace content {

// This class has two main sets of APIs. One can be used to start the sandbox
// for internal content process types, the other is indirectly exposed as
// a public content/ API and uses a supplied policy.
class SandboxSeccompBPF {
 public:
  struct Options {
    bool use_amd_specific_policies = false;  // For ChromiumOs.
  };

  // This is the API to enable a seccomp-bpf sandbox for content/
  // process-types:
  // Is the sandbox globally enabled, can anything use it at all ?
  // This looks at global command line flags to see if the sandbox
  // should be enabled at all.
  static bool IsSeccompBPFDesired();
  // Check if the kernel supports seccomp-bpf.
  static bool SupportsSandbox();

#if !defined(OS_NACL_NONSFI)
  // Check if the kernel supports TSYNC (thread synchronization) with seccomp.
  static bool SupportsSandboxWithTsync();
  // Start the sandbox and apply the policy for sandbox_type, depending on
  // command line switches and options.
  static bool StartSandbox(service_manager::SandboxType sandbox_type,
                           base::ScopedFD proc_fd,
                           const Options& options);
#endif  // !defined(OS_NACL_NONSFI)

  // This is the API to enable a seccomp-bpf sandbox by using an
  // external policy.
  static bool StartSandboxWithExternalPolicy(
      std::unique_ptr<sandbox::bpf_dsl::Policy> policy,
      base::ScopedFD proc_fd);
  // The "baseline" policy can be a useful base to build a sandbox policy.
  static std::unique_ptr<sandbox::bpf_dsl::Policy> GetBaselinePolicy();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(SandboxSeccompBPF);
};

}  // namespace content

#endif  // CONTENT_COMMON_SANDBOX_LINUX_SANDBOX_SECCOMP_BPF_LINUX_H_