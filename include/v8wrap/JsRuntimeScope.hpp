#pragma once
#include "Global.hpp"
#include <v8-context.h>
#include <v8-isolate.h>
#include <v8-locker.h>
#include <v8.h>


namespace v8wrap {

class JsRuntime;

class JsRuntimeScope final {
public:
    explicit JsRuntimeScope(JsRuntime& runtime);
    explicit JsRuntimeScope(JsRuntime* runtime);
    ~JsRuntimeScope();

    V8WRAP_DISALLOW_COPY_AND_MOVE(JsRuntimeScope);
    V8WRAP_DISALLOW_NEW();

    static JsRuntime* currentRuntime();

    static JsRuntime& currentRuntimeChecked();

    static std::tuple<v8::Isolate*, v8::Local<v8::Context>> currentIsolateAndContextChecked();

    static v8::Isolate* currentRuntimeIsolateChecked();

    static v8::Local<v8::Context> currentRuntimeContextChecked();

private:
    // 作用域链
    JsRuntime const* mRuntime{nullptr};
    JsRuntimeScope*  mPrev{nullptr};

    // v8作用域
    v8::Locker         mLocker;
    v8::Isolate::Scope mIsolateScope;
    v8::HandleScope    mHandleScope;
    v8::Context::Scope mContextScope;

    static thread_local JsRuntimeScope* gCurrentScope;
};

class ExitJsRuntimeScope final {
    v8::Unlocker mUnlocker;

public:
    explicit ExitJsRuntimeScope();
    ~ExitJsRuntimeScope() = default;

    V8WRAP_DISALLOW_COPY_AND_MOVE(ExitJsRuntimeScope);
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