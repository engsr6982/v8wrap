#pragma once
#include "v8wrap/Concepts.hpp"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsValue.hpp"
#include <string>
#include <type_traits>


namespace v8wrap {


template <typename T>
struct TypeConverter;


template <>
struct TypeConverter<bool> {
    static Local<JsBoolean> toJs(bool value) { return JsBoolean::newBoolean(value); }

    static bool toCpp(Local<JsValue> const& value) { return value.asBoolean().getValue(); }
};


// int/uint/float/double → JsNumber
template <IsJsNumberLike T>
struct TypeConverter<T> {
    static Local<JsNumber> toJs(T value) { return JsNumber::newNumber(static_cast<double>(value)); }

    static T toCpp(Local<JsValue> const& value) { return static_cast<T>(value.asNumber().getDouble()); }
};


// int64_t / uint64_t → JsBigInt
template <IsI64OrU64 T>
struct TypeConverter<T> {
    static Local<JsBigInt> toJs(T value) { return JsBigInt::newBigInt(value); }

    static T toCpp(Local<JsValue> const& value) {
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
    static Local<JsString> toJs(T const& value) { return JsString::newString(value); }
    // static Local<JsString> toJs(std::string const& value) { return JsString::newString(value); }

    static std::string toCpp(Local<JsValue> const& value) { return value.asString().getValue(); } // always UTF-8
};


// enum -> JsNumber (enum value)
template <typename T>
    requires std::is_enum_v<T>
struct TypeConverter<T> {
    static Local<JsNumber> toJs(T value) { return JsNumber::newNumber(static_cast<int>(value)); }

    static T toCpp(Local<JsValue> const& value) { return static_cast<T>(value.asNumber().getInt32()); }
};


template <typename T>
[[nodiscard]] inline Local<JsValue> ConvertToJs(T const& value) {
    return TypeConverter<T>::toJs(value).asValue();
}

template <typename T>
[[nodiscard]] inline T ConvertToCpp(Local<JsValue> const& value) {
    return TypeConverter<T>::toCpp(value);
}

} // namespace v8wrap