#pragma once
#include "v8-value.h"
#include "v8wrap/reference/Local.h"
#include "v8wrap/reference/Weak.h"
#include "v8wrap/runtime/Engine.h"
#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/types/Value.h"


namespace v8wrap {


// Weak<T>
template <typename T>
Weak<T>::Weak() noexcept = default;

template <typename T>
Weak<T>::Weak(Local<T> const& val)
: engine_(EngineScope::currentRuntime()),
  handle_(engine_->isolate(), ValueHelper::unwrap(val)) {
    markWeak();
}

template <typename T>
Weak<T>::Weak(Global<T> const& val)
: engine_(EngineScope::currentRuntime()),
  handle_(engine_->isolate(), ValueHelper::unwrap(val)) {
    markWeak();
}

template <typename T>
Weak<T>::Weak(Weak<T>&& other) noexcept {
    engine_ = other.engine_;
    handle_ = std::move(other.handle_);
    markWeak();
    other.engine_ = nullptr;
}

template <typename T>
Weak<T>& Weak<T>::operator=(Weak<T>&& other) noexcept {
    if (&other != this) {
        engine_ = other.engine_;
        handle_ = std::move(other.handle_);
        markWeak();
        other.engine_ = nullptr;
    }
    return *this;
}

template <typename T>
Weak<T>::~Weak() {
    reset();
}

template <typename T>
Local<T> Weak<T>::get() const {
    return Local<T>{handle_.Get(engine_->mIsolate)};
}

template <typename T>
Local<Value> Weak<T>::getValue() const {
    return Local<Value>{handle_.Get(engine_->mIsolate).template As<v8::Value>()};
}

template <typename T>
bool Weak<T>::isEmpty() const {
    return handle_.IsEmpty();
}

template <typename T>
void Weak<T>::reset() {
    handle_.Reset();
    engine_ = nullptr;
}


template <typename T>
void Weak<T>::markWeak() {
    if (!handle_.IsEmpty()) {
        handle_.SetWeak();
    }
}

} // namespace v8wrap