/*
 * Copyright (c) 2015 Samsung Electronics, Visual Display Division.
 * All Rights Reserved.
 *
 * @author  Michal Jurkiewicz <m.jurkiewicz@samsung.com>
 *
 * This source code is written basing on the Chromium source code.
 *
 * Copyright (c) 2014 The Chromium Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the Chromium-LICENSE file.
 */

#ifndef PPAPI_CPP_SAMSUNG_COMPOSITOR_LAYER_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_COMPOSITOR_LAYER_SAMSUNG_H_

#include "ppapi/c/samsung/ppb_compositor_layer_samsung.h"
#include "ppapi/cpp/compositor_layer.h"

namespace pp {

class CompositorLayerSamsung : public CompositorLayer {
 public:
  /** Default constructor for creating an is_null()
   * <code>CompositorLayerSamsung</code> object.
   */
  CompositorLayerSamsung();

  /** The copy constructor for <code>CompositorLayerSamsung</code>.
   *
   * @param[in] other A reference to a <code>CompositorLayerSamsung</code>.
   */
  CompositorLayerSamsung(const CompositorLayerSamsung& other);

  /** Constructs a <code>CompositorLayerSamsung</code> from
   * a <code>CompositorLayer</code>.
   *
   * @param[in] other A reference to a <code>CompositorLayer</code>.
   */
  explicit CompositorLayerSamsung(const CompositorLayer& other);

  /** Constructs a <code>CompositorLayer</code> from a <code>Resource</code>.
   *
   * @param[in] resource A <code>PPB_CompositorLayer</code> resource.
   */
  explicit CompositorLayerSamsung(const Resource& resource);

  /** A constructor used when you have received a <code>PP_Resource</code> as a
   * return value that has had 1 ref added for you.
   *
   * @param[in] resource A <code>PPB_CompositorLayer_Samsung</code> resource.
   */
  CompositorLayerSamsung(PassRef, PP_Resource resource);

  /** Destructor.
   */
  ~CompositorLayerSamsung();

  /**
   * Sets the background plane layer, which allows to present
   * background plane content.
   * If the layer is uninitialized, it will initialize the layer first,
   * and then set the background plane.
   * If the layer has been initialized to another kind of layer, the layer will
   * not be changed, and <code>PP_ERROR_BADARGUMENT</code> will be returned.
   * Position of layer can be controlled using <code>SetClipRect</code>
   * method from <code>PPB_CompositorLayer</code>.
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] size A <code>PP_Size</code> for the size of the layer before
   * transform.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t SetBackgroundPlane(const Size& size);

  /**
   * Checks whether a <code>Resource</code> is a compositor layer samsung,
   * to test whether it is appropriate for use with the
   * <code>CompositorLayerSamsung</code> constructor.
   *
   * @param[in] resource A <code>Resource</code> to test.
   *
   * @return True if <code>resource</code> is a compositor layer.
   */
  static bool IsCompositorLayerSamsung(const Resource& resource);
};

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_COMPOSITOR_LAYER_SAMSUNG_H_
