// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/LayoutView.h"

#include "core/layout/LayoutTestHelper.h"

namespace blink {

class LayoutViewTest : public RenderingTest {};

TEST_F(LayoutViewTest, UpdateCountersLayout) {
  SetBodyInnerHTML(
      "<style>"
      "  div.incX { counter-increment: x }"
      "  div.incY { counter-increment: y }"
      "  div::before { content: counter(y) }"
      "</style>"
      "<div id=inc></div>");

  GetDocument().View()->UpdateAllLifecyclePhases();
  Element* inc = GetDocument().getElementById("inc");

  inc->setAttribute("class", "incX");
  GetDocument().UpdateStyleAndLayoutTree();
  EXPECT_FALSE(GetDocument().View()->NeedsLayout());

  GetDocument().View()->UpdateAllLifecyclePhases();
  inc->setAttribute("class", "incY");
  GetDocument().UpdateStyleAndLayoutTree();
  EXPECT_TRUE(GetDocument().View()->NeedsLayout());
}

}  // namespace blink
