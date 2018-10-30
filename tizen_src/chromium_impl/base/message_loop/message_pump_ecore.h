// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MESSAGE_LOOP_MESSAGE_PUMP_ECORE_H_
#define BASE_MESSAGE_LOOP_MESSAGE_PUMP_ECORE_H_

#include "base/base_export.h"
#include "base/macros.h"
#include "base/message_loop/message_pump.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"

#include <Ecore.h>

namespace base {

class BASE_EXPORT MessagePumpEcore : public MessagePump {
 public:
  MessagePumpEcore();
  ~MessagePumpEcore() override;

  void Run(Delegate* delegate) override;
  void Quit() override;
  void ScheduleWork() override;
  void ScheduleDelayedWork(const TimeTicks& delayed_work_time) override;

 private:
  static void OnWakeup(void* data, void* buffer, unsigned int nbyte);
  static Eina_Bool OnTimerFired(void* data);
  static Eina_Bool OnEcoreEvent(void* data, int ev_type, void* ev);
  // We may make recursive calls to Run, so we save state that needs to be
  // separate between them in this structure type.
  struct RunState;
  RunState* state_;

  // We use a wakeup pipe to make sure we'll get out of the ecore mainloop
  // blocking phase when another thread has scheduled us to do some work.
  Ecore_Pipe* wakeup_pipe_;

  // The lock that protects access to wakeup pipe.
  base::Lock wakeup_pipe_lock_;

  // This is the time when we need to do delayed work.
  TimeTicks delayed_work_time_;

  // This is the timer that is needed to get out of ecore mainloop blocking
  // phase after delay.
  Ecore_Timer* delayed_work_timer_;

  Ecore_Event_Handler* ecore_event_handler_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpEcore);
};

}  // namespace base

#endif // BASE_MESSAGE_LOOP_MESSAGE_PUMP_ECORE_H_
