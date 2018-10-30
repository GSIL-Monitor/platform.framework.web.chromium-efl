// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_pump_ecore.h"

#include "base/logging.h"

namespace base {

struct MessagePumpEcore::RunState {
  Delegate* delegate;
  bool should_quit;
  int run_depth;
};

MessagePumpEcore::MessagePumpEcore()
    : state_(nullptr),
      wakeup_pipe_(nullptr),
      delayed_work_timer_(nullptr),
      ecore_event_handler_(nullptr) {
  int ret = ecore_init();
  CHECK_GT(ret, 0);
  wakeup_pipe_ = ecore_pipe_add(&OnWakeup, this);
}

MessagePumpEcore::~MessagePumpEcore() {
  DCHECK(!state_);
  if (ecore_event_handler_) {
    ecore_event_handler_del(ecore_event_handler_);
    ecore_event_handler_ = nullptr;
  }
  if (wakeup_pipe_) {
    ecore_pipe_del(wakeup_pipe_);
    wakeup_pipe_ = nullptr;
  }

  if (delayed_work_timer_) {
    ecore_timer_del(delayed_work_timer_);
    delayed_work_timer_ = nullptr;
  }

  ecore_shutdown();
}

void MessagePumpEcore::Run(Delegate* delegate) {
  RunState state;
  state.delegate = delegate;
  state.should_quit = false;
  state.run_depth = state_ ? state_->run_depth + 1 : 1;

  RunState* previous_state = state_;
  state_ = &state;

  if (!ecore_event_handler_)
    ecore_event_handler_ = ecore_event_handler_add(
        ECORE_EVENT_SIGNAL_EXIT, MessagePumpEcore::OnEcoreEvent,
        static_cast<void*>(this));

  for (;;) {
    bool more_work_is_plausible = ecore_main_loop_iterate_may_block(EINA_FALSE);
    if (state_->should_quit)
      break;

    more_work_is_plausible |= state_->delegate->DoWork();
    if (state_->should_quit)
      break;

    more_work_is_plausible |=
        state_->delegate->DoDelayedWork(&delayed_work_time_);
    if (state_->should_quit)
      break;

    if (more_work_is_plausible)
      continue;

    more_work_is_plausible = state_->delegate->DoIdleWork();
    if (state_->should_quit)
      break;

    if (more_work_is_plausible)
      continue;

    if (delayed_work_time_.is_null()) {
      ecore_main_loop_iterate_may_block(EINA_TRUE);
    } else {
      TimeDelta delay = delayed_work_time_ - TimeTicks::Now();
      if (delay > TimeDelta()) {
        delayed_work_timer_ =
            ecore_timer_add(delay.InSecondsF(), &OnTimerFired, this);
        ecore_main_loop_iterate_may_block(EINA_TRUE);
        if (delayed_work_timer_) {
          ecore_timer_del(delayed_work_timer_);
          delayed_work_timer_ = nullptr;
        }
      } else {
        // It looks like delayed_work_time_ indicates a time in the past, so we
        // need to call DoDelayedWork now.
        delayed_work_time_ = TimeTicks();
      }
    }

    if (state_->should_quit)
      break;
  }

  state_ = previous_state;
}

void MessagePumpEcore::Quit() {
  if (state_) {
    state_->should_quit = true;
    ScheduleWork();
  } else {
    NOTREACHED() << "Quit called outside Run!";
  }
}

void MessagePumpEcore::ScheduleWork() {
  if (wakeup_pipe_ == nullptr)
    return;
  AutoLock lock(wakeup_pipe_lock_);
  char msg = 'W';
  if (!ecore_pipe_write(wakeup_pipe_, &msg, 1)) {
    NOTREACHED() << "Could not write to the message loop wakeup pipe!";
  }
}

void MessagePumpEcore::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
  delayed_work_time_ = delayed_work_time;
}

Eina_Bool MessagePumpEcore::OnEcoreEvent(void* data, int ev_type, void* ev) {
  auto message_pump_ecore_instance = static_cast<MessagePumpEcore*>(data);
  if (message_pump_ecore_instance) {
    if (message_pump_ecore_instance->ecore_event_handler_) {
      ecore_event_handler_del(
          message_pump_ecore_instance->ecore_event_handler_);
      message_pump_ecore_instance->ecore_event_handler_ = nullptr;
    }
    message_pump_ecore_instance->Quit();
  }
  return ECORE_CALLBACK_PASS_ON;
}
// static
void MessagePumpEcore::OnWakeup(void* data, void* buffer, unsigned int nbyte) {}

// static
Eina_Bool MessagePumpEcore::OnTimerFired(void* data) {
  static_cast<MessagePumpEcore*>(data)->delayed_work_timer_ = nullptr;
  return ECORE_CALLBACK_CANCEL;
}

} // namespace base
