// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HitTestCanvasResult_h
#define HitTestCanvasResult_h

#include "core/dom/Element.h"

namespace blink {

class CORE_EXPORT HitTestCanvasResult final
    : public GarbageCollectedFinalized<HitTestCanvasResult> {
 public:
  static HitTestCanvasResult* Create(String id, Member<Element> control) {
    return new HitTestCanvasResult(id, control);
  }

  String GetId() const;
  Element* GetControl() const;

  DECLARE_TRACE();

 private:
  HitTestCanvasResult(String id, Member<Element> control);

  String id_;
  Member<Element> control_;
};

}  // namespace blink

#endif  // HitTestCanvasResult_h
