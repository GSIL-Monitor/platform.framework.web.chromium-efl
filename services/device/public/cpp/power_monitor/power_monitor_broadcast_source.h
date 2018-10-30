// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_PUBLIC_CPP_POWER_MONITOR_POWER_MONITOR_BROADCAST_SOURCE_H_
#define SERVICES_DEVICE_PUBLIC_CPP_POWER_MONITOR_POWER_MONITOR_BROADCAST_SOURCE_H_

#include <memory>

#include "base/atomicops.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/power_monitor/power_monitor_source.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/public/interfaces/power_monitor.mojom.h"

namespace base {
class SequencedTaskRunner;
}

namespace service_manager {
class Connector;
}

namespace device {

// Receives state changes from Power Monitor through mojo, and relays them to
// the PowerMonitor of the current process.
class PowerMonitorBroadcastSource : public base::PowerMonitorSource {
 public:
  PowerMonitorBroadcastSource(
      service_manager::Connector* connector,
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~PowerMonitorBroadcastSource() override;

 private:
  friend class PowerMonitorBroadcastSourceTest;
  friend class MockClient;
  FRIEND_TEST_ALL_PREFIXES(PowerMonitorBroadcastSourceTest,
                           PowerMessageReceiveBroadcast);
  FRIEND_TEST_ALL_PREFIXES(PowerMonitorMessageBroadcasterTest,
                           PowerMessageBroadcast);

  class Client : public device::mojom::PowerMonitorClient {
   public:
    Client();
    ~Client() override;

    void Init(std::unique_ptr<service_manager::Connector> connector);
    bool last_reported_battery_power_state();

    void PowerStateChange(bool on_battery_power) override;
    void Suspend() override;
    void Resume() override;

   private:
    volatile base::subtle::AtomicWord last_reported_battery_power_state_;
    std::unique_ptr<service_manager::Connector> connector_;
    mojo::Binding<device::mojom::PowerMonitorClient> binding_;

    DISALLOW_COPY_AND_ASSIGN(Client);
  };

  // This constructor is used by test code to mock the Client class.
  PowerMonitorBroadcastSource(
      std::unique_ptr<Client> client,
      service_manager::Connector* connector,
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  Client* client_for_testing() const { return client_.get(); }

  bool IsOnBatteryPowerImpl() override;
  std::unique_ptr<Client> client_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(PowerMonitorBroadcastSource);
};

}  // namespace device

#endif  // SERVICES_DEVICE_PUBLIC_CPP_POWER_MONITOR_POWER_MONITOR_BROADCAST_SOURCE_H_
