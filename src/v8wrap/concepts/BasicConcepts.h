#pragma once
#include <concepts>
#include <string_view>
#include <type_traits>


namespace v8wrap ::concepts {


template <typename T>
concept NumberLike = std::is_arithmetic_v<T>;

template <typename T>
concept StringLike = std::convertible_to<T, std::string_view>;

template <typename T>
concept HasDefaultConstructor = requires { T{}; };

template <typename T>
concept HasEquality = requires(T const& lhs, T const& rhs) {
    { lhs == rhs } -> std::convertible_to<bool>;
};


} // namespace v8wrap::concepts