// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>
#import <IOSurface/IOSurface.h>

#include <ifaddrs.h>
#include <servers/bootstrap.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/mac_util.h"
#include "base/process/kill.h"
#include "base/sys_info.h"
#include "base/test/multiprocess_test.h"
#include "base/test/test_timeouts.h"
#include "content/test/test_content_client.h"
#include "sandbox/mac/sandbox_compiler.h"
#include "sandbox/mac/seatbelt_exec.h"
#include "services/service_manager/sandbox/mac/renderer_v2.sb.h"
#include "services/service_manager/sandbox/mac/sandbox_mac.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace content {

namespace {

void SetParametersForTest(sandbox::SandboxCompiler* compiler,
                          const base::FilePath& logging_path,
                          const base::FilePath& executable_path) {
  bool enable_logging = true;
  CHECK(compiler->InsertBooleanParam(
      service_manager::Sandbox::kSandboxEnableLogging, enable_logging));
  CHECK(compiler->InsertBooleanParam(
      service_manager::Sandbox::kSandboxDisableDenialLogging, !enable_logging));

  std::string homedir =
      service_manager::Sandbox::GetCanonicalSandboxPath(base::GetHomeDir())
          .value();
  CHECK(compiler->InsertStringParam(
      service_manager::Sandbox::kSandboxHomedirAsLiteral, homedir));

  int32_t major_version, minor_version, bugfix_version;
  base::SysInfo::OperatingSystemVersionNumbers(&major_version, &minor_version,
                                               &bugfix_version);
  int32_t os_version = (major_version * 100) + minor_version;
  CHECK(compiler->InsertStringParam(service_manager::Sandbox::kSandboxOSVersion,
                                    std::to_string(os_version)));

  std::string bundle_path = service_manager::Sandbox::GetCanonicalSandboxPath(
                                base::mac::MainBundlePath())
                                .value();
  CHECK(compiler->InsertStringParam(
      service_manager::Sandbox::kSandboxBundlePath, bundle_path));

  CHECK(compiler->InsertStringParam(
      service_manager::Sandbox::kSandboxChromeBundleId,
      "com.google.Chrome.test.sandbox"));
  CHECK(compiler->InsertStringParam(
      service_manager::Sandbox::kSandboxBrowserPID, std::to_string(getpid())));

  CHECK(compiler->InsertStringParam(
      service_manager::Sandbox::kSandboxLoggingPathAsLiteral,
      logging_path.value()));

  // Parameters normally set by the main executable.
  CHECK(compiler->InsertStringParam("CURRENT_PID", std::to_string(getpid())));
  CHECK(
      compiler->InsertStringParam("EXECUTABLE_PATH", executable_path.value()));
}

}  // namespace

// These tests check that the V2 sandbox compiles, initializes, and
// correctly enforces resource access on all macOS versions. Note that
// with the exception of certain controlled locations, such as a dummy
// log file, these tests cannot check that write access to system files
// is blocked. These tests run on developers' machines and bots, so
// if the write access goes through, that machine could be corrupted.
class SandboxV2Test : public base::MultiProcessTest {};

MULTIPROCESS_TEST_MAIN(SandboxProfileProcess) {
  TestContentClient content_client;
  sandbox::SandboxCompiler compiler(
      service_manager::kSeatbeltPolicyString_renderer_v2);

  // Create the logging file and pass /bin/ls as the executable path.
  base::ScopedTempDir temp_dir;
  CHECK(temp_dir.CreateUniqueTempDir());
  CHECK(temp_dir.IsValid());
  base::FilePath temp_path = temp_dir.GetPath();
  temp_path = service_manager::Sandbox::GetCanonicalSandboxPath(temp_path);
  const base::FilePath log_file = temp_path.Append("log-file");
  const base::FilePath exec_file("/bin/ls");

  SetParametersForTest(&compiler, log_file, exec_file);

  std::string error;
  bool result = compiler.CompileAndApplyProfile(&error);
  CHECK(result) << error;

  // Test the properties of the sandbox profile.
  const char log_msg[] = "logged";
  CHECK_NE(-1, base::WriteFile(log_file, log_msg, sizeof(log_msg)));
  // Log file is write only.
  char read_buf[sizeof(log_msg)];
  CHECK_EQ(-1, base::ReadFile(log_file, read_buf, sizeof(read_buf)));

  // Try executing the blessed binary.
  CHECK_NE(-1, system(exec_file.value().c_str()));

  // Try and realpath a file.
  char resolved_name[4096];
  CHECK_NE(nullptr, realpath(log_file.value().c_str(), resolved_name));

  // Test shared memory access.
  int shm_fd = shm_open("apple.shm.notification_center", O_RDONLY, 0644);
  CHECK_GE(shm_fd, 0);

  // Test mach service access. The port is leaked because the multiprocess
  // test exits quickly after this look up.
  mach_port_t service_port;
  kern_return_t status = bootstrap_look_up(
      bootstrap_port, "com.apple.system.logger", &service_port);
  CHECK_EQ(status, BOOTSTRAP_SUCCESS) << bootstrap_strerror(status);

  mach_port_t forbidden_mach;
  status = bootstrap_look_up(bootstrap_port, "com.apple.cfprefsd.daemon",
                             &forbidden_mach);
  CHECK_NE(BOOTSTRAP_SUCCESS, status);

  // Read bundle contents.
  base::FilePath bundle_path = base::mac::MainBundlePath();
  struct stat st;
  CHECK_NE(-1, stat(bundle_path.value().c_str(), &st));

  // Test that general file system access isn't available.
  base::FilePath ascii_path("/usr/share/misc/ascii");
  std::string ascii_contents;
  CHECK(!base::ReadFileToStringWithMaxSize(ascii_path, &ascii_contents, 4096));

  base::FilePath system_certs(
      "/System/Library/Keychains/SystemRootCertificates.keychain");
  std::string keychain_contents;
  CHECK(!base::ReadFileToStringWithMaxSize(system_certs, &keychain_contents,
                                           4096));

  // Check that not all sysctls, including those that can get the MAC address,
  // are allowed. See crbug.com/738129. Only 10.10+ supports sysctl filtering.
  if (base::mac::IsAtLeastOS10_10()) {
    struct ifaddrs* ifap;
    CHECK_EQ(-1, getifaddrs(&ifap));
  }

  std::vector<uint8_t> sysctl_data(4096);
  size_t data_size = sysctl_data.size();
  CHECK_EQ(0,
           sysctlbyname("hw.ncpu", sysctl_data.data(), &data_size, nullptr, 0));

  return 0;
}

TEST_F(SandboxV2Test, SandboxProfileTest) {
  base::Process process = SpawnChild("SandboxProfileProcess");
  ASSERT_TRUE(process.IsValid());
  int exit_code = 42;
  EXPECT_TRUE(process.WaitForExitWithTimeout(TestTimeouts::action_max_timeout(),
                                             &exit_code));
  EXPECT_EQ(exit_code, 0);
}

}  // namespace content
