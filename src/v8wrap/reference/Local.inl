#pragma once
#include "v8wrap/reference/Local.h"
#include "v8wrap/runtime/Exception.h"

#include "v8wrap/bind/TypeConverter.h"

#include <cassert>

namespace v8wrap {


template <typename T>
    requires concepts::JsValueType<T>
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


template <typename... Args>
    requires(sizeof...(Args) > 0)
Local<Value> Local<Function>::call(Local<Value> const& thiz, Args&&... args) const {
    std::array<Local<Value>, sizeof...(Args)> argv = {bind::ConvertToJs(std::forward<Args>(args))...};
    return this->call(thiz, std::span<const Local<Value>>(argv));
}

template <typename... Args>
[[nodiscard]] Local<Value> Local<Function>::callAsConstructor(Args&&... args) const {
    std::array<Local<Value>, sizeof...(Args)> argv = {bind::ConvertToJs(std::forward<Args>(args))...};
    return callAsConstructor(std::span<const Local<Value>>(argv));
}


} // namespace v8wrap