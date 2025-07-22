#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsRuntime.hpp"
#include <stdexcept>

namespace v8wrap {

thread_local JsRuntimeScope* JsRuntimeScope::gCurrentScope = nullptr;

JsRuntimeScope::JsRuntimeScope(JsRuntime const& runtime)
: mRuntime(&runtime),
  mPrev(gCurrentScope),
  mLocker(runtime.mIsolate),
  mIsolateScope(runtime.mIsolate),
  mHandleScope(runtime.mIsolate),
  mContextScope(runtime.mContext.Get(runtime.mIsolate)) {
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


ExitJsRuntimeScope::ExitJsRuntimeScope(JsRuntime const& runtime) : mUnlocker(runtime.mIsolate) {}
ExitJsRuntimeScope::~ExitJsRuntimeScope() = default;

namespace internal {

V8EscapeScope::V8EscapeScope(JsRuntime const& runtime) : mHandleScope(runtime.mIsolate) {}

} // namespace internal


} // namespace v8wrap