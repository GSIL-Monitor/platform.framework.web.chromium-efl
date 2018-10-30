// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InbandCue_h
#define InbandCue_h

#include "core/html/track/TextTrackCue.h"
#include "core/typed_arrays/DOMArrayBuffer.h"

namespace blink {

class InbandCue final : public TextTrackCue {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static InbandCue* Create(double start, double end, const std::string& data) {
    DOMArrayBuffer* arrayBuffer =
        DOMArrayBuffer::Create(data.c_str(), data.length());
    return new InbandCue(start, end, arrayBuffer);
  }

  virtual ~InbandCue() override;

  DOMArrayBuffer* data() const { return data_.Get(); }
  void setData(DOMArrayBuffer* data);

  void UpdateDisplay(HTMLDivElement& container) override{};

  void UpdatePastAndFutureNodes(double movieTime) override{};

  void RemoveDisplayTree(RemovalNotification) override{};

  ExecutionContext* GetExecutionContext() const override { return nullptr; }

#ifndef NDEBUG
  String ToString() const override;
#endif

  DECLARE_VIRTUAL_TRACE();

 private:
  InbandCue(double start, double end, DOMArrayBuffer* array_buffer);
  Member<DOMArrayBuffer> data_;
};

}  // namespace blink

#endif  // InbandCue_h
