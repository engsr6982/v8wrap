#pragma once
#include "JsValue.hpp"
#include <v8-context.h>
#include <v8-isolate.h>
#include <v8-locker.h>
#include <v8.h>


namespace v8wrap {

class JsRuntime;

class JsRuntimeScope {
    v8::Locker         mLocker;
    v8::Isolate::Scope mIsolateScope;
    v8::HandleScope    mHandleScope;
    v8::Context::Scope mContextScope;

public:
    explicit JsRuntimeScope(JsRuntime const& runtime);
    ~JsRuntimeScope() = default;
};

class ExitJsRuntimeScope {
    v8::Unlocker mUnlocker;

public:
    explicit ExitJsRuntimeScope(JsRuntime const& runtime);
    ~ExitJsRuntimeScope() = default;
};


namespace internal {

class V8EscapeScope {
    v8::EscapableHandleScope mHandleScope;

public:
    explicit V8EscapeScope(JsRuntime const& runtime);
    ~V8EscapeScope() = default;
};

} // namespace internal

} // namespace v8wrap