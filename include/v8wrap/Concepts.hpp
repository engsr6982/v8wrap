#pragma once
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


} // namespace v8wrap