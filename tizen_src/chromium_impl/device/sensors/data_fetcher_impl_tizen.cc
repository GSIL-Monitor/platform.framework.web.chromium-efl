// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/sensors/data_fetcher_impl_tizen.h"

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "build/tizen_version.h"

namespace device {

static sensor_h sensor_orientation_;
static sensor_h sensor_accelerometer_;
static sensor_h sensor_gyroscope_;
static sensor_listener_h listener_orientation_;
static sensor_listener_h listener_accelerometer_;
static sensor_listener_h listener_gyroscope_;

DataFetcherImplTizen::DataFetcherImplTizen()
    : device_motion_buffer_(NULL),
      device_orientation_buffer_(NULL),
      last_motion_data_(NULL),
      has_last_motion_data_(false),
      last_motion_timestamp_(0),
      is_orientation_buffer_ready_(false) {
  sensor_get_default_sensor(SENSOR_ORIENTATION, &sensor_orientation_);
  sensor_get_default_sensor(SENSOR_ACCELEROMETER, &sensor_accelerometer_);
  sensor_get_default_sensor(SENSOR_GYROSCOPE, &sensor_gyroscope_);

  if (sensor_orientation_)
    sensor_create_listener(sensor_orientation_, &listener_orientation_);

  if (sensor_accelerometer_) {
    sensor_create_listener(sensor_accelerometer_, &listener_accelerometer_);

    if (sensor_gyroscope_)
      sensor_create_listener(sensor_gyroscope_, &listener_gyroscope_);
  }
}

DataFetcherImplTizen::~DataFetcherImplTizen() {
  if (sensor_orientation_) {
    sensor_listener_unset_event_cb(listener_orientation_);
    sensor_listener_stop(listener_orientation_);
    sensor_destroy_listener(listener_orientation_);
  }

  if (sensor_accelerometer_) {
    sensor_listener_unset_event_cb(listener_accelerometer_);
    sensor_listener_stop(listener_accelerometer_);
    sensor_destroy_listener(listener_accelerometer_);

    if (sensor_gyroscope_) {
      sensor_listener_stop(listener_gyroscope_);
      sensor_destroy_listener(listener_gyroscope_);
    }
  }
}

DataFetcherImplTizen* DataFetcherImplTizen::GetInstance() {
  return base::Singleton<DataFetcherImplTizen,
                         base::LeakySingletonTraits<DataFetcherImplTizen> >::get();
}

bool DataFetcherImplTizen::StartFetchingDeviceMotionData(
    DeviceMotionHardwareBuffer* buffer) {
  DCHECK(buffer);
  {
    base::AutoLock autolock(motion_buffer_lock_);
    device_motion_buffer_ = buffer;
  }
  sensor_listener_set_event_cb(listener_accelerometer_,
      kDeviceSensorIntervalMicroseconds / 1000,
      DataFetcherImplTizen::onAccelerationChanged, this);
  return Start(CONSUMER_TYPE_MOTION);
}

void DataFetcherImplTizen::StopFetchingDeviceMotionData() {
  Stop(CONSUMER_TYPE_MOTION);
  {
    base::AutoLock autolock(motion_buffer_lock_);
    if (device_motion_buffer_) {
      sensor_listener_unset_event_cb(listener_accelerometer_);
      device_motion_buffer_ = NULL;
    }
  }
}

bool DataFetcherImplTizen::StartFetchingDeviceOrientationData(
    DeviceOrientationHardwareBuffer* buffer) {
  DCHECK(buffer);
  {
    base::AutoLock autolock(orientation_buffer_lock_);
    device_orientation_buffer_ = buffer;
  }
  sensor_listener_set_event_cb(listener_orientation_,
      kDeviceSensorIntervalMicroseconds / 1000,
      DataFetcherImplTizen::onOrientationChanged, this);
  bool success = Start(CONSUMER_TYPE_ORIENTATION);

  {
    base::AutoLock autolock(orientation_buffer_lock_);
    // If Start() was unsuccessful then set the buffer ready flag to true
    // to start firing all-null events.
    SetOrientationBufferReadyStatus(!success);
  }
  return success;
}

void DataFetcherImplTizen::StopFetchingDeviceOrientationData() {
  Stop(CONSUMER_TYPE_ORIENTATION);
  {
    base::AutoLock autolock(orientation_buffer_lock_);
    if (device_orientation_buffer_) {
      SetOrientationBufferReadyStatus(false);
      sensor_listener_unset_event_cb(listener_orientation_);
      device_orientation_buffer_ = NULL;
    }
  }
}

bool DataFetcherImplTizen::Start(ConsumerType type) {
  switch(type) {
  case CONSUMER_TYPE_ORIENTATION:
    return (SENSOR_ERROR_NONE == sensor_listener_start(listener_orientation_));
  case CONSUMER_TYPE_MOTION:
    if (SENSOR_ERROR_NONE == sensor_listener_start(listener_accelerometer_)) {
      // For DeviceMotion event, Accelerometer Sensor is essential, and Gyroscope Sensor
      // being optional. If only Accel. Sensor, it will work partially. Thus return true.
      return (!sensor_gyroscope_ ||
              (SENSOR_ERROR_NONE == sensor_listener_start(listener_gyroscope_)));
    }
    return false;
  default:
    NOTREACHED();
    return false;
  }

}

void DataFetcherImplTizen::Stop(ConsumerType type) {
  switch(type) {
  case CONSUMER_TYPE_ORIENTATION:
    if (listener_orientation_)
      sensor_listener_stop(listener_orientation_);
    return;
  case CONSUMER_TYPE_MOTION:
    if (listener_accelerometer_)
      sensor_listener_stop(listener_accelerometer_);
    if (listener_gyroscope_)
      sensor_listener_stop(listener_gyroscope_);
    memset(&last_motion_data_, 0, sizeof(last_motion_data_));
    has_last_motion_data_ = false;
    return;
  default:
    NOTREACHED();
    return;
  }
}

//static
void DataFetcherImplTizen::onOrientationChanged(sensor_h sensor,
    sensor_event_s *event, void* userData) {
  DataFetcherImplTizen *fetcher = static_cast<DataFetcherImplTizen*>(userData);
  base::AutoLock autolock(fetcher->orientation_buffer_lock_);

  if (!fetcher->device_orientation_buffer_)
    return;

  fetcher->device_orientation_buffer_->seqlock.WriteBegin();

  float azimuth = event->values[0];
  float pitch = event->values[1];
  float roll = event->values[2];

  fetcher->device_orientation_buffer_->data.alpha = azimuth;
  fetcher->device_orientation_buffer_->data.has_alpha = true;
  fetcher->device_orientation_buffer_->data.beta = pitch;
  fetcher->device_orientation_buffer_->data.has_beta = true;
  fetcher->device_orientation_buffer_->data.gamma = roll;
  fetcher->device_orientation_buffer_->data.has_gamma = true;
  fetcher->device_orientation_buffer_->seqlock.WriteEnd();

  if (!fetcher->is_orientation_buffer_ready_)
    fetcher->SetOrientationBufferReadyStatus(true);
}

//static
void DataFetcherImplTizen::onAccelerationChanged(sensor_h sensor,
    sensor_event_s *event, void* userData) {
  DataFetcherImplTizen *self = static_cast<DataFetcherImplTizen*>(userData);
  base::AutoLock autolock(self->motion_buffer_lock_);

  if (!self->device_motion_buffer_)
	return;

  float x = event->values[0];
  float y = event->values[1];
  float z = event->values[2];

  float gravityX = x * 0.2f;
  float gravityY = y * 0.2f;
  float gravityZ = z * 0.2f;
  bool accelerationAvailable = false;

  unsigned long long timestamp = event->timestamp;

  double interval = static_cast<double>(self->last_motion_timestamp_ ?
      (timestamp - self->last_motion_timestamp_) / 1000 :
      kDeviceSensorIntervalMicroseconds / 1000);
  self->last_motion_timestamp_ = timestamp;

  if (self->has_last_motion_data_) {
    const DeviceMotionHardwareBuffer* m = self->last_motion_data_;
    gravityX += (m->data.acceleration_including_gravity_x - m->data.acceleration_x) * 0.8f;
    gravityY += (m->data.acceleration_including_gravity_y - m->data.acceleration_y) * 0.8f;
    gravityZ += (m->data.acceleration_including_gravity_z - m->data.acceleration_z) * 0.8f;
    accelerationAvailable = true;
  }

  float alpha, beta, gamma;
  bool rotationRateAvailable = false;

  sensor_event_s event_gyroscope;
  if (SENSOR_ERROR_NONE == sensor_listener_read_data(listener_gyroscope_, &event_gyroscope))
    rotationRateAvailable = true;

  alpha = event_gyroscope.values[0];
  beta = event_gyroscope.values[1];
  gamma = event_gyroscope.values[2];

  self->device_motion_buffer_->seqlock.WriteBegin();

  self->device_motion_buffer_->data.acceleration_including_gravity_x = x;
  self->device_motion_buffer_->data.has_acceleration_including_gravity_x = true;
  self->device_motion_buffer_->data.acceleration_including_gravity_y = y;
  self->device_motion_buffer_->data.has_acceleration_including_gravity_y = true;
  self->device_motion_buffer_->data.acceleration_including_gravity_z = z;
  self->device_motion_buffer_->data.has_acceleration_including_gravity_z = true;

  self->device_motion_buffer_->data.acceleration_x = x - gravityX;
  self->device_motion_buffer_->data.has_acceleration_x = accelerationAvailable;
  self->device_motion_buffer_->data.acceleration_y = y - gravityY;
  self->device_motion_buffer_->data.has_acceleration_y = accelerationAvailable;
  self->device_motion_buffer_->data.acceleration_z = z - gravityZ;
  self->device_motion_buffer_->data.has_acceleration_z = accelerationAvailable;

  self->device_motion_buffer_->data.rotation_rate_alpha = alpha;
  self->device_motion_buffer_->data.has_rotation_rate_alpha = rotationRateAvailable;
  self->device_motion_buffer_->data.rotation_rate_beta = beta;
  self->device_motion_buffer_->data.has_rotation_rate_beta = rotationRateAvailable;
  self->device_motion_buffer_->data.rotation_rate_gamma = gamma;
  self->device_motion_buffer_->data.has_rotation_rate_gamma = rotationRateAvailable;

  if (!sensor_gyroscope_)
	self->device_motion_buffer_->data.all_available_sensors_are_active =
	    accelerationAvailable;
  else
    self->device_motion_buffer_->data.all_available_sensors_are_active =
        (accelerationAvailable && rotationRateAvailable);

  self->last_motion_data_ = self->device_motion_buffer_;
  self->has_last_motion_data_ = true;

  self->device_motion_buffer_->data.interval = interval;

  self->device_motion_buffer_->seqlock.WriteEnd();
}

void DataFetcherImplTizen::SetOrientationBufferReadyStatus(bool ready) {
  device_orientation_buffer_->seqlock.WriteBegin();
  device_orientation_buffer_->data.absolute = ready;
  device_orientation_buffer_->data.all_available_sensors_are_active = ready;
  device_orientation_buffer_->seqlock.WriteEnd();
  is_orientation_buffer_ready_ = ready;
}

} // namespace content
