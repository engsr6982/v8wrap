#pragma once
#include <type_traits>

#include <string_view>

#include "v8wrap/concepts/BasicConcepts.h"

namespace v8wrap::traits {

template <typename T>
struct RawTypeHelper {
    using Decayed = std::decay_t<T>;
    using type    = std::conditional_t<
           concepts::StringLike<Decayed>,
           std::string_view,              // 命中 StringLike -> 强制映射为 string_view (fix const char[N])
           std::remove_pointer_t<Decayed> // 其他类型 -> 移除指针
           >;
};

template <typename T>
using RawType_t = typename RawTypeHelper<T>::type;


template <typename T>
inline constexpr size_t size_of_v = sizeof(T);

template <>
inline constexpr size_t size_of_v<void> = 0;


} // namespace v8wrap::traits