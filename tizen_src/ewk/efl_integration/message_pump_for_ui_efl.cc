// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "message_pump_for_ui_efl.h"

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "cc/base/switches.h"
#include "common/content_switches_efl.h"

namespace base {

namespace {

const char dummy_pipe_message[] = "W";
const int dummy_pipe_message_size = 1;

}

MessagePumpForUIEfl::MessagePumpForUIEfl()
    : pipe_(ecore_pipe_add(&PipeCallback, this)),
      schedule_delayed_work_timer_(nullptr),
      run_loop_(nullptr),
      keep_running_(true),
      work_scheduled_(false) {}

MessagePumpForUIEfl::~MessagePumpForUIEfl() {
  CommandLine* cmdline = CommandLine::ForCurrentProcess();
  if (!cmdline->HasSwitch(
          switches::kLimitMemoryAllocationInScheduleDelayedWork)) {
    /* LCOV_EXCL_START */
    for (std::unordered_set<TimerPair*>::iterator it = pending_timers_.begin();
         it != pending_timers_.end(); ++it) {
      ecore_timer_del((*it)->second);
      delete *it;
      /* LCOV_EXCL_STOP */
    }
  } else {
    // Since we pass |this| object to timer's callback, deactivate the timer
    // to prevent it from accessing invalid |this| object.
    if (schedule_delayed_work_timer_)
      ecore_timer_del(schedule_delayed_work_timer_);  // LCOV_EXCL_LINE
  }
}

// FIXME: need to be implemented for tests.
void MessagePumpForUIEfl::Run(MessagePump::Delegate* delegate) {
  DCHECK(keep_running_) << "Quit must have been called outside of Run!";

  bool previous_keep_running = keep_running_;
  keep_running_ = true;

  for (;;) {
    bool did_work = delegate->DoWork();
    if (!keep_running_)
      break;

    TimeTicks next_time;
    did_work |= delegate->DoDelayedWork(&next_time);
    if (!keep_running_)
      break;

    if (did_work)
      continue;

    did_work = delegate->DoIdleWork();
    if (!keep_running_)
      break;
  }

  keep_running_ = previous_keep_running;
}

// FIXME: need to be implemented for tests.
void MessagePumpForUIEfl::Quit() {
  // RunLoop must be destroyed before chromium cleanup
  ecore_pipe_del(pipe_);
  if (schedule_delayed_work_timer_)
    ecore_timer_del(schedule_delayed_work_timer_);
  if (run_loop_) {
    DCHECK(run_loop_->running());
    run_loop_->AfterRun();
    delete run_loop_;
    run_loop_ = nullptr;
  }
  pipe_ = nullptr;
  schedule_delayed_work_timer_ = nullptr;
  keep_running_ = false;
}

void MessagePumpForUIEfl::ScheduleWork() {
  {
    AutoLock locker(schedule_work_lock_);
    if (work_scheduled_)
      return;
    work_scheduled_ = true;
  }

  DCHECK(pipe_);
  bool ok =
      ecore_pipe_write(pipe_, dummy_pipe_message, dummy_pipe_message_size);
  DCHECK(ok);
}

void MessagePumpForUIEfl::ScheduleDelayedWork(
    const TimeTicks& delayed_work_time) {
  CommandLine* cmdline = CommandLine::ForCurrentProcess();
  TimeTicks now = TimeTicks::Now();
  double delay = 0;
  if (delayed_work_time > now)
    delay = TimeDelta(delayed_work_time - now).InSecondsF();

  if (!cmdline->HasSwitch(
          switches::kLimitMemoryAllocationInScheduleDelayedWork)) {
    /* LCOV_EXCL_START */
    TimerPair* new_pair = new TimerPair();
    new_pair->first = this;
    new_pair->second = ecore_timer_add(delay, &TimerCallback, new_pair);
    pending_timers_.insert(new_pair);
    /* LCOV_EXCL_STOP */
  } else {
    if (schedule_delayed_work_timer_)
      ecore_timer_del(schedule_delayed_work_timer_);

    schedule_delayed_work_timer_ = ecore_timer_add(delay, &TimerCallback, this);
  }
}

void MessagePumpForUIEfl::PipeCallback(void* data, void*, unsigned int) {
  static_cast<MessagePumpForUIEfl*>(data)->DoWork();
}

Eina_Bool MessagePumpForUIEfl::TimerCallback(void* data) {
  CommandLine* cmdline = CommandLine::ForCurrentProcess();
  if (!cmdline->HasSwitch(
          switches::kLimitMemoryAllocationInScheduleDelayedWork)) {
    /* LCOV_EXCL_START */
    TimerPair* current_timer_pair = static_cast<TimerPair*>(data);
    current_timer_pair->first->DoDelayedWork();
    current_timer_pair->first->pending_timers_.erase(current_timer_pair);
    delete current_timer_pair;
    /* LCOV_EXCL_STOP */
  } else {
    MessagePumpForUIEfl* message_pump_for_ui_efl =
        static_cast<MessagePumpForUIEfl*>(data);
    message_pump_for_ui_efl->schedule_delayed_work_timer_ = nullptr;
    message_pump_for_ui_efl->DoDelayedWork();
  }

  return ECORE_CALLBACK_CANCEL;
}

void MessagePumpForUIEfl::DoWork() {
  {
    AutoLock locker(schedule_work_lock_);
    DCHECK(work_scheduled_);
    work_scheduled_ = false;
  }

  Delegate* delegate = MessageLoopForUI::current();
  if (!run_loop_) {
    run_loop_ = new RunLoop();
    bool result = run_loop_->BeforeRun();
    DCHECK(result);
  }

  bool more_work_is_plausible = delegate->DoWork();

  TimeTicks delayed_work_time;
  more_work_is_plausible |= delegate->DoDelayedWork(&delayed_work_time);

  if (more_work_is_plausible) {
    ScheduleWork();
    return;
  }

  more_work_is_plausible |= delegate->DoIdleWork();
  if (more_work_is_plausible) {
    ScheduleWork();  // LCOV_EXCL_LINE
    return;          // LCOV_EXCL_LINE
  }

  if (!delayed_work_time.is_null())
    ScheduleDelayedWork(delayed_work_time);
}

void MessagePumpForUIEfl::DoDelayedWork() {
  TimeTicks next_delayed_work_time;
  static_cast<Delegate*>(MessageLoopForUI::current())->
      DoDelayedWork(&next_delayed_work_time);

  if (!next_delayed_work_time.is_null())
    ScheduleDelayedWork(next_delayed_work_time);
}

}
