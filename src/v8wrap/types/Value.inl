#pragma once
#include "v8wrap/reference/Local.h"
#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/runtime/Exception.h"
#include "v8wrap/types/Value.h"
#include <utility>

#include "v8wrap/bind/adapter/FunctionAdapter.h"

namespace v8wrap {


template <typename T>
    requires std::same_as<T, int64_t>
Local<BigInt> BigInt::newBigInt(T i) {
    auto isolate = EngineScope::currentRuntimeIsolateChecked();
    return Local<BigInt>{v8::BigInt::New(isolate, i)};
}

template <typename T>
    requires std::same_as<T, uint64_t>
Local<BigInt> BigInt::newBigInt(T u) {
    auto isolate = EngineScope::currentRuntimeIsolateChecked();
    return Local<BigInt>{v8::BigInt::NewFromUnsigned(isolate, u)};
}


template <typename T>
    requires concepts::JsFunctionCallback<T>
Local<Function> Function::newFunction(T&& cb) {
    return newFunctionImpl(std::forward<T>(cb));
}

template <typename Fn>
    requires(!concepts::JsFunctionCallback<Fn>)
Local<Function> Function::newFunction(Fn&& func) {
    return newFunctionImpl(bind::adapter::bindStaticFunction(std::forward<Fn>(func)));
}

template <typename... Fn>
    requires(sizeof...(Fn) > 1 && (!concepts::JsFunctionCallback<Fn> && ...))
Local<Function> Function::newFunction(Fn&&... func) {
    return newFunctionImpl(bind::adapter::bindStaticOverloadedFunction(std::forward<Fn>(func)...));
}


template <typename T>
    requires concepts::JsValueType<T>
v8::Local<internal::V8Type_v<T>> ValueHelper::unwrap(Local<T> const& value) {
    return value.val; // friend
}
template <typename T>
Local<T> ValueHelper::wrap(v8::Local<internal::V8Type_v<T>> const& value) {
    return Local<T>{value};
}


} // namespace v8wrap