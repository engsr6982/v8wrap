#pragma once
#include "v8wrap/reference/Reference.h"
#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/runtime/Exception.h"
#include "v8wrap/types/Value.h"
#include <utility>



#include "v8wrap/Bindings.h"


namespace v8wrap {


template <typename T>
    requires IsI64<T>
Local<BigInt> BigInt::newBigInt(T i) {
    auto isolate = EngineScope::currentRuntimeIsolateChecked();
    return Local<BigInt>{v8::BigInt::New(isolate, i)};
}

template <typename T>
    requires IsU64<T>
Local<BigInt> BigInt::newBigInt(T u) {
    auto isolate = EngineScope::currentRuntimeIsolateChecked();
    return Local<BigInt>{v8::BigInt::NewFromUnsigned(isolate, u)};
}


template <typename T>
    requires IsJsFunctionCallback<T>
Local<Function> Function::newFunction(T&& cb) {
    return newFunctionImpl(std::forward<T>(cb));
}

template <typename Fn>
    requires(!IsJsFunctionCallback<Fn>)
Local<Function> Function::newFunction(Fn&& func) {
    return newFunctionImpl(internal::bindStaticFunction(std::forward<Fn>(func)));
}

template <typename... Fn>
    requires(sizeof...(Fn) > 1 && (!IsJsFunctionCallback<Fn> && ...))
Local<Function> Function::newFunction(Fn&&... func) {
    return newFunctionImpl(internal::bindStaticOverloadedFunction(std::forward<Fn>(func)...));
}


template <typename T>
    requires IsWrappedV8Type<T>
v8::Local<internal::V8Type_v<T>> JsValueHelper::unwrap(Local<T> const& value) {
    return value.val; // friend
}
template <typename T>
Local<T> JsValueHelper::wrap(v8::Local<internal::V8Type_v<T>> const& value) {
    return Local<T>{value};
}


} // namespace v8wrap