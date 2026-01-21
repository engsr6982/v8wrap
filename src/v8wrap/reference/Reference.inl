#pragma once
#include "v8wrap/reference/Reference.h"
#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/runtime/Exception.h"



#include <cassert>


namespace v8wrap {


template <typename T>
    requires IsWrappedV8Type<T>
Local<T> Local<Value>::as() const {
    if constexpr (std::is_same_v<T, Value>) {
        return asValue();
    } else if constexpr (std::is_same_v<T, Null>) {
        return asNull();
    } else if constexpr (std::is_same_v<T, Undefined>) {
        return asUndefined();
    } else if constexpr (std::is_same_v<T, Boolean>) {
        return asBoolean();
    } else if constexpr (std::is_same_v<T, Number>) {
        return asNumber();
    } else if constexpr (std::is_same_v<T, BigInt>) {
        return asBigInt();
    } else if constexpr (std::is_same_v<T, String>) {
        return asString();
    } else if constexpr (std::is_same_v<T, Symbol>) {
        return asSymbol();
    } else if constexpr (std::is_same_v<T, Function>) {
        return asFunction();
    } else if constexpr (std::is_same_v<T, Object>) {
        return asObject();
    } else if constexpr (std::is_same_v<T, Array>) {
        return asArray();
    }
    throw Exception("Unable to convert Local<Value> to T, forgot to add if branch?");
}


// Global<T>
template <typename T>
Global<T>::Global() noexcept : impl{} {}

template <typename T>
Global<T>::Global(Local<T> const& val) : impl(EngineScope::currentRuntime(), val) {
    auto* runtime = EngineScope::currentRuntime();
    assert(runtime && "Global<T> must be created inside a EngineScope");
}

template <typename T>
Global<T>::Global(Weak<T> const& val) : impl(EngineScope::currentRuntime(), val) {}

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
Local<Value> Global<T>::getValue() const {
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
Weak<T>::Weak(Local<T> const& val) : impl(EngineScope::currentRuntime(), val) {
    impl.makeWeak();
    auto* runtime = EngineScope::currentRuntime();
    assert(runtime && "Weak<T> must be created inside a EngineScope");
}

template <typename T>
Weak<T>::Weak(Global<T> const& val) : impl(EngineScope::currentRuntime(), val) {
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
Local<Value> Weak<T>::getValue() const {
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