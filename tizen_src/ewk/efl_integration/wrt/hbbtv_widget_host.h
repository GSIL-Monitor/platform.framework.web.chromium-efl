// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_EFL_INTEGRATION_WRT_HBBTV_WIDGET_HOST
#define EWK_EFL_INTEGRATION_WRT_HBBTV_WIDGET_HOST

#include <string>

namespace device {
class TimeZoneMonitor;
}

class HbbtvWidgetHostGetter;

class HbbtvWidgetHost {
 public:
  static HbbtvWidgetHost& Get();
  ~HbbtvWidgetHost();

 public:
  void RegisterURLSchemesAsCORSEnabled(const std::string&);
  const std::string& GetCORSEnabledURLSchemes();

  void RegisterJSPluginMimeTypes(const std::string&);
  const std::string& GetJSPluginMimeTypes();

  void SetTimeOffset(double);
  double GetTimeOffset();
  void SetTimeZoneOffset(double time_zone_offset, double daylight_saving_time);
  void SetTimeZoneMonitor(device::TimeZoneMonitor*);

 private:
  HbbtvWidgetHost();

  double time_offset_ = 0.0;
  double time_zone_offset_ = 0.0;
  double daylight_saving_time_ = 0.0;
  device::TimeZoneMonitor* time_zone_monitor_ = nullptr;
  std::string cors_enabled_url_schemes_;
  std::string mime_types_;
  friend class HbbtvWidgetHostGetter;
};

#endif  // EWK_EFL_INTEGRATION_WRT_HBBTV_WIDGET_HOST
