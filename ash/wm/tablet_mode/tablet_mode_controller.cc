// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/tablet_mode/tablet_mode_controller.h"

#include <utility>

#include "ash/public/cpp/ash_switches.h"
#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "ash/wm/tablet_mode/scoped_disable_internal_mouse_and_keyboard.h"
#include "ash/wm/tablet_mode/scoped_disable_internal_mouse_and_keyboard_ozone.h"
#include "ash/wm/tablet_mode/tablet_mode_observer.h"
#include "ash/wm/tablet_mode/tablet_mode_window_manager.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/chromeos/accelerometer/accelerometer_util.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/geometry/vector3d_f.h"

namespace ash {

namespace {

// The hinge angle at which to enter tablet mode.
const float kEnterTabletModeAngle = 200.0f;

// The angle at which to exit tablet mode, this is specifically less than the
// angle to enter tablet mode to prevent rapid toggling when near the angle.
const float kExitTabletModeAngle = 160.0f;

// Defines a range for which accelerometer readings are considered accurate.
// When the lid is near open (or near closed) the accelerometer readings may be
// inaccurate and a lid that is fully open may appear to be near closed (and
// vice versa).
const float kMinStableAngle = 20.0f;
const float kMaxStableAngle = 340.0f;

// The time duration to consider the lid to be recently opened.
// This is used to prevent entering tablet mode if an erroneous accelerometer
// reading makes the lid appear to be fully open when the user is opening the
// lid from a closed position.
const int kLidRecentlyOpenedDurationSeconds = 2;

// When the device approaches vertical orientation (i.e. portrait orientation)
// the accelerometers for the base and lid approach the same values (i.e.
// gravity pointing in the direction of the hinge). When this happens abrupt
// small acceleration perpendicular to the hinge can lead to incorrect hinge
// angle calculations. To prevent this the accelerometer updates will be
// smoothed over time in order to reduce this noise.
// This is the minimum acceleration parallel to the hinge under which to begin
// smoothing in m/s^2.
const float kHingeVerticalSmoothingStart = 7.0f;
// This is the maximum acceleration parallel to the hinge under which smoothing
// will incorporate new acceleration values, in m/s^2.
const float kHingeVerticalSmoothingMaximum = 8.7f;

// The maximum deviation between the magnitude of the two accelerometers under
// which to detect hinge angle in m/s^2. These accelerometers are attached to
// the same physical device and so should be under the same acceleration.
const float kNoisyMagnitudeDeviation = 1.0f;

// The angle between chromeos::AccelerometerReadings are considered stable if
// their magnitudes do not differ greatly. This returns false if the deviation
// between the screen and keyboard accelerometers is too high.
bool IsAngleBetweenAccelerometerReadingsStable(
    const chromeos::AccelerometerUpdate& update) {
  return std::abs(
             ui::ConvertAccelerometerReadingToVector3dF(
                 update.get(chromeos::ACCELEROMETER_SOURCE_ATTACHED_KEYBOARD))
                 .Length() -
             ui::ConvertAccelerometerReadingToVector3dF(
                 update.get(chromeos::ACCELEROMETER_SOURCE_SCREEN))
                 .Length()) <= kNoisyMagnitudeDeviation;
}

bool IsEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kAshEnableTabletMode);
}

// Checks the command line to see which force tablet mode is turned on, if
// any.
TabletModeController::UiMode GetTabletMode() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kAshUiMode)) {
    std::string switch_value =
        command_line->GetSwitchValueASCII(switches::kAshUiMode);
    if (switch_value == switches::kAshUiModeClamshell)
      return TabletModeController::UiMode::CLAMSHELL;

    if (switch_value == switches::kAshUiModeTablet)
      return TabletModeController::UiMode::TABLETMODE;
  }
  return TabletModeController::UiMode::NONE;
}

std::unique_ptr<ScopedDisableInternalMouseAndKeyboard>
CreateScopedDisableInternalMouseAndKeyboard() {
  return std::make_unique<ScopedDisableInternalMouseAndKeyboardOzone>();
}

}  // namespace

