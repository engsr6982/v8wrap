#pragma once
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsValue.hpp"


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


} // namespace v8wrap