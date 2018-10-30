/* Copyright (c) 2015 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppb_compositor_layer_samsung.idl,
 *   modified Tue Nov 24 10:25:59 2015.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_COMPOSITOR_LAYER_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_COMPOSITOR_LAYER_SAMSUNG_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_COMPOSITORLAYER_SAMSUNG_INTERFACE_0_1 \
  "PPB_CompositorLayer_Samsung;0.1"
#define PPB_COMPOSITORLAYER_SAMSUNG_INTERFACE \
  PPB_COMPOSITORLAYER_SAMSUNG_INTERFACE_0_1

/**
 * @file
 * This file defines interface <code>PPB_CompositorLayer_Samsung</code>
 * that extends <code>PPB_CompositorLayer</code> interface with possibility
 * to create Samsung specific layers.
 */

/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * It is extension for <code>PPB_CompositorLayer</code> interface,
 * which allows creating Samsung specific layers.
 * To create <code>PPB_CompositorLayer_Samsung</code> resource please use
 * <code>AddLayer</code> method from <code>PPB_Compositor</code> interface.
 */
struct PPB_CompositorLayer_Samsung_0_1 {
  /**
   * Determines if a resource is a compositor layer samsung resource.
   *
   * @param[in] resource The <code>PP_Resource</code> to test.
   *
   * @return A <code>PP_Bool</code> with <code>PP_TRUE</code> if the given
   * resource is a compositor layer resource or <code>PP_FALSE</code>
   * otherwise.
   */
  PP_Bool (*IsCompositorLayerSamsung)(PP_Resource resource);
  /**
   * Sets the background plane layer,
   * which allows to present background plane content.
   * If the layer is uninitialized, it will initialize the layer first,
   * and then set the background plane.
   * If the layer has been initialized to another kind of layer, the layer will
   * not be changed, and <code>PP_ERROR_BADARGUMENT</code> will be returned.
   * Position of layer can be controlled using
   * <code>SetClipRect</code> method from <code>PPB_CompositorLayer</code>.
   *
   * param[in] layer A <code>PP_Resource</code> corresponding to a compositor
   * layer resource.
   * param[in] size A <code>PP_Size</code> for the size of the layer before
   * transform.
   *
   * @return An int32_t containing a result code from <code>pp_errors.h</code>.
   */
  int32_t (*SetBackgroundPlane)(PP_Resource layer, const struct PP_Size* size);
};

typedef struct PPB_CompositorLayer_Samsung_0_1 PPB_CompositorLayer_Samsung;
/**
 * @}
 */

#endif /* PPAPI_C_SAMSUNG_PPB_COMPOSITOR_LAYER_SAMSUNG_H_ */
