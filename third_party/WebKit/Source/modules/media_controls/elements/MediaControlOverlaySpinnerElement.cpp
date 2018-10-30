// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/media_controls/elements/MediaControlOverlaySpinnerElement.h"

#include "core/html/HTMLMediaElement.h"

namespace blink {
MediaControlOverlaySpinnerElement::MediaControlOverlaySpinnerElement(
    MediaControlsImpl& media_controls)
    : MediaControlDivElement(media_controls, kMediaOverlaySpinner) {
  SetShadowPseudoId(AtomicString("-webkit-media-controls-overlay-spinner"));
}

void MediaControlOverlaySpinnerElement::UpdateDisplayType() {
  SetIsWanted(MediaElement().seeking());
}

}  // namespace blink
