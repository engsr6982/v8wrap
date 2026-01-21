#pragma once
#include "v8wrap/Concepts.h"
#include "v8wrap/reference/Reference.h"
#include "v8wrap/types/Value.h"
#include <string>
#include <type_traits>


namespace v8wrap {


template <typename T>
struct TypeConverter {
    static_assert(sizeof(T) == 0, "Cannot convert Js to this type T; no TypeConverter defined.");
};


template <>
struct TypeConverter<bool> {
    static Local<Boolean> toJs(bool value) { return Boolean::newBoolean(value); }

    static bool toCpp(Local<Value> const& value) { return value.asBoolean().getValue(); }
};


// int/uint/float/double → Number
template <IsJsNumberLike T>
struct TypeConverter<T> {
    static Local<Number> toJs(T value) { return Number::newNumber(static_cast<double>(value)); }

    static T toCpp(Local<Value> const& value) { return static_cast<T>(value.asNumber().getDouble()); }
};


// int64_t / uint64_t → BigInt
template <IsI64OrU64 T>
struct TypeConverter<T> {
    static Local<BigInt> toJs(T value) { return BigInt::newBigInt(value); }

    static T toCpp(Local<Value> const& value) {
        if constexpr (IsI64<T>) {
            return value.asBigInt().getInt64();
        } else {
            return value.asBigInt().getUint64();
        }
    }
};


template <typename T>
    requires StringLike<T>
struct TypeConverter<T> {
    static Local<String> toJs(T const& value) { return String::newString(value); }
    // static Local<String> toJs(std::string const& value) { return String::newString(value); }

    static std::string toCpp(Local<Value> const& value) { return value.asString().getValue(); } // always UTF-8
};


// enum -> Number (enum value)
template <typename T>
    requires std::is_enum_v<T>
struct TypeConverter<T> {
    static Local<Number> toJs(T value) { return Number::newNumber(static_cast<int>(value)); }

    static T toCpp(Local<Value> const& value) { return static_cast<T>(value.asNumber().getInt32()); }
};


// internal type
template <typename T>
    requires IsWrappedV8Type<T>
struct TypeConverter<Local<T>> {
    static Local<Value> toJs(Local<T> const& value) { return value.asValue(); }
    static Local<T>     toCpp(Local<Value> const& value) { return value.as<T>(); }
};


namespace internal {

template <typename T>
using TypedConverter = TypeConverter<typename std::decay<T>::type>;
// using TypedConverter = TypeConverter<std::remove_cvref_t<T>>;
// using TypedConverter = TypeConverter<T>;


template <typename T, typename = void>
struct IsTypeConverterAvailable : std::false_type {};

template <typename T>
struct IsTypeConverterAvailable<
    T,
    std::void_t<
        decltype(TypedConverter<T>::toJs(std::declval<T>())),
        decltype(TypedConverter<T>::toCpp(std::declval<Local<Value>>()))>> : std::true_type {};

template <typename T>
constexpr bool IsTypeConverterAvailable_v = IsTypeConverterAvailable<T>::value;

} // namespace internal


template <typename T>
[[nodiscard]] inline Local<Value> ConvertToJs(T&& value) {
    static_assert(
        internal::IsTypeConverterAvailable_v<T>,
        "Cannot convert T to Js; there is no available TypeConverter."
    );
    return internal::TypedConverter<T>::toJs(std::forward<T>(value)).asValue();
}

template <typename T>
[[nodiscard]] inline decltype(auto) ConvertToCpp(Local<Value> const& value) {
    static_assert(
        internal::IsTypeConverterAvailable_v<T>,
        "Cannot convert Js to T; there is no available TypeConverter."
    );
    return internal::TypedConverter<T>::toCpp(value);
}

} // namespace v8wrap