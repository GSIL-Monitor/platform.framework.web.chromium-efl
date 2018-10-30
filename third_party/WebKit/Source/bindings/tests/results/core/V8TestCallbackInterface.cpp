// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/WebKit/Source/bindings/templates/callback_interface.cpp.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#include "V8TestCallbackInterface.h"

#include "bindings/core/v8/IDLTypes.h"
#include "bindings/core/v8/NativeValueTraitsImpl.h"
#include "bindings/core/v8/ScriptController.h"
#include "bindings/core/v8/V8BindingForCore.h"
#include "bindings/core/v8/V8TestInterfaceEmpty.h"
#include "core/dom/ExecutionContext.h"
#include "platform/wtf/Assertions.h"
#include "platform/wtf/GetPtr.h"
#include "platform/wtf/RefPtr.h"

namespace blink {

V8TestCallbackInterface::V8TestCallbackInterface(v8::Local<v8::Function> callback, ScriptState* scriptState)
    : script_state_(scriptState) {
  callback_.Set(scriptState->GetIsolate(), callback);
}

V8TestCallbackInterface::~V8TestCallbackInterface() {}

DEFINE_TRACE(V8TestCallbackInterface) {
  TestCallbackInterface::Trace(visitor);
}

void V8TestCallbackInterface::voidMethod() {
  if (!script_state_->ContextIsValid())
    return;
  ExecutionContext* executionContext =
      ExecutionContext::From(script_state_.get());
  DCHECK(!executionContext->IsContextSuspended());
  if (!executionContext || executionContext->IsContextDestroyed())
    return;

  ScriptState::Scope scope(script_state_.get());

  v8::Local<v8::Value> *argv = 0;

  v8::Isolate* isolate = script_state_->GetIsolate();
  V8ScriptRunner::CallFunction(callback_.NewLocal(isolate),
                               ExecutionContext::From(script_state_.get()),
                               v8::Undefined(isolate),
                               0,
                               argv,
                               isolate);
}

bool V8TestCallbackInterface::booleanMethod() {
  if (!script_state_->ContextIsValid())
    return true;
  ExecutionContext* executionContext =
      ExecutionContext::From(script_state_.get());
  DCHECK(!executionContext->IsContextSuspended());
  if (!executionContext || executionContext->IsContextDestroyed())
    return true;

  ScriptState::Scope scope(script_state_.get());

  v8::Local<v8::Value> *argv = 0;

  v8::Isolate* isolate = script_state_->GetIsolate();
  v8::TryCatch exceptionCatcher(isolate);
  exceptionCatcher.SetVerbose(true);
  V8ScriptRunner::CallFunction(callback_.NewLocal(isolate),
                               executionContext,
                               v8::Undefined(isolate),
                               0,
                               argv,
                               isolate);
  return !exceptionCatcher.HasCaught();
}

void V8TestCallbackInterface::voidMethodBooleanArg(bool boolArg) {
  if (!script_state_->ContextIsValid())
    return;
  ExecutionContext* executionContext =
      ExecutionContext::From(script_state_.get());
  DCHECK(!executionContext->IsContextSuspended());
  if (!executionContext || executionContext->IsContextDestroyed())
    return;

  ScriptState::Scope scope(script_state_.get());

  v8::Local<v8::Value> boolArgHandle = v8::Boolean::New(script_state_->GetIsolate(), boolArg);
  v8::Local<v8::Value> argv[] = { boolArgHandle };

  v8::Isolate* isolate = script_state_->GetIsolate();
  V8ScriptRunner::CallFunction(callback_.NewLocal(isolate),
                               ExecutionContext::From(script_state_.get()),
                               v8::Undefined(isolate),
                               1,
                               argv,
                               isolate);
}

void V8TestCallbackInterface::voidMethodSequenceArg(const HeapVector<Member<TestInterfaceEmpty>>& sequenceArg) {
  if (!script_state_->ContextIsValid())
    return;
  ExecutionContext* executionContext =
      ExecutionContext::From(script_state_.get());
  DCHECK(!executionContext->IsContextSuspended());
  if (!executionContext || executionContext->IsContextDestroyed())
    return;

  ScriptState::Scope scope(script_state_.get());

  v8::Local<v8::Value> sequenceArgHandle = ToV8(sequenceArg, script_state_->GetContext()->Global(), script_state_->GetIsolate());
  v8::Local<v8::Value> argv[] = { sequenceArgHandle };

  v8::Isolate* isolate = script_state_->GetIsolate();
  V8ScriptRunner::CallFunction(callback_.NewLocal(isolate),
                               ExecutionContext::From(script_state_.get()),
                               v8::Undefined(isolate),
                               1,
                               argv,
                               isolate);
}

void V8TestCallbackInterface::voidMethodFloatArg(float floatArg) {
  if (!script_state_->ContextIsValid())
    return;
  ExecutionContext* executionContext =
      ExecutionContext::From(script_state_.get());
  DCHECK(!executionContext->IsContextSuspended());
  if (!executionContext || executionContext->IsContextDestroyed())
    return;

  ScriptState::Scope scope(script_state_.get());

  v8::Local<v8::Value> floatArgHandle = v8::Number::New(script_state_->GetIsolate(), floatArg);
  v8::Local<v8::Value> argv[] = { floatArgHandle };

  v8::Isolate* isolate = script_state_->GetIsolate();
  V8ScriptRunner::CallFunction(callback_.NewLocal(isolate),
                               ExecutionContext::From(script_state_.get()),
                               v8::Undefined(isolate),
                               1,
                               argv,
                               isolate);
}

void V8TestCallbackInterface::voidMethodTestInterfaceEmptyArg(TestInterfaceEmpty* testInterfaceEmptyArg) {
  if (!script_state_->ContextIsValid())
    return;
  ExecutionContext* executionContext =
      ExecutionContext::From(script_state_.get());
  DCHECK(!executionContext->IsContextSuspended());
  if (!executionContext || executionContext->IsContextDestroyed())
    return;

  ScriptState::Scope scope(script_state_.get());

  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, script_state_->GetContext()->Global(), script_state_->GetIsolate());
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle };

  v8::Isolate* isolate = script_state_->GetIsolate();
  V8ScriptRunner::CallFunction(callback_.NewLocal(isolate),
                               ExecutionContext::From(script_state_.get()),
                               v8::Undefined(isolate),
                               1,
                               argv,
                               isolate);
}

