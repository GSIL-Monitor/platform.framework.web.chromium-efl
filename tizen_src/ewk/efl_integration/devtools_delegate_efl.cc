// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "devtools_delegate_efl.h"

#include <vector>

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "base/bind.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "common/version_info.h"
#include "content/browser/devtools/devtools_http_handler.h"
#include "content/browser/devtools/devtools_manager.h"
#include "content/browser/devtools/grit/devtools_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/devtools_socket_factory.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "content/shell/browser/shell.h"
#include "content/shell/grit/shell_resources.h"
#include "net/base/net_errors.h"
#include "net/socket/tcp_server_socket.h"
#include "third_party/zlib/google/compression_utils.h"
#include "ui/base/resource/resource_bundle.h"
#if defined(OS_TIZEN_TV_PRODUCT)
#include "devtools_port_manager.h"
#endif

using content::BrowserThread;
using content::DevToolsHttpHandler;

namespace {

// Copy of internal class implementation from
// content/shell/browser/shell_devtools_delegate.cc

const uint16_t kMinTetheringPort = 9333;
const uint16_t kMaxTetheringPort = 9444;
const int kBackLog = 10;

class TCPServerSocketFactory : public content::DevToolsSocketFactory {
 public:
  TCPServerSocketFactory(const std::string& address, uint16_t port)
      : address_(address), port_(port), server_socket_(NULL) {}
  TCPServerSocketFactory(std::unique_ptr<net::ServerSocket> server_socket)
      : port_(0), server_socket_(server_socket.release()) {}

 private:
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    if (server_socket_)
      return std::unique_ptr<net::ServerSocket>(server_socket_);
    std::unique_ptr<net::ServerSocket> socket(
        new net::TCPServerSocket(nullptr, net::NetLogSource()));
    if (socket->ListenWithAddressAndPort(address_, port_, kBackLog) != net::OK)
      return std::unique_ptr<net::ServerSocket>();

    return socket;
  }

  std::unique_ptr<net::ServerSocket> CreateForTethering(
      std::string* out_name) override {
    return nullptr;
  }

  net::ServerSocket* server_socket_;
  std::string address_;
  uint16_t port_;
};

}  // namespace

namespace content {

// static
uint16_t DevToolsDelegateEfl::port_ = 0;

// static
int DevToolsDelegateEfl::Start(int port) {
  if (port_ > 0)
    return port_;

  // It's a hacky way to early detected if port is available. The problem is
  // that the only thing we can do after checking is hope that noone will take
  // the port until it's initialized on IO thread again. The best approach would
  // be creating async callbacks that would inform when inspector server started
  // and on which port.
  static std::string addr = "0.0.0.0";
  std::unique_ptr<net::ServerSocket> sock(
      new net::TCPServerSocket(NULL, net::NetLogSource()));

  const base::CommandLine* const command_line =
      base::CommandLine::ForCurrentProcess();
  // See if the user specified a port on the command line (useful for
  // automation). If not, use an ephemeral port by specifying 0.
  if (!port && command_line->HasSwitch(switches::kRemoteDebuggingPort)) {
    int temp_port = 0;
    std::string port_str =
        command_line->GetSwitchValueASCII(switches::kRemoteDebuggingPort);
    base::StringToInt(port_str, &temp_port);
    if (temp_port > 0 && temp_port < 65535) {
      port = temp_port;
    } else {
      DLOG(WARNING) << "Invalid http debugger port number " << temp_port;
    }
  }

  // why backlog is 1?
  if (sock->ListenWithAddressAndPort(addr, port, 1) != net::OK) {
    port_ = 0;
#if defined(OS_TIZEN_TV_PRODUCT)
    if (devtools_http_handler::DevToolsPortManager::GetInstance())
      devtools_http_handler::DevToolsPortManager::GetInstance()->ReleasePort();
#endif
    return port_;
  }

  net::IPEndPoint givenIp;
  sock->GetLocalAddress(&givenIp);
  port_ = givenIp.port();

  std::unique_ptr<content::DevToolsSocketFactory> factory(
      new TCPServerSocketFactory(std::move(sock)));

  LOG(INFO) << __FUNCTION__ << "() port:" << port_;
  DevToolsAgentHost::StartRemoteDebuggingServer(
      std::move(factory), std::string(), base::FilePath(), base::FilePath());
  return port_;
}

// static
bool DevToolsDelegateEfl::Stop() {
  if (port_ == 0)
    return false;
  port_ = 0;
  DevToolsAgentHost::StopRemoteDebuggingServer();
  return true;
}

std::string DevToolsDelegateEfl::GetDiscoveryPageHTML() {
  return ResourceBundle::GetSharedInstance()
      .GetRawDataResource(IDR_CONTENT_SHELL_DEVTOOLS_DISCOVERY_PAGE)
      .as_string();
}

std::string DevToolsDelegateEfl::GetFrontendResource(const std::string& path) {
  std::string data =
      content::DevToolsFrontendHost::GetFrontendResource(path).as_string();
  // Detect gzip resource with the gzip magic number(1f 8b) in header
  if (data.length() > 2 && data[0] == '\x1F' && data[1] == '\x8B') {
    std::string uncompress_data;
    if (compression::GzipUncompress(data, &uncompress_data))
      return uncompress_data;
    LOG(ERROR) << "Failed to uncompress " << path;
  }
  return data;
}

#if defined(OS_TIZEN_TV_PRODUCT)
void DevToolsDelegateEfl::ReleasePort() {
  if (devtools_http_handler::DevToolsPortManager::GetInstance())
    devtools_http_handler::DevToolsPortManager::GetInstance()->ReleasePort();
}
#endif

}  // namespace content
