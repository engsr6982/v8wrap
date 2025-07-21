#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsRuntime.hpp"
#include "v8wrap/JsValue.hpp"


namespace v8wrap {

JsRuntimeScope::JsRuntimeScope(JsRuntime const& runtime)
: mLocker(runtime.mIsolate),
  mIsolateScope(runtime.mIsolate),
  mHandleScope(runtime.mIsolate),
  mContextScope(runtime.mContext.Get(runtime.mIsolate)) {}

ExitJsRuntimeScope::ExitJsRuntimeScope(JsRuntime const& runtime) : mUnlocker(runtime.mIsolate) {}


namespace internal {

V8EscapeScope::V8EscapeScope(JsRuntime const& runtime) : mHandleScope(runtime.mIsolate) {}

} // namespace internal


} // namespace v8wrap