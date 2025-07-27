#pragma once
#include "v8wrap/JsException.hpp"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsValue.hpp"
#include <utility>

#include "v8wrap/Bindings.hpp"


namespace v8wrap {


template <typename T>
    requires IsI64<T>
Local<JsBigInt> JsBigInt::newBigInt(T i) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsBigInt>{v8::BigInt::New(isolate, i)};
}

template <typename T>
    requires IsU64<T>
Local<JsBigInt> JsBigInt::newBigInt(T u) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsBigInt>{v8::BigInt::NewFromUnsigned(isolate, u)};
}


template <typename T>
    requires IsJsFunctionCallback<T>
Local<JsFunction> JsFunction::newFunction(T cb) {
    return newFunctionImpl(std::move(cb));
}

template <typename Fn>
    requires(!IsJsFunctionCallback<Fn>)
Local<JsFunction> JsFunction::newFunction(Fn&& func) {
    return newFunctionImpl(internal::bindStaticFunction(std::forward<Fn>(func)));
}

} // namespace v8wrap