// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <v8.h>

#define EXPORT __attribute__((__visibility__("default")))

namespace {
void Run(const char* source) {
  v8::Script::Compile(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), source))->Run();
}
} // namespace

extern "C" EXPORT unsigned int DynamicPluginVersion(void) {
  return 1;
}

extern "C" EXPORT void DynamicPluginStartSession(const char* tizne_app_id,
                               v8::Handle<v8::Context> context,
                               int routingHandle,
                               const char* baseURL) {
  Run("document.write(\"DynamicPluginStartSession\");");
}

extern "C" EXPORT void DynamicDatabaseAttach(int attach) {
}
