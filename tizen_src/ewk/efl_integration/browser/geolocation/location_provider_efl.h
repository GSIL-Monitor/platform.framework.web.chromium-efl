// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LOCATION_PROVIDER_EFL_H_
#define LOCATION_PROVIDER_EFL_H_

#include "base/compiler_specific.h"
#include "base/memory/ptr_util.h"
#include "browser/geolocation/access_token_store_efl.h"
#include "device/geolocation/access_token_store.h"
#include "device/geolocation/geolocation_delegate.h"
#include "device/geolocation/geolocation_provider.h"
#include "device/geolocation/geoposition.h"
#include "device/geolocation/location_provider.h"

#include <location/locations.h>

namespace base {
class MessageLoop;
}

namespace device {

// A provider of Geolocation services to override AccessTokenStore.
class GeolocationDelegateEfl : public GeolocationDelegate {
 public:
  GeolocationDelegateEfl() = default;

  scoped_refptr<AccessTokenStore> CreateAccessTokenStore() {
    return new AccessTokenStoreEfl();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(GeolocationDelegateEfl);
};

class LocationProviderEfl : public LocationProvider {
 public:
  virtual ~LocationProviderEfl();

  static LocationProvider* Create();

  void SetUpdateCallback(
      const LocationProviderUpdateCallback& callback) override;
  virtual bool StartProvider(bool high_accuracy) override;
  virtual void StopProvider() override;
  virtual const Geoposition& GetPosition() override;
  virtual void OnPermissionGranted() override;

 private:
  LocationProviderEfl();
  static void GeoPositionChangedCb(double,
                                   double,
                                   double,
                                   double,
                                   double,
                                   double,
                                   time_t,
                                   void*);
  void
  NotifyPositionChanged(double, double, double, double, double, double, time_t);

  void NotifyCallback(const Geoposition&);

  Geoposition last_position_;
  location_manager_h location_manager_;
  base::MessageLoop* geolocation_message_loop_;

  LocationProviderUpdateCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(LocationProviderEfl);
};

}  // namespace device

#endif  // LOCATION_PROVIDER_EFL_H_