TabletModeController::TabletModeController()
    : have_seen_accelerometer_data_(false),
      can_detect_lid_angle_(false),
      tabletmode_usage_interval_start_time_(base::Time::Now()),
      tick_clock_(new base::DefaultTickClock()),
      tablet_mode_switch_is_on_(false),
      lid_is_closed_(false),
      auto_hide_title_bars_(!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshDisableTabletAutohideTitlebars)),
      scoped_session_observer_(this),
      weak_factory_(this) {
  Shell::Get()->AddShellObserver(this);
  base::RecordAction(base::UserMetricsAction("Touchview_Initially_Disabled"));

  // TODO(jonross): Do not create TabletModeController if the flag is
  // unavailable. This will require refactoring
  // IsTabletModeWindowManagerEnabled to check for the existence of the
  // controller.
  if (IsEnabled()) {
    Shell::Get()->window_tree_host_manager()->AddObserver(this);
    chromeos::AccelerometerReader::GetInstance()->AddObserver(this);
  }
  chromeos::PowerManagerClient* power_manager_client =
      chromeos::DBusThreadManager::Get()->GetPowerManagerClient();
  power_manager_client->AddObserver(this);
  power_manager_client->GetSwitchStates(base::Bind(
      &TabletModeController::OnGetSwitchStates, weak_factory_.GetWeakPtr()));
}

TabletModeController::~TabletModeController() {
  Shell::Get()->RemoveShellObserver(this);

  if (IsEnabled()) {
    Shell::Get()->window_tree_host_manager()->RemoveObserver(this);
    chromeos::AccelerometerReader::GetInstance()->RemoveObserver(this);
  }
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
}

bool TabletModeController::CanEnterTabletMode() {
  // If we have ever seen accelerometer data, then HandleHingeRotation may
  // trigger tablet mode at some point in the future.
  // All TabletMode-enabled devices can enter tablet mode.
  return have_seen_accelerometer_data_ || IsEnabled();
}

// TODO(jcliang): Hide or remove EnableTabletModeWindowManager
// (http://crbug.com/620241).
void TabletModeController::EnableTabletModeWindowManager(bool should_enable) {
  bool is_enabled = !!tablet_mode_window_manager_.get();
  if (should_enable == is_enabled)
    return;

  if (should_enable) {
    tablet_mode_window_manager_.reset(new TabletModeWindowManager());
    base::RecordAction(base::UserMetricsAction("Touchview_Enabled"));
    RecordTabletModeUsageInterval(TABLET_MODE_INTERVAL_INACTIVE);
    for (auto& observer : tablet_mode_observers_)
      observer.OnTabletModeStarted();

    if (client_)  // Null at startup and in tests.
      client_->OnTabletModeToggled(true);
  } else {
    tablet_mode_window_manager_->SetIgnoreWmEventsForExit();
    for (auto& observer : tablet_mode_observers_)
      observer.OnTabletModeEnding();
    tablet_mode_window_manager_.reset();
    base::RecordAction(base::UserMetricsAction("Touchview_Disabled"));
    RecordTabletModeUsageInterval(TABLET_MODE_INTERVAL_ACTIVE);
    for (auto& observer : tablet_mode_observers_)
      observer.OnTabletModeEnded();

    if (client_)  // Null at startup and in tests.
      client_->OnTabletModeToggled(false);
  }
}

bool TabletModeController::IsTabletModeWindowManagerEnabled() const {
  return tablet_mode_window_manager_.get() != NULL;
}

void TabletModeController::AddWindow(aura::Window* window) {
  if (IsTabletModeWindowManagerEnabled())
    tablet_mode_window_manager_->AddWindow(window);
}

