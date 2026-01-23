#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/runtime/Engine.h"
#include <stdexcept>


namespace v8wrap {

thread_local EngineScope* EngineScope::gCurrentScope = nullptr;

EngineScope::EngineScope(Engine& runtime) : EngineScope(&runtime) {}
EngineScope::EngineScope(Engine* runtime)
: mRuntime(runtime),
  mPrev(gCurrentScope),
  mLocker(runtime->isolate_),
  mIsolateScope(runtime->isolate_),
  mHandleScope(runtime->isolate_),
  mContextScope(runtime->context_.Get(runtime->isolate_)) {
    gCurrentScope = this;
}

EngineScope::~EngineScope() { gCurrentScope = mPrev; }

Engine* EngineScope::currentRuntime() {
    if (gCurrentScope) {
        return const_cast<Engine*>(gCurrentScope->mRuntime);
    }
    return nullptr;
}

Engine& EngineScope::currentRuntimeChecked() {
    auto current = currentRuntime();
    if (current == nullptr) {
        throw std::logic_error("No EngineScope active");
    }
    return *current;
}

std::tuple<v8::Isolate*, v8::Local<v8::Context>> EngineScope::currentIsolateAndContextChecked() {
    auto& current = currentRuntimeChecked();
    return std::make_tuple(current.isolate_, current.context_.Get(current.isolate_));
}

v8::Isolate*           EngineScope::currentRuntimeIsolateChecked() { return currentRuntimeChecked().isolate_; }
v8::Local<v8::Context> EngineScope::currentRuntimeContextChecked() {
    auto& current = currentRuntimeChecked();
    return current.context_.Get(current.isolate_);
}


ExitEngineScope::ExitEngineScope() : mUnlocker(EngineScope::currentRuntimeChecked().isolate_) {}

namespace internal {

V8EscapeScope::V8EscapeScope() : mHandleScope(EngineScope::currentRuntimeChecked().isolate_) {}
V8EscapeScope::V8EscapeScope(v8::Isolate* isolate) : mHandleScope(isolate) {}

} // namespace internal


} // namespace v8wrap