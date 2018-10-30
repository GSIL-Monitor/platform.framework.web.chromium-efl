// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "hbbtv_widget_host.h"

#include "base/lazy_instance.h"
#include "common/render_messages_ewk.h"
#include "content/browser/zygote_host/zygote_communication_linux.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/zygote_handle_linux.h"
#include "services/device/time_zone_monitor/time_zone_monitor.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"

using content::RenderProcessHost;

HbbtvWidgetHost::HbbtvWidgetHost() {}

HbbtvWidgetHost::~HbbtvWidgetHost() {}

class HbbtvWidgetHostGetter {
 public:
  HbbtvWidgetHost& Host() { return host_; }

 private:
  HbbtvWidgetHost host_;
};

void SendToAllRenderers(IPC::Message* message) {
  content::RenderProcessHost::iterator it =
      content::RenderProcessHost::AllHostsIterator();
  while (!it.IsAtEnd()) {
    it.GetCurrentValue()->Send(new IPC::Message(*message));
    it.Advance();
  }
  delete message;
}

base::LazyInstance<HbbtvWidgetHostGetter>::Leaky g_hbbtv_widget_host_getter =
    LAZY_INSTANCE_INITIALIZER;

HbbtvWidgetHost& HbbtvWidgetHost::Get() {
  return g_hbbtv_widget_host_getter.Get().Host();
}

void HbbtvWidgetHost::RegisterURLSchemesAsCORSEnabled(
    const std::string& schemes) {
  if (cors_enabled_url_schemes_ == schemes)
    return;

  cors_enabled_url_schemes_ = schemes;
  LOG(INFO) << "schemes = " << schemes;
  SendToAllRenderers(
      new HbbtvMsg_RegisterURLSchemesAsCORSEnabled(cors_enabled_url_schemes_));
}

const std::string& HbbtvWidgetHost::GetCORSEnabledURLSchemes() {
  return cors_enabled_url_schemes_;
}

void HbbtvWidgetHost::RegisterJSPluginMimeTypes(const std::string& mime_types) {
  if (mime_types_ == mime_types)
    return;

  LOG(INFO) << "mime types " << mime_types;
  mime_types_ = mime_types;
  SendToAllRenderers(new HbbtvMsg_RegisterJSPluginMimeTypes(mime_types));
}

const std::string& HbbtvWidgetHost::GetJSPluginMimeTypes() {
  return mime_types_;
}

double HbbtvWidgetHost::GetTimeOffset() {
  return time_offset_;
}

void HbbtvWidgetHost::SetTimeOffset(double time_offset) {
  if (time_offset_ == time_offset)
    return;

  time_offset_ = time_offset;
  base::Time::SetTimeOffset(time_offset_);
  LOG(INFO) << "time offset " << time_offset_;
  SendToAllRenderers(new HbbtvMsg_SetTimeOffset(time_offset_));
}

static void FormatHour(long time, std::ostringstream& ost) {
  static const double kMinutesPerHour = 60.0;
  ost.width(2);
  ost.fill('0');
  ost << (int)(time / kMinutesPerHour);
  ost << ":";
  ost.width(2);
  ost.fill('0');
  ost << (int)(time % static_cast<int>(kMinutesPerHour));
}

static std::string CreateTimeZoneID(double time_zone_offset,
                                    double daylight_saving_time) {
  static const double kMilliSecondsPerMinute = 60.0 * 1000.0;
  static const char* kMinusSign = "-";
  static const char* kPlusSign = "+";

  long time_zone = (long)(time_zone_offset / kMilliSecondsPerMinute);
  const long daylight_saving =
      (long)(daylight_saving_time / kMilliSecondsPerMinute);

  if (!time_zone && (daylight_saving <= 0))
    return std::string();

  std::ostringstream ost;
  const char* sign = kMinusSign;

  if (daylight_saving > 0) {
    long total_offset = time_zone + daylight_saving;
    if (total_offset < 0) {
      total_offset *= -1;
      sign = kPlusSign;
    }
    ost << "GMST" << sign;
    FormatHour(time_zone, ost);
    ost << ":00GMDT" << sign;
    FormatHour(total_offset, ost);
    ost << ":00";
    return ost.str();
  }

  if (time_zone < 0) {
    time_zone *= -1;
    sign = kPlusSign;
  }
  ost << "GMT" << sign;
  FormatHour(time_zone, ost);

  return ost.str();
}

void HbbtvWidgetHost::SetTimeZoneOffset(double time_zone_offset,
                                        double daylight_saving_time) {
  if (time_zone_offset_ == time_zone_offset &&
      daylight_saving_time_ == daylight_saving_time)
    return;

  std::string zone_id_str =
      CreateTimeZoneID(time_zone_offset, daylight_saving_time);
  if (zone_id_str.empty())
    return;

  time_zone_offset_ = time_zone_offset;
  daylight_saving_time_ = daylight_saving_time;

  /**
  For China is UTC+8:00 or GMT+8:00
  TZ needs to set it with opposite sign as follows:
  TZ=UTC-8:00; export TZ
  or,
  TZ=GMT-8:00; export TZ
  or,
  TZ=Asia/Shanghai;export TZ
  For US EST (GMT-5:00, New York):
  TZ=UTC+5:00; export TZ
  or,
  TZ=GMT+5:00; export TZ
  or,
  TZ=America/New_York;export TZ
  */
  setenv("TZ", zone_id_str.c_str(), 1);
  tzset();

  // correct opposite sign from TZ
  double total_offset = time_zone_offset + daylight_saving_time;
  if (total_offset > 0) {
    zone_id_str.replace(zone_id_str.find("-"), 1, "+");
  } else {
    zone_id_str.replace(zone_id_str.find("+"), 1, "-");
  }
  LOG(INFO) << "timezone reset to " << zone_id_str;
  content::GetGenericZygote()->SetTimeZoneOffset(zone_id_str);

  icu::TimeZone::adoptDefault(icu::TimeZone::createTimeZone(
      icu::UnicodeString::fromUTF8(zone_id_str)));

  if (time_zone_monitor_)
    time_zone_monitor_->NotifyTimeZoneChange(zone_id_str);
}

void HbbtvWidgetHost::SetTimeZoneMonitor(device::TimeZoneMonitor* monitor) {
  time_zone_monitor_ = monitor;
}