void V8TestCallbackInterface::voidMethodTestInterfaceEmptyStringArg(TestInterfaceEmpty* testInterfaceEmptyArg, const String& stringArg) {
  if (!script_state_->ContextIsValid())
    return;
  ExecutionContext* executionContext =
      ExecutionContext::From(script_state_.get());
  DCHECK(!executionContext->IsContextSuspended());
  if (!executionContext || executionContext->IsContextDestroyed())
    return;

  ScriptState::Scope scope(script_state_.get());

  v8::Local<v8::Value> testInterfaceEmptyArgHandle = ToV8(testInterfaceEmptyArg, script_state_->GetContext()->Global(), script_state_->GetIsolate());
  v8::Local<v8::Value> stringArgHandle = V8String(script_state_->GetIsolate(), stringArg);
  v8::Local<v8::Value> argv[] = { testInterfaceEmptyArgHandle, stringArgHandle };

  v8::Isolate* isolate = script_state_->GetIsolate();
  V8ScriptRunner::CallFunction(callback_.NewLocal(isolate),
                               ExecutionContext::From(script_state_.get()),
                               v8::Undefined(isolate),
                               2,
                               argv,
                               isolate);
}

void V8TestCallbackInterface::callbackWithThisValueVoidMethodStringArg(ScriptValue thisValue, const String& stringArg) {
  if (!script_state_->ContextIsValid())
    return;
  ExecutionContext* executionContext =
      ExecutionContext::From(script_state_.get());
  DCHECK(!executionContext->IsContextSuspended());
  if (!executionContext || executionContext->IsContextDestroyed())
    return;

  ScriptState::Scope scope(script_state_.get());
  v8::Local<v8::Value> thisHandle = thisValue.V8Value();

  v8::Local<v8::Value> stringArgHandle = V8String(script_state_->GetIsolate(), stringArg);
  v8::Local<v8::Value> argv[] = { stringArgHandle };

  v8::Isolate* isolate = script_state_->GetIsolate();
  V8ScriptRunner::CallFunction(callback_.NewLocal(isolate),
                               ExecutionContext::From(script_state_.get()),
                               thisHandle,
                               1,
                               argv,
                               isolate);
}

}  // namespace blink
