// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MESSAGE_PUMP_FOR_UI_EFL
#define MESSAGE_PUMP_FOR_UI_EFL

#include "base/message_loop/message_pump.h"
#include "base/synchronization/lock.h"
#include <Ecore.h>
#include <unordered_set>

namespace base {

class RunLoop;
class TimeTicks;
// TODO(g.czajkowski): two lines below are needless for limit-memory-allocation-in-schedule-delayed-work.
class MessagePumpForUIEfl;
typedef std::pair<MessagePumpForUIEfl*, Ecore_Timer*> TimerPair;

class BASE_EXPORT MessagePumpForUIEfl : public base::MessagePump {
 public:
  MessagePumpForUIEfl();
  ~MessagePumpForUIEfl() override;

  void Run(Delegate *) override;
  void Quit() override;
  void ScheduleWork() override;
  void ScheduleDelayedWork(const base::TimeTicks&) override;

 private:
  static void PipeCallback(void*, void*, unsigned int);
  static Eina_Bool TimerCallback(void*);
  void DoWork();
  void DoDelayedWork();

  // TODO(g.czajkowski): needless if limit-memory-allocation-in-schedule-delayed-work gets accepted.
  std::unordered_set<TimerPair*> pending_timers_;
  Ecore_Pipe* pipe_;
  Ecore_Timer* schedule_delayed_work_timer_;
  RunLoop* run_loop_;
  base::Lock schedule_work_lock_;
  bool keep_running_;
  bool work_scheduled_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpForUIEfl);
};

}

#endif
