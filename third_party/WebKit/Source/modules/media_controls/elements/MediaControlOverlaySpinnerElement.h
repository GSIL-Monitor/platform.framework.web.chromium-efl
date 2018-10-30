#ifndef MediaControlOverlaySpinnerElement_h
#define MediaControlOverlaySpinnerElement_h

#include "modules/media_controls/elements/MediaControlDivElement.h"

namespace blink {

class MediaControlsImpl;

class MediaControlOverlaySpinnerElement : public MediaControlDivElement {
 public:
  explicit MediaControlOverlaySpinnerElement(MediaControlsImpl&);

  void UpdateDisplayType();
};

}  // namespace blink

#endif  // MediaControlOverlaySpinnerElement_h
