#pragma once
#include <concepts>
#include <string>
#include <type_traits>

#include "v8wrap/Types.h"

namespace v8wrap::concepts {


template <typename T>
concept JsValueType =
    std::same_as<T, Value> || std::same_as<T, Undefined> || std::same_as<T, Null> || std::same_as<T, Boolean>
    || std::same_as<T, Number> || std::same_as<T, String> || std::same_as<T, Object> || std::same_as<T, Array>
    || std::same_as<T, Function> || std::same_as<T, BigInt> || std::same_as<T, Symbol>;


template <typename T>
concept JsFunctionCallback =
    std::is_invocable_r_v<Local<Value>, T, Arguments const&> || std::convertible_to<T, FunctionCallback>;

template <typename T>
concept JsInstanceMethodCallback = std::is_invocable_r_v<Local<Value>, T, void*, Arguments const&>;

// JavaScript value types: undefined、null、boolean、number、string、symbol
template <typename T>
concept JsPrimitiveType = std::is_same_v<T, Undefined> || std::is_same_v<T, Null> || std::is_same_v<T, Boolean>
                       || std::is_same_v<T, Number> || std::is_same_v<T, String> || std::is_same_v<T, Symbol>;

template <typename T>
constexpr bool JsPrimitiveType_v = JsPrimitiveType<T>;

template <typename T>
concept JsPrimitiveConvertible =
    JsPrimitiveType<T> || std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<std::decay_t<T>, std::string>;

template <typename T>
constexpr bool JsPrimitiveConvertible_v = JsPrimitiveConvertible<T>;

} // namespace v8wrap::concepts