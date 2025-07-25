#pragma once
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include <cassert>

namespace v8wrap {


// Global<T>
template <typename T>
Global<T>::Global() noexcept : impl{} {}

template <typename T>
Global<T>::Global(Local<T> const& val) : impl(JsRuntimeScope::currentRuntime(), val) {
    auto* runtime = JsRuntimeScope::currentRuntime();
    assert(runtime && "Global<T> must be created inside a JsRuntimeScope");
}

template <typename T>
Global<T>::Global(Weak<T> const& val) : impl(JsRuntimeScope::currentRuntime(), val) {}

template <typename T>
Global<T>::Global(Global<T>&& other) noexcept : impl(std::move(other.impl)) {}

template <typename T>
Global<T>& Global<T>::operator=(Global<T>&& other) noexcept {
    if (&other != this) {
        impl = std::move(other.impl);
    }
    return *this;
}

template <typename T>
Global<T>::~Global() = default;

template <typename T>
Local<T> Global<T>::get() const {
    return impl.template get<T>();
}

template <typename T>
Local<JsValue> Global<T>::getValue() const {
    return impl.getValue();
}

template <typename T>
bool Global<T>::isEmpty() const {
    return impl.isEmpty();
}

template <typename T>
void Global<T>::reset() {
    impl.reset();
}


// Weak<T>
template <typename T>
Weak<T>::Weak() noexcept : impl{} {
    impl.makeWeak();
}

template <typename T>
Weak<T>::Weak(Local<T> const& val) : impl(JsRuntimeScope::currentRuntime(), val) {
    impl.makeWeak();
    auto* runtime = JsRuntimeScope::currentRuntime();
    assert(runtime && "Weak<T> must be created inside a JsRuntimeScope");
}

template <typename T>
Weak<T>::Weak(Global<T> const& val) : impl(JsRuntimeScope::currentRuntime(), val) {
    impl.makeWeak();
}

template <typename T>
Weak<T>::Weak(Weak<T>&& other) noexcept : impl(std::move(other.impl)) {
    impl.makeWeak();
}

template <typename T>
Weak<T>& Weak<T>::operator=(Weak<T>&& other) noexcept {
    if (&other != this) {
        impl = std::move(other.impl);
        impl.makeWeak();
    }
    return *this;
}

template <typename T>
Weak<T>::~Weak() = default;

template <typename T>
Local<T> Weak<T>::get() const {
    return impl.template get<T>();
}

template <typename T>
Local<JsValue> Weak<T>::getValue() const {
    return impl.getValue();
}

template <typename T>
bool Weak<T>::isEmpty() const {
    return impl.isEmpty();
}

template <typename T>
void Weak<T>::reset() {
    impl.reset();
}


} // namespace v8wrap