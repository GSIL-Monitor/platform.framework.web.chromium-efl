#ifndef TOMBSTONE_H_
#define TOMBSTONE_H_

// NOTE
// current TombStone is built in seperated SO file from libbrowser.so.
// That's because AOSP Backtrace can be built with NDK 21, but libbrowser.so is
// using NDK 16. So Backtrace or Unwind AOSP headers must not be exposed here
// if client of TombStone use those header files with differenct api_level NDK,
// the structures of Backtrace can be compiled diffently from our prebuilt
// Tombsonte SO. Therefore, Backtrace funtions must be used inside Tombstone SO.

#include <signal.h>

class TombStone {
 public:
  // depending on these parameter,
  // Case1. callstack unwinded from sighandler
  //        siginfo, ucontext must be specified from sighandler args of
  //            sigaction
  //        tid must be current runnging thread
  //        print callstack at the time of crash
  //        this is used in crash case
  // Case2. current running callstack
  //        siginfo, ucontext must be NULL
  //        print callstack which is currently running
  //        this is used in ANR or RenderUnresponsive case
  int signal_;
  siginfo_t* siginfo_;
  void* ucontext_;
  int timeout_;

  /////////////////////////////////////////////
  // print infomation filter
  /////////////////////////////////////////////

  // filters for siblings
  // dump siblings threads 's backtrace and stack
  // even though this vaule is false, siblings threads name will be printed
  bool dump_siblings_backtrace_;

  bool dump_memory_map_;

  TombStone();
  TombStone(int pid, int tid);

  std::string engrave();
  pid_t GetPid() { return pid_; }
  void SetPid(pid_t pid) { pid_ = pid; }
  pid_t GetTid() { return tid_; }

 private:
  // Support only TOMBSTONE_CURRENT_PID because browser does not have
  // permission to sandbox for ptrace call
  pid_t pid_;

  // thread id which you want to print information
  // if you just want to pring current running thread, use TOMBSTONE_CURRENT_TID
  pid_t tid_;

  int get_si_code() { return (siginfo_ != NULL) ? (siginfo_->si_code) : 0; }
  void* get_si_addr() { return (siginfo_ != NULL) ? (siginfo_->si_addr) : 0; }
};

#endif  // TOMBSTONE_H_
