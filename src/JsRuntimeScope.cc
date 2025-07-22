#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsRuntime.hpp"
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


ExitJsRuntimeScope::ExitJsRuntimeScope() : mUnlocker(JsRuntimeScope::currentRuntimeChecked().mIsolate) {}

namespace internal {

V8EscapeScope::V8EscapeScope() : mHandleScope(JsRuntimeScope::currentRuntimeChecked().mIsolate) {}

} // namespace internal


} // namespace v8wrap