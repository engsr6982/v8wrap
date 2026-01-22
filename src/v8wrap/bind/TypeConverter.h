#pragma once
#include "v8wrap/Types.h"
#include "v8wrap/traits/TypeTraits.h"

#include <string>
#include <string_view>
#include <type_traits>


namespace v8wrap::bind {


template <typename T>
struct TypeConverter {
    static_assert(
        sizeof(T) == 0,
        "TypeConverter does not have a specialization for type T. Did you forget to specialize the template?"
    );
};

template <typename T>
concept HasTypeConverter = requires { typename TypeConverter<T>; };


namespace internal {

template <typename T>
using RawTypeConverter = TypeConverter<traits::RawType_t<T>>;

template <typename  T>
concept TypeConverterAvailable = requires(Local<Value> const& v) {
    { RawTypeConverter<T>::toCpp(v) };
} &&
(
    requires(traits::RawType_t<T>& ref) { RawTypeConverter<T>::toJs(ref); } ||
    requires(traits::RawType_t<T> val) { RawTypeConverter<T>::toJs(val); } ||
    requires(traits::RawType_t<T>* ptr) { RawTypeConverter<T>::toJs(ptr); }
);

template <typename T>
constexpr bool IsTypeConverterAvailable_v = TypeConverterAvailable<T>;


/**
 * @brief C++ 值类型转换器
 * @note 此转换器设计目的是对于某些特殊情况，例如 void foo(std::string_view)
 *       在绑定时，TypeConverter 对字符串的特化是接受 StringLike_v，但返回值统一为 std::string
 *       这种特殊情况下，会导致 ConvertToCpp<std::string_view> 内部类型断言失败:
 * @code using RawConvRet = std::remove_cv_t<std::remove_reference_t<TypedToCppRet<std::string_view>>> // std::string
 * @code std::same_v<RawConvRet, std::string_view> // false
 *
 * @note 为了解决此问题，引入了 CppValueTypeTransformer，用于放宽类型约束
 * @note 需要注意的是 CppValueTypeTransformer 仅放宽了类型约束，实际依然需要特化 TypeConverter<T>
 */
template <typename From, typename To>
struct CppValueTypeTransformer : std::false_type {};

template <>
struct CppValueTypeTransformer<std::string, std::string_view> : std::true_type {};

template <typename From, typename To>
inline constexpr bool CppValueTypeTransformer_v = CppValueTypeTransformer<From, To>::value;


} // namespace internal


template <typename T>
[[nodiscard]] inline Local<Value> ConvertToJs(T&& value);

template <typename T>
[[nodiscard]] inline decltype(auto) ConvertToCpp(Local<Value> const& value);

} // namespace v8wrap::bind

#include "TypeConverter.inl"
