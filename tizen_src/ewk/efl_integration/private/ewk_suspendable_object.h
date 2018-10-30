// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EWK_SUSPENDABLE_OBJECT_H_
#define EWK_SUSPENDABLE_OBJECT_H_

#include <string>

class Ewk_Suspendable_Object {
 public:
  Ewk_Suspendable_Object()
      : decided_(false),
        suspended_(false) {}
  virtual ~Ewk_Suspendable_Object() { }

  bool SetDecision(bool allowed);
  void Ignore();
  bool Suspend();

  bool IsDecided() const { return decided_; }
  bool IsSuspended() const { return suspended_; }

 private:
  virtual void RunCallback(bool allowed) {}

  bool decided_;
  bool suspended_;
};

#endif /* EWK_SUSPENDABLE_OBJECT_H_ */
