// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/cpp/samsung/url_data_source_samsung.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/module_impl.h"

namespace pp {

namespace {

template <> const char* interface_name<PPB_URLDataSource_Samsung_1_0>() {
  return PPB_URLDATASOURCE_SAMSUNG_INTERFACE;
}

}  // namespace

URLDataSource_Samsung::URLDataSource_Samsung(const InstanceHandle& instance,
                                             const std::string& url) {
  if (has_interface<PPB_URLDataSource_Samsung_1_0>()) {
    PassRefFromConstructor(get_interface<PPB_URLDataSource_Samsung_1_0>()->
        Create(instance.pp_instance(), url.c_str()));
  }
}

URLDataSource_Samsung::URLDataSource_Samsung(const URLDataSource_Samsung& other)
    : MediaDataSource_Samsung(other) {
}

URLDataSource_Samsung::URLDataSource_Samsung(PP_Resource resource)
    : MediaDataSource_Samsung(resource) {
}

URLDataSource_Samsung::URLDataSource_Samsung(PassRef, PP_Resource resource)
    : MediaDataSource_Samsung(PASS_REF, resource) {
}

URLDataSource_Samsung& URLDataSource_Samsung::operator=(
    const URLDataSource_Samsung& other) {
  MediaDataSource_Samsung::operator=(other);
  return *this;
}

URLDataSource_Samsung::~URLDataSource_Samsung() {
}

int32_t URLDataSource_Samsung::GetStreamingProperty(
    PP_StreamingProperty type,
    const CompletionCallbackWithOutput<pp::Var>& callback) {
  if (has_interface<PPB_URLDataSource_Samsung_1_0>()) {
    return get_interface<PPB_URLDataSource_Samsung_1_0>()->GetStreamingProperty(
        pp_resource(), type,
        callback.output(), callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

int32_t URLDataSource_Samsung::SetStreamingProperty(
    PP_StreamingProperty type,
    const Var& value,
    const CompletionCallback& callback) {
  if (has_interface<PPB_URLDataSource_Samsung_1_0>()) {
    return get_interface<PPB_URLDataSource_Samsung_1_0>()->SetStreamingProperty(
        pp_resource(), type, value.pp_var(),
        callback.pp_completion_callback());
  }

  return callback.MayForce(PP_ERROR_NOINTERFACE);
}

}  // namespace pp
