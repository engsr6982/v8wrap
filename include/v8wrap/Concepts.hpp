#pragma once
#include "Types.hpp"
#include <concepts>
#include <string>
#include <type_traits>


namespace v8wrap {


template <typename T>
concept IsI64 = std::is_same_v<T, int64_t>;

template <typename T>
concept IsU64 = std::is_same_v<T, uint64_t>;

template <typename T>
concept IsI64OrU64 = IsI64<T> || IsU64<T>;

template <typename T>
concept IsJsNumberLike = std::is_arithmetic_v<T> && !IsI64OrU64<T>;


template <typename T>
concept StringLike = requires(const T& s) {
    { std::basic_string_view(s) } -> std::convertible_to<std::string_view>;
} || std::is_same_v<std::remove_cvref_t<T>, std::string>;


template <typename T>
concept IsJsFunctionCallback = std::is_invocable_r_v<Local<JsValue>, T, Arguments const&>;


template <typename T>
concept IsWrappedV8Type =
    std::is_same_v<std::remove_cvref_t<T>, JsValue> || std::is_same_v<std::remove_cvref_t<T>, JsNull>
    || std::is_same_v<std::remove_cvref_t<T>, JsUndefined> || std::is_same_v<std::remove_cvref_t<T>, JsBoolean>
    || std::is_same_v<std::remove_cvref_t<T>, JsNumber> || std::is_same_v<std::remove_cvref_t<T>, JsBigInt>
    || std::is_same_v<std::remove_cvref_t<T>, JsString> || std::is_same_v<std::remove_cvref_t<T>, JsSymbol>
    || std::is_same_v<std::remove_cvref_t<T>, JsFunction> || std::is_same_v<std::remove_cvref_t<T>, JsObject>
    || std::is_same_v<std::remove_cvref_t<T>, JsArray>;


} // namespace v8wrap