/*
 * Copyright (c) 2015 Samsung Electronics, Visual Display Division.
 * All Rights Reserved.
 *
 * @author  Michal Jurkiewicz <m.jurkiewicz@samsung.com>
 */

#include "ppapi/cpp/samsung/compositor_layer_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <>
const char* interface_name<PPB_CompositorLayer_Samsung_0_1>() {
  return PPB_COMPOSITORLAYER_SAMSUNG_INTERFACE_0_1;
}

}  // namespace

CompositorLayerSamsung::CompositorLayerSamsung() {}

CompositorLayerSamsung::CompositorLayerSamsung(
    const CompositorLayerSamsung& other)
    : CompositorLayer(other) {}

CompositorLayerSamsung::CompositorLayerSamsung(const CompositorLayer& other)
    : CompositorLayer(other) {}

CompositorLayerSamsung::CompositorLayerSamsung(const Resource& resource)
    : CompositorLayer(resource) {
  PP_DCHECK(IsCompositorLayerSamsung(resource));
}

CompositorLayerSamsung::CompositorLayerSamsung(PassRef, PP_Resource resource)
    : CompositorLayer(PASS_REF, resource) {}

CompositorLayerSamsung::~CompositorLayerSamsung() {}

int32_t CompositorLayerSamsung::SetBackgroundPlane(const Size& size) {
  if (has_interface<PPB_CompositorLayer_Samsung_0_1>()) {
    return get_interface<PPB_CompositorLayer_Samsung_0_1>()->SetBackgroundPlane(
        pp_resource(), &size.pp_size());
  }
  return PP_ERROR_NOINTERFACE;
}

bool CompositorLayerSamsung::IsCompositorLayerSamsung(
    const Resource& resource) {
  if (has_interface<PPB_CompositorLayer_Samsung_0_1>()) {
    return PP_ToBool(get_interface<PPB_CompositorLayer_Samsung_0_1>()
                         ->IsCompositorLayerSamsung(resource.pp_resource()));
  }
  return false;
}

}  // namespace pp
