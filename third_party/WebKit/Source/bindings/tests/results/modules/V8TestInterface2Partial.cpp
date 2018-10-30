// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/WebKit/Source/bindings/templates/partial_interface.cpp.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#include "V8TestInterface2Partial.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/IDLTypes.h"
#include "bindings/core/v8/NativeValueTraitsImpl.h"
#include "bindings/core/v8/V8DOMConfiguration.h"
#include "bindings/core/v8/V8TestInterface2.h"
#include "bindings/tests/idls/modules/TestInterface2Partial.h"
#include "bindings/tests/idls/modules/TestInterface2Partial2.h"
#include "core/dom/ExecutionContext.h"
#include "platform/bindings/RuntimeCallStats.h"
#include "platform/bindings/V8ObjectConstructor.h"
#include "platform/runtime_enabled_features.h"
#include "platform/wtf/GetPtr.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

namespace TestInterface2PartialV8Internal {

static void voidMethodPartial1Method(const v8::FunctionCallbackInfo<v8::Value>& info) {
  TestInterface2* impl = V8TestInterface2::ToImpl(info.Holder());

  if (UNLIKELY(info.Length() < 1)) {
    V8ThrowException::ThrowTypeError(info.GetIsolate(), ExceptionMessages::FailedToExecute("voidMethodPartial1", "TestInterface2", ExceptionMessages::NotEnoughArguments(1, info.Length())));
    return;
  }

  V8StringResource<> value;
  value = info[0];
  if (!value.Prepare())
    return;

  TestInterface2Partial::voidMethodPartial1(*impl, value);
}

static void voidMethodPartial2Method(const v8::FunctionCallbackInfo<v8::Value>& info) {
  TestInterface2* impl = V8TestInterface2::ToImpl(info.Holder());

  if (UNLIKELY(info.Length() < 1)) {
    V8ThrowException::ThrowTypeError(info.GetIsolate(), ExceptionMessages::FailedToExecute("voidMethodPartial2", "TestInterface2", ExceptionMessages::NotEnoughArguments(1, info.Length())));
    return;
  }

  V8StringResource<> value;
  value = info[0];
  if (!value.Prepare())
    return;

  TestInterface2Partial2::voidMethodPartial2(*impl, value);
}

} // namespace TestInterface2PartialV8Internal

void V8TestInterface2Partial::voidMethodPartial1MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(info.GetIsolate(), "Blink_TestInterface2_voidMethodPartial1");

  TestInterface2PartialV8Internal::voidMethodPartial1Method(info);
}

void V8TestInterface2Partial::voidMethodPartial2MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  RUNTIME_CALL_TIMER_SCOPE_DISABLED_BY_DEFAULT(info.GetIsolate(), "Blink_TestInterface2_voidMethodPartial2");

  TestInterface2PartialV8Internal::voidMethodPartial2Method(info);
}

static const V8DOMConfiguration::MethodConfiguration V8TestInterface2Methods[] = {
    {"voidMethodPartial2", V8TestInterface2Partial::voidMethodPartial2MethodCallback, 1, v8::None, V8DOMConfiguration::kOnPrototype, V8DOMConfiguration::kCheckHolder, V8DOMConfiguration::kDoNotCheckAccess, V8DOMConfiguration::kAllWorlds},
};

void V8TestInterface2Partial::installV8TestInterface2Template(
    v8::Isolate* isolate,
    const DOMWrapperWorld& world,
    v8::Local<v8::FunctionTemplate> interfaceTemplate) {
  // Initialize the interface object's template.
  V8TestInterface2::installV8TestInterface2Template(isolate, world, interfaceTemplate);

  v8::Local<v8::Signature> signature = v8::Signature::New(isolate, interfaceTemplate);
  ALLOW_UNUSED_LOCAL(signature);
  v8::Local<v8::ObjectTemplate> instanceTemplate = interfaceTemplate->InstanceTemplate();
  ALLOW_UNUSED_LOCAL(instanceTemplate);
  v8::Local<v8::ObjectTemplate> prototypeTemplate = interfaceTemplate->PrototypeTemplate();
  ALLOW_UNUSED_LOCAL(prototypeTemplate);

  // Register IDL constants, attributes and operations.
  V8DOMConfiguration::InstallMethods(
      isolate, world, instanceTemplate, prototypeTemplate, interfaceTemplate,
      signature, V8TestInterface2Methods, WTF_ARRAY_LENGTH(V8TestInterface2Methods));

  // Custom signature

  V8TestInterface2Partial::InstallRuntimeEnabledFeaturesOnTemplate(
      isolate, world, interfaceTemplate);
}

void V8TestInterface2Partial::InstallRuntimeEnabledFeaturesOnTemplate(
    v8::Isolate* isolate,
    const DOMWrapperWorld& world,
    v8::Local<v8::FunctionTemplate> interface_template) {
  V8TestInterface2::InstallRuntimeEnabledFeaturesOnTemplate(isolate, world, interface_template);

  v8::Local<v8::Signature> signature = v8::Signature::New(isolate, interface_template);
  ALLOW_UNUSED_LOCAL(signature);
  v8::Local<v8::ObjectTemplate> instance_template = interface_template->InstanceTemplate();
  ALLOW_UNUSED_LOCAL(instance_template);
  v8::Local<v8::ObjectTemplate> prototype_template = interface_template->PrototypeTemplate();
  ALLOW_UNUSED_LOCAL(prototype_template);

  // Register IDL constants, attributes and operations.

  // Custom signature
  if (RuntimeEnabledFeatures::Interface2PartialFeatureNameEnabled()) {
    const V8DOMConfiguration::MethodConfiguration voidMethodPartial1MethodConfiguration[] = {
      {"voidMethodPartial1", V8TestInterface2Partial::voidMethodPartial1MethodCallback, 1, v8::None, V8DOMConfiguration::kOnPrototype, V8DOMConfiguration::kCheckHolder, V8DOMConfiguration::kDoNotCheckAccess, V8DOMConfiguration::kAllWorlds}
    };
    for (const auto& methodConfig : voidMethodPartial1MethodConfiguration)
      V8DOMConfiguration::InstallMethod(isolate, world, instance_template, prototype_template, interface_template, signature, methodConfig);
  }
}

void V8TestInterface2Partial::initialize() {
  // Should be invoked from ModulesInitializer.
  V8TestInterface2::UpdateWrapperTypeInfo(
      &V8TestInterface2Partial::installV8TestInterface2Template,
      nullptr,
      &V8TestInterface2Partial::InstallRuntimeEnabledFeaturesOnTemplate,
      nullptr);
}

}  // namespace blink
