#pragma once
#include "Types.h"
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
concept IsJsFunctionCallback = std::is_invocable_r_v<Local<Value>, T, Arguments const&>;

template <typename T>
concept IsJsGetterCallback = std::is_invocable_r_v<Local<Value>, T>;

template <typename T>
concept IsJsSetterCallback = std::is_invocable_r_v<void, T, Local<Value> const&>;

template <typename T>
concept IsJsInstanceConstructor = std::is_invocable_r_v<void*, T, Arguments const&>;

template <typename T>
concept IsJsInstanceMethodCallback = std::is_invocable_r_v<Local<Value>, T, void*, Arguments const&>;

template <typename T>
concept IsJsInstanceGetterCallback = std::is_invocable_r_v<Local<Value>, T, void*, Arguments const&>;

template <typename T>
concept IsJsInstanceSetterCallback = std::is_invocable_r_v<void, T, void*, Arguments const&>;


template <typename T>
concept HasDefaultConstructor = requires {
    { T{} } -> std::same_as<T>;
    requires std::is_default_constructible_v<T>;
};

template <typename T>
concept HasUserDeclaredDestructor = !std::is_trivially_destructible_v<T>;

template <typename T>
concept IsWrappedV8Type =
    std::is_same_v<std::remove_cvref_t<T>, Value> || std::is_same_v<std::remove_cvref_t<T>, Null>
    || std::is_same_v<std::remove_cvref_t<T>, Undefined> || std::is_same_v<std::remove_cvref_t<T>, Boolean>
    || std::is_same_v<std::remove_cvref_t<T>, Number> || std::is_same_v<std::remove_cvref_t<T>, BigInt>
    || std::is_same_v<std::remove_cvref_t<T>, String> || std::is_same_v<std::remove_cvref_t<T>, Symbol>
    || std::is_same_v<std::remove_cvref_t<T>, Function> || std::is_same_v<std::remove_cvref_t<T>, Object>
    || std::is_same_v<std::remove_cvref_t<T>, Array>;


} // namespace v8wrap