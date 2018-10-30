// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/geolocation/location_provider_efl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/time/time.h"
#include "device/geolocation/geoposition.h"

namespace device {

static double KilometerPerHourToMeterPerSecond(double kilometer_per_hour) {
  return kilometer_per_hour * 5 / 18;
}

LocationProviderEfl::LocationProviderEfl()
    : LocationProvider(),
      location_manager_(NULL),
      geolocation_message_loop_(base::MessageLoop::current()) {}

LocationProviderEfl::~LocationProviderEfl() {
  StopProvider();
}

// static
void LocationProviderEfl::GeoPositionChangedCb(double latitude,
                                               double longitude,
                                               double altitude,
                                               double speed,
                                               double direction,
                                               double horizontal_accuracy,
                                               time_t timestamp,
                                               void* user_data) {
  LocationProviderEfl* location_provider =
      static_cast<LocationProviderEfl*>(user_data);
  DCHECK(location_provider);
  location_provider->NotifyPositionChanged(latitude, longitude, altitude, speed,
                                           direction, horizontal_accuracy,
                                           timestamp);
}

void LocationProviderEfl::NotifyCallback(const Geoposition& position) {
  if (!callback_.is_null())
    callback_.Run(this, position);
}

void LocationProviderEfl::SetUpdateCallback(
    const LocationProviderUpdateCallback& callback) {
  callback_ = callback;
}

void LocationProviderEfl::NotifyPositionChanged(double latitude,
                                                double longitude,
                                                double altitude,
                                                double speed,
                                                double direction,
                                                double horizontal_accuracy,
                                                time_t timestamp) {
  location_accuracy_level_e level;

  DCHECK(location_manager_);
  DCHECK(geolocation_message_loop_);

  last_position_.latitude = latitude;
  last_position_.longitude = longitude;
  last_position_.altitude = altitude;
  last_position_.timestamp = base::Time::FromTimeT(timestamp);
  last_position_.speed = KilometerPerHourToMeterPerSecond(speed);
  last_position_.heading = direction;

  location_manager_get_last_accuracy(location_manager_,
      &level, &last_position_.accuracy, &last_position_.altitude_accuracy);

  base::Closure task = base::Bind(&LocationProviderEfl::NotifyCallback,
                                    base::Unretained(this),
                                    last_position_);
  geolocation_message_loop_->task_runner()->PostTask(FROM_HERE, task);
}

bool LocationProviderEfl::StartProvider(bool high_accuracy) {
  if (location_manager_) {
    // already started!
    return false;
  }

  int ret = location_manager_create(LOCATIONS_METHOD_HYBRID, &location_manager_);
  if (ret != LOCATIONS_ERROR_NONE) {
    LOG(ERROR) << "Failed to create location manager!";
    return false;
  }

  ret = location_manager_set_location_changed_cb(location_manager_,
                                                 GeoPositionChangedCb, 1, this);
  if (ret != LOCATIONS_ERROR_NONE) {
    LOG(ERROR) << "Failed to register position changed callback!";
    location_manager_destroy(location_manager_);
    location_manager_ = NULL;
    return false;
  }

  ret = static_cast<location_error_e>(location_manager_start(location_manager_));
  if (ret != LOCATIONS_ERROR_NONE) {
    LOG(ERROR) << "Failed to start location manager: " << ret;
    location_manager_unset_position_updated_cb(location_manager_);
    location_manager_destroy(location_manager_);
    location_manager_ = NULL;
    return false;
  }

  return true;
}

void LocationProviderEfl::StopProvider() {
  if (location_manager_) {
    int ret = location_manager_stop(location_manager_);
    if (ret == LOCATIONS_ERROR_NONE) {
      location_manager_unset_position_updated_cb(location_manager_);
    }

    // TODO: didn't stop but destroy?
    location_manager_destroy(location_manager_);
    location_manager_ = NULL;
  }
}

const Geoposition& LocationProviderEfl::GetPosition() {
  return last_position_;
}

void LocationProviderEfl::OnPermissionGranted() {
}

//static
LocationProvider* LocationProviderEfl::Create() {
  return new LocationProviderEfl();
}

std::unique_ptr<LocationProvider> NewSystemLocationProvider() {
  return base::WrapUnique(LocationProviderEfl::Create());
}

}  // namespace device
