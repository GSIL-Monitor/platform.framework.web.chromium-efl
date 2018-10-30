/*
 * Copyright (C) 2015 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EDGE_EFFECT_H
#define EDGE_EFFECT_H

#include <Evas.h>
#include <string>

#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT EdgeEffect {
 public:
  enum Direction { TOP = 1, BOTTOM, LEFT, RIGHT };

  EdgeEffect(Evas_Object* view);
  ~EdgeEffect();

  static void EnableGlobally(bool enable);

  void ShowOrHide(const Direction direction, bool show);
  void Hide();
  void Enable();
  void Disable();

  void ShowObject();
  void HideObject();

  void UpdatePosition(int top_height, int bottom_height);

 private:
  const std::string GetSourceString(const Direction direction);

  Evas_Object* parent_view_;
  Evas_Object* edge_object_ = nullptr;

  // Flag is set when edge effect is enabled.
  bool enabled_ = false;
  bool visible_[RIGHT + 1] = {false};
};

} // namespace content


#endif // EDGE_EFFECT_H
