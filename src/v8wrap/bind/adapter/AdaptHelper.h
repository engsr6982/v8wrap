#pragma once
#include "v8wrap/bind/TypeConverter.h"
#include "v8wrap/types/Value.h"

namespace v8wrap::bind::adapter {


// 辅助模板：根据参数类型决定存储类型
// template <typename T>
// using TupleElementType = std::conditional_t<
//     std::is_lvalue_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>,
//     T,                     // 保持 T&（非 const 左值引用）
//     std::remove_cvref_t<T> // 其他情况按值存储（移除 cv 和引用）
//     >;

// 辅助模板：根据 ConvertToCpp<T> 的返回类型决定存储类型
template <typename T>
using ConvertReturnType = decltype(ConvertToCpp<T>(std::declval<Arguments const&>()[std::declval<size_t>()]));

template <typename T>
using TupleElementType = std::conditional_t<
    std::is_lvalue_reference_v<ConvertReturnType<T>>,
    ConvertReturnType<T>,                     // 保持返回的引用类型（U& 或 const U&）
    std::remove_cvref_t<ConvertReturnType<T>> // 否则按值存储
    >;

// 转换参数类型
template <typename Tuple, std::size_t... Is>
inline decltype(auto) ConvertArgsToTuple(Arguments const& args, std::index_sequence<Is...>) {
    // [1]deprecated: 使用 std::make_tuple 进行全值传递
    //  - std::make_tuple 会强制将所有元素按值存储，即使原类型是引用
    //  - 若 Tuple 中包含引用类型（如 T&），此处会被 decay 为值类型，导致类型不匹配
    // return std::make_tuple(ConvertToCpp<std::tuple_element_t<Is, Tuple>>(args[Is])...);

    // [2]deprecated: 直接使用目标 Tuple 类型构造
    //  - 当 Tuple 元素类型为 const T& 时，可能绑定到 ConvertToCpp 返回的临时对象（右值）
    //  - 临时对象在当前语句结束后销毁，导致元组中存储的引用变为悬空引用
    // return Tuple(ConvertToCpp<std::tuple_element_t<Is, Tuple>>(args[Is])...);

    // [3]deprecated: 使用 TupleElementType 辅助模板，将引用类型转换为值类型
    //  - 此方法可避免 [1] 和 [2] 的问题，但依然会造成不必要的拷贝
    // using ResultTuple = std::tuple<TupleElementType<std::tuple_element_t<Is, Tuple>>...>;
    // return ResultTuple(ConvertToCpp<std::tuple_element_t<Is, Tuple>>(args[Is])...);

    using ResultTuple = std::tuple<TupleElementType<std::tuple_element_t<Is, Tuple>>...>;
    return ResultTuple(ConvertToCpp<std::tuple_element_t<Is, Tuple>>(args[Is])...);
}


} // namespace v8wrap::bind::adapter