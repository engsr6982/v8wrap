#pragma once
#include "Global.h"
#include "v8-value.h"
#include "v8wrap/reference/Local.h"
#include "v8wrap/runtime/Engine.h"
#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/types/Value.h"

namespace v8wrap {

// Global<T>
template <typename T>
Global<T>::Global() noexcept = default;

template <typename T>
Global<T>::Global(Local<T> const& val)
: engine_(EngineScope::currentRuntime()),
  handle_(engine_->isolate(), ValueHelper::unwrap(val)) {}

template <typename T>
Global<T>::Global(Weak<T> const& val) : engine_(EngineScope::currentRuntime()),
                                        handle_(ValueHelper::unwrap(val)) {}

template <typename T>
Global<T>::Global(Global<T>&& other) noexcept {
    engine_       = other.engine_;
    handle_       = std::move(other.handle_);
    other.engine_ = nullptr;
}

template <typename T>
Global<T>& Global<T>::operator=(Global<T>&& other) noexcept {
    if (&other != this) {
        engine_       = other.engine_;
        handle_       = std::move(other.handle_);
        other.engine_ = nullptr;
    }
    return *this;
}

template <typename T>
Global<T>::~Global() {
    reset();
}

template <typename T>
Local<T> Global<T>::get() const {
    return Local<T>{handle_.Get(engine_->mIsolate)};
}

template <typename T>
Local<Value> Global<T>::getValue() const {
    return Local<Value>{handle_.Get(engine_->mIsolate).template As<v8::Value>()};
}

template <typename T>
bool Global<T>::isEmpty() const {
    return handle_.IsEmpty();
}

template <typename T>
void Global<T>::reset() {
    handle_.Reset();
    engine_ = nullptr;
}


} // namespace v8wrap