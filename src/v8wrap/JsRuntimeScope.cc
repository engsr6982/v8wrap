#include "v8wrap/JsRuntimeScope.h"
#include "v8wrap/JsRuntime.h"
#include <stdexcept>

namespace v8wrap {

thread_local JsRuntimeScope* JsRuntimeScope::gCurrentScope = nullptr;

JsRuntimeScope::JsRuntimeScope(JsRuntime& runtime) : JsRuntimeScope(&runtime) {}
JsRuntimeScope::JsRuntimeScope(JsRuntime* runtime)
: mRuntime(runtime),
  mPrev(gCurrentScope),
  mLocker(runtime->mIsolate),
  mIsolateScope(runtime->mIsolate),
  mHandleScope(runtime->mIsolate),
  mContextScope(runtime->mContext.Get(runtime->mIsolate)) {
    gCurrentScope = this;
}

JsRuntimeScope::~JsRuntimeScope() { gCurrentScope = mPrev; }

JsRuntime* JsRuntimeScope::currentRuntime() {
    if (gCurrentScope) {
        return const_cast<JsRuntime*>(gCurrentScope->mRuntime);
    }
    return nullptr;
}

JsRuntime& JsRuntimeScope::currentRuntimeChecked() {
    auto current = currentRuntime();
    if (current == nullptr) {
        throw std::logic_error("No JsRuntimeScope active");
    }
    return *current;
}

std::tuple<v8::Isolate*, v8::Local<v8::Context>> JsRuntimeScope::currentIsolateAndContextChecked() {
    auto& current = currentRuntimeChecked();
    return std::make_tuple(current.mIsolate, current.mContext.Get(current.mIsolate));
}

v8::Isolate*           JsRuntimeScope::currentRuntimeIsolateChecked() { return currentRuntimeChecked().mIsolate; }
v8::Local<v8::Context> JsRuntimeScope::currentRuntimeContextChecked() {
    auto& current = currentRuntimeChecked();
    return current.mContext.Get(current.mIsolate);
}


ExitJsRuntimeScope::ExitJsRuntimeScope() : mUnlocker(JsRuntimeScope::currentRuntimeChecked().mIsolate) {}

namespace internal {

V8EscapeScope::V8EscapeScope() : mHandleScope(JsRuntimeScope::currentRuntimeChecked().mIsolate) {}
V8EscapeScope::V8EscapeScope(v8::Isolate* isolate) : mHandleScope(isolate) {}

} // namespace internal


} // namespace v8wrap