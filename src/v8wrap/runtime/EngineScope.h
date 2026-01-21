#pragma once
#include "v8wrap/Global.h"

V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-context.h>
#include <v8-isolate.h>
#include <v8-locker.h>
#include <v8.h>
V8_WRAP_WARNING_GUARD_END


namespace v8wrap {

class Engine;

class EngineScope final {
public:
    explicit EngineScope(Engine& runtime);
    explicit EngineScope(Engine* runtime);
    ~EngineScope();

    V8WRAP_DISALLOW_COPY_AND_MOVE(EngineScope);
    V8WRAP_DISALLOW_NEW();

    static Engine* currentRuntime();

    static Engine& currentRuntimeChecked();

    static std::tuple<v8::Isolate*, v8::Local<v8::Context>> currentIsolateAndContextChecked();

    static v8::Isolate* currentRuntimeIsolateChecked();

    static v8::Local<v8::Context> currentRuntimeContextChecked();

private:
    // 作用域链
    Engine const* mRuntime{nullptr};
    EngineScope*  mPrev{nullptr};

    // v8作用域
    v8::Locker         mLocker;
    v8::Isolate::Scope mIsolateScope;
    v8::HandleScope    mHandleScope;
    v8::Context::Scope mContextScope;

    static thread_local EngineScope* gCurrentScope;
};

class ExitEngineScope final {
    v8::Unlocker mUnlocker;

public:
    explicit ExitEngineScope();
    ~ExitEngineScope() = default;

    V8WRAP_DISALLOW_COPY_AND_MOVE(ExitEngineScope);
    V8WRAP_DISALLOW_NEW();
};


namespace internal {

class V8EscapeScope final {
    v8::EscapableHandleScope mHandleScope;

public:
    explicit V8EscapeScope();
    explicit V8EscapeScope(v8::Isolate* isolate);
    ~V8EscapeScope() = default;

    template <typename T>
    v8::Local<T> escape(v8::Local<T> value) {
        return mHandleScope.Escape(value);
    }
};

} // namespace internal

} // namespace v8wrap