void TabletModeController::BindRequest(
    mojom::TabletModeControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void TabletModeController::AddObserver(TabletModeObserver* observer) {
  tablet_mode_observers_.AddObserver(observer);
}

void TabletModeController::RemoveObserver(TabletModeObserver* observer) {
  tablet_mode_observers_.RemoveObserver(observer);
}

bool TabletModeController::ShouldAutoHideTitlebars() const {
  return auto_hide_title_bars_ && IsTabletModeWindowManagerEnabled();
}

void TabletModeController::OnAccelerometerUpdated(
    scoped_refptr<const chromeos::AccelerometerUpdate> update) {
  if (!AllowEnterExitTabletMode())
    return;

  have_seen_accelerometer_data_ = true;
  can_detect_lid_angle_ =
      update->has(chromeos::ACCELEROMETER_SOURCE_SCREEN) &&
      update->has(chromeos::ACCELEROMETER_SOURCE_ATTACHED_KEYBOARD);

  if (!can_detect_lid_angle_)
    return;

  if (!display::Display::HasInternalDisplay())
    return;

  if (!Shell::Get()->display_manager()->IsActiveDisplayId(
          display::Display::InternalDisplayId())) {
    return;
  }

  // Whether or not we enter tablet mode affects whether we handle screen
  // rotation, so determine whether to enter tablet mode first.
  if (ui::IsAccelerometerReadingStable(*update,
                                       chromeos::ACCELEROMETER_SOURCE_SCREEN) &&
      ui::IsAccelerometerReadingStable(
          *update, chromeos::ACCELEROMETER_SOURCE_ATTACHED_KEYBOARD) &&
      IsAngleBetweenAccelerometerReadingsStable(*update)) {
    // update.has(chromeos::ACCELEROMETER_SOURCE_ATTACHED_KEYBOARD)
    // Ignore the reading if it appears unstable. The reading is considered
    // unstable if it deviates too much from gravity and/or the magnitude of the
    // reading from the lid differs too much from the reading from the base.
    HandleHingeRotation(update);
  }
}

void TabletModeController::LidEventReceived(
    chromeos::PowerManagerClient::LidState state,
    const base::TimeTicks& time) {
  if (!AllowEnterExitTabletMode())
    return;

  const bool open = state == chromeos::PowerManagerClient::LidState::OPEN;
  if (open)
    last_lid_open_time_ = time;
  lid_is_closed_ = !open;
  LeaveTabletMode();
}

void TabletModeController::TabletModeEventReceived(
    chromeos::PowerManagerClient::TabletMode mode,
    const base::TimeTicks& time) {
  if (!AllowEnterExitTabletMode())
    return;

  const bool on = mode == chromeos::PowerManagerClient::TabletMode::ON;
  tablet_mode_switch_is_on_ = on;
  // Do not change if docked.
  if (!display::Display::HasInternalDisplay() ||
      !Shell::Get()->display_manager()->IsActiveDisplayId(
          display::Display::InternalDisplayId())) {
    return;
  }
  // The tablet mode switch activates at 300 degrees, so it is always reliable
  // when |on|. However we wish to exit tablet mode at a smaller angle, so
  // when |on| is false we ignore if it is possible to calculate the lid angle.
  if (on && !IsTabletModeWindowManagerEnabled()) {
    EnterTabletMode();
  } else if (!on && IsTabletModeWindowManagerEnabled() &&
             !can_detect_lid_angle_) {
    LeaveTabletMode();
  }
}

void TabletModeController::SuspendImminent() {
  // The system is about to suspend, so record TabletMode usage interval metrics
  // based on whether TabletMode mode is currently active.
  RecordTabletModeUsageInterval(CurrentTabletModeIntervalType());
}

void TabletModeController::SuspendDone(const base::TimeDelta& sleep_duration) {
  // We do not want TabletMode usage metrics to include time spent in suspend.
  tabletmode_usage_interval_start_time_ = base::Time::Now();
}

void TabletModeController::HandleHingeRotation(
    scoped_refptr<const chromeos::AccelerometerUpdate> update) {
  static const gfx::Vector3dF hinge_vector(1.0f, 0.0f, 0.0f);
  gfx::Vector3dF base_reading(ui::ConvertAccelerometerReadingToVector3dF(
      update->get(chromeos::ACCELEROMETER_SOURCE_ATTACHED_KEYBOARD)));
  gfx::Vector3dF lid_reading(ui::ConvertAccelerometerReadingToVector3dF(
      update->get(chromeos::ACCELEROMETER_SOURCE_SCREEN)));

  // As the hinge approaches a vertical angle, the base and lid accelerometers
  // approach the same values making any angle calculations highly inaccurate.
  // Smooth out instantaneous acceleration when nearly vertical to increase
  // accuracy.
  float largest_hinge_acceleration =
      std::max(std::abs(base_reading.x()), std::abs(lid_reading.x()));
  float smoothing_ratio =
      std::max(0.0f, std::min(1.0f, (largest_hinge_acceleration -
                                     kHingeVerticalSmoothingStart) /
                                        (kHingeVerticalSmoothingMaximum -
                                         kHingeVerticalSmoothingStart)));

  // We cannot trust the computed lid angle when the device is held vertically.
  bool is_angle_reliable =
      largest_hinge_acceleration <= kHingeVerticalSmoothingMaximum;

  base_smoothed_.Scale(smoothing_ratio);
  base_reading.Scale(1.0f - smoothing_ratio);
  base_smoothed_.Add(base_reading);

  lid_smoothed_.Scale(smoothing_ratio);
  lid_reading.Scale(1.0f - smoothing_ratio);
  lid_smoothed_.Add(lid_reading);

  if (tablet_mode_switch_is_on_)
    return;

  // Ignore the component of acceleration parallel to the hinge for the purposes
  // of hinge angle calculation.
  gfx::Vector3dF base_flattened(base_smoothed_);
  gfx::Vector3dF lid_flattened(lid_smoothed_);
  base_flattened.set_x(0.0f);
  lid_flattened.set_x(0.0f);

  // Compute the angle between the base and the lid.
  float lid_angle = 180.0f - gfx::ClockwiseAngleBetweenVectorsInDegrees(
                                 base_flattened, lid_flattened, hinge_vector);
  if (lid_angle < 0.0f)
    lid_angle += 360.0f;

  bool is_angle_stable = is_angle_reliable && lid_angle >= kMinStableAngle &&
                         lid_angle <= kMaxStableAngle;

  // Clear the last_lid_open_time_ for a stable reading so that there is less
  // chance of a delay if the lid is moved from the close state to the fully
  // open state very quickly.
  if (is_angle_stable)
    last_lid_open_time_ = base::TimeTicks();

  // Toggle tablet mode on or off when corresponding thresholds are passed.
  if (IsTabletModeWindowManagerEnabled() && is_angle_stable &&
      lid_angle <= kExitTabletModeAngle) {
    LeaveTabletMode();
  } else if (!IsTabletModeWindowManagerEnabled() && !lid_is_closed_ &&
             lid_angle >= kEnterTabletModeAngle &&
             (is_angle_stable || !WasLidOpenedRecently())) {
    EnterTabletMode();
  }
}

void TabletModeController::EnterTabletMode() {
  // Always reset first to avoid creation before destruction of a previous
  // object.
  event_blocker_ = CreateScopedDisableInternalMouseAndKeyboard();

  if (IsTabletModeWindowManagerEnabled())
    return;
  EnableTabletModeWindowManager(true);
}

void TabletModeController::LeaveTabletMode() {
  event_blocker_.reset();

  if (!IsTabletModeWindowManagerEnabled())
    return;
  EnableTabletModeWindowManager(false);
}

void TabletModeController::FlushForTesting() {
  bindings_.FlushForTesting();
}

void TabletModeController::OnShellInitialized() {
  force_ui_mode_ = GetTabletMode();
  if (force_ui_mode_ == UiMode::TABLETMODE)
    EnterTabletMode();
}

void TabletModeController::OnDisplayConfigurationChanged() {
  if (!display::Display::HasInternalDisplay() ||
      !Shell::Get()->display_manager()->IsActiveDisplayId(
          display::Display::InternalDisplayId())) {
    LeaveTabletMode();
  } else if (tablet_mode_switch_is_on_ && !IsTabletModeWindowManagerEnabled()) {
    // The internal display has returned, as we are exiting docked mode.
    // The device is still in tablet mode, so trigger tablet mode, as this
    // switch leads to the ignoring of accelerometer events. When the switch is
    // not set the next stable accelerometer readings will trigger maximize
    // mode.
    EnterTabletMode();
  }
}

void TabletModeController::RecordTabletModeUsageInterval(
    TabletModeIntervalType type) {
  if (!CanEnterTabletMode())
    return;

  base::Time current_time = base::Time::Now();
  base::TimeDelta delta = current_time - tabletmode_usage_interval_start_time_;
  switch (type) {
    case TABLET_MODE_INTERVAL_INACTIVE:
      UMA_HISTOGRAM_LONG_TIMES("Ash.TouchView.TouchViewInactive", delta);
      total_non_tabletmode_time_ += delta;
      break;
    case TABLET_MODE_INTERVAL_ACTIVE:
      UMA_HISTOGRAM_LONG_TIMES("Ash.TouchView.TouchViewActive", delta);
      total_tabletmode_time_ += delta;
      break;
  }

  tabletmode_usage_interval_start_time_ = current_time;
}

TabletModeController::TabletModeIntervalType
TabletModeController::CurrentTabletModeIntervalType() {
  if (IsTabletModeWindowManagerEnabled())
    return TABLET_MODE_INTERVAL_ACTIVE;
  return TABLET_MODE_INTERVAL_INACTIVE;
}

void TabletModeController::SetClient(mojom::TabletModeClientPtr client) {
  client_ = std::move(client);
  client_->OnTabletModeToggled(IsTabletModeWindowManagerEnabled());
}

bool TabletModeController::AllowEnterExitTabletMode() const {
  return force_ui_mode_ == UiMode::NONE;
}

void TabletModeController::OnChromeTerminating() {
  // The system is about to shut down, so record TabletMode usage interval
  // metrics based on whether TabletMode mode is currently active.
  RecordTabletModeUsageInterval(CurrentTabletModeIntervalType());

  if (CanEnterTabletMode()) {
    UMA_HISTOGRAM_CUSTOM_COUNTS("Ash.TouchView.TouchViewActiveTotal",
                                total_tabletmode_time_.InMinutes(), 1,
                                base::TimeDelta::FromDays(7).InMinutes(), 50);
    UMA_HISTOGRAM_CUSTOM_COUNTS("Ash.TouchView.TouchViewInactiveTotal",
                                total_non_tabletmode_time_.InMinutes(), 1,
                                base::TimeDelta::FromDays(7).InMinutes(), 50);
    base::TimeDelta total_runtime =
        total_tabletmode_time_ + total_non_tabletmode_time_;
    if (total_runtime.InSeconds() > 0) {
      UMA_HISTOGRAM_PERCENTAGE(
          "Ash.TouchView.TouchViewActivePercentage",
          100 * total_tabletmode_time_.InSeconds() / total_runtime.InSeconds());
    }
  }
}

void TabletModeController::OnGetSwitchStates(
    chromeos::PowerManagerClient::LidState lid_state,
    chromeos::PowerManagerClient::TabletMode tablet_mode) {
  LidEventReceived(lid_state, base::TimeTicks::Now());
  TabletModeEventReceived(tablet_mode, base::TimeTicks::Now());
}

bool TabletModeController::WasLidOpenedRecently() const {
  if (last_lid_open_time_.is_null())
    return false;

  base::TimeTicks now = tick_clock_->NowTicks();
  DCHECK(now >= last_lid_open_time_);
  base::TimeDelta elapsed_time = now - last_lid_open_time_;
  return elapsed_time.InSeconds() <= kLidRecentlyOpenedDurationSeconds;
}

void TabletModeController::SetTickClockForTest(
    std::unique_ptr<base::TickClock> tick_clock) {
  DCHECK(tick_clock_);
  tick_clock_ = std::move(tick_clock);
}

}  // namespace ash
