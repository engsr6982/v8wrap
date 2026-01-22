#pragma once
#include "TypeConverter.h"
#include "v8wrap/bind/adapter/CallbackAdapter.h"
#include "v8wrap/concepts/BasicConcepts.h"
#include "v8wrap/concepts/ScriptConcepts.h"
#include "v8wrap/reference/Local.h"
#include "v8wrap/runtime/Exception.h"
#include "v8wrap/types/Value.h"


#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>

namespace v8wrap::bind {

// internal type
template <typename T>
    requires concepts::JsValueType<T>
struct TypeConverter<Local<T>> {
    static Local<Value> toJs(Local<T> const& value) { return value.asValue(); }

    static Local<T> toCpp(Local<Value> const& value) { return value.as<T>(); }
};

// bool <-> Boolean
template <>
struct TypeConverter<bool> {
    static Local<Boolean> toJs(bool value) { return Boolean::newBoolean(value); }

    static bool toCpp(Local<Value> const& value) { return value.asBoolean().getValue(); }
};

// int/uint/float/double <-> Number
template <typename T>
    requires(concepts::NumberLike<T> && !std::same_as<T, int64_t> && !std::same_as<T, uint64_t>)
struct TypeConverter<T> {
    static Local<Number> toJs(T value) { return Number::newNumber(static_cast<double>(value)); }

    static T toCpp(Local<Value> const& value) { return static_cast<T>(value.asNumber().getDouble()); }
};

// int64/uint64 <-> BigInt
template <typename T>
    requires(std::same_as<T, int64_t> || std::same_as<T, uint64_t>)
struct TypeConverter<T> {
    static Local<BigInt> toJs(T value) { return BigInt::newBigInt(value); }

    static T toCpp(Local<Value> const& value) {
        if constexpr (std::same_as<T, int64_t>) {
            return value.asBigInt().getInt64();
        } else {
            return value.asBigInt().getUint64();
        }
    }
};

// std::string <-> String
template <typename T>
    requires concepts::StringLike<T>
struct TypeConverter<T> {
    static Local<String> toJs(T const& value) { return String::newString(std::string_view{value}); }

    static std::string toCpp(Local<Value> const& value) { return value.asString().getValue(); } // always UTF-8
};

// enum -> Number (enum value)
template <typename T>
    requires std::is_enum_v<T>
struct TypeConverter<T> {
    static Local<Number> toJs(T value) { return Number::newNumber(static_cast<int>(value)); }

    static T toCpp(Local<Value> const& value) { return static_cast<T>(value.asNumber().getInt32()); }
};


// std::function -> Function
template <typename R, typename... Args>
struct TypeConverter<std::function<R(Args...)>> {
    static_assert(
        (HasTypeConverter<Args> && ...),
        "Cannot convert std::function to Function; all parameter types must have a TypeConverter"
    );

    // ! UnSupported: cannot convert function to Value
    static Local<Value> toJs(std::function<R(Args...)> const& /* value */) {
        // TODO: impl
        throw std::logic_error("UnSupported: cannot convert std::function to Value");
    }

    static std::function<R(Args...)> toCpp(Local<Value> const& value) {
        return adapter::bindScriptCallback<R, Args...>(value);
    }
};

// std::optional <-> null/undefined
template <typename T>
struct TypeConverter<std::optional<T>> {
    static Local<Value> toJs(std::optional<T> const& value) {
        if (value) {
            return ConvertToJs(value.value());
        }
        return Null::newNull(); // default to null
    }

    static std::optional<T> toCpp(Local<Value> const& value) {
        if (value.isUndefined() || value.isNull()) {
            return std::nullopt;
        }
        return std::optional<T>{ConvertToCpp<T>(value)};
    }
};

// std::vector <-> Array
template <typename T>
struct TypeConverter<std::vector<T>> {
    static Local<Value> toJs(std::vector<T> const& value) {
        auto array = Array::newArray(value.size());
        for (std::size_t i = 0; i < value.size(); ++i) {
            array.set(i, ConvertToJs(value[i]));
        }
        return array;
    }

    static std::vector<T> toCpp(Local<Value> const& value) {
        auto array = value.asArray();

        std::vector<T> result;
        result.reserve(array.length());
        for (std::size_t i = 0; i < array.length(); ++i) {
            result.push_back(ConvertToCpp<T>(array[i]));
        }
        return result;
    }
};

// std::unordered_map <-> Object
template <typename K, typename V>
    requires concepts::StringLike<K> // JavaScript only supports string keys
struct TypeConverter<std::unordered_map<K, V>> {
    static_assert(HasTypeConverter<V>, "Cannot convert std::unordered_map to Object; type V has no TypeConverter");

    static Local<Value> toJs(std::unordered_map<K, V> const& value) {
        auto object = Object::newObject();
        for (auto const& [key, val] : value) {
            object.set(key, ConvertToJs(val));
        }
        return object;
    }

    static std::unordered_map<K, V> toCpp(Local<Value> const& value) {
        auto object = value.asObject();
        auto keys   = object.getOwnPropertyNames();

        std::unordered_map<K, V> result;
        for (auto const& key : keys) {
            result[key.getValue()] = ConvertToCpp<V>(object.get(key));
        }
        return result;
    }
};

// std::variant <-> Type
template <typename... Is>
struct TypeConverter<std::variant<Is...>> {
    static_assert(
        (HasTypeConverter<Is> && ...),
        "Cannot convert std::variant to Object; all types must have a TypeConverter"
    );
    using TypedVariant = std::variant<Is...>;

    static Local<Value> toJs(TypedVariant const& value) {
        if (value.valueless_by_exception()) {
            return Null::newNull();
        }
        return std::visit([&](auto const& v) -> Local<Value> { return ConvertToJs(v); }, value);
    }

    static TypedVariant toCpp(Value const& value) { return tryToCpp(value); }

    template <size_t I = 0>
    static TypedVariant tryToCpp(Value const& value) {
        if constexpr (I >= sizeof...(Is)) {
            throw Exception{
                "Cannot convert Value to std::variant; no matching type found.",
                Exception::Type::TypeError
            };
        } else {
            using Type = std::variant_alternative_t<I, TypedVariant>;
            try {
                return ConvertToCpp<Type>(value);
            } catch (Exception const&) {
                return tryToCpp<I + 1>(value);
            }
        }
    }
};

template <>
struct TypeConverter<std::monostate> {
    static Local<Value> toJs(std::monostate) { return Null::newNull(); }

    static std::monostate toCpp(Local<Value> const& value) {
        if (value.isUndefined() || value.isNull()) {
            return std::monostate{};
        }
        [[unlikely]] throw Exception{"Expected null/undefined for std::monostate", Exception::Type::TypeError};
    }
};


template <typename Ty1, typename Ty2>
struct TypeConverter<std::pair<Ty1, Ty2>> {
    static_assert(internal::IsTypeConverterAvailable_v<Ty1>);
    static_assert(internal::IsTypeConverterAvailable_v<Ty2>);

    static Local<Value> toJs(std::pair<Ty1, Ty2> const& pair) {
        auto array = Array::newArray(2);
        array.set(0, ConvertToJs(pair.first));
        array.set(1, ConvertToJs(pair.second));
        return array;
    }
    static std::pair<Ty1, Ty2> toCpp(Local<Value> const& value) {
        if (!value.isArray() || value.asArray().length() != 2) {
            throw Exception{"Invalid argument type, expected array with 2 elements"};
        }
        auto array = value.asArray();
        return std::make_pair(ConvertToCpp<Ty1>(array.get(0)), ConvertToCpp<Ty2>(array.get(1)));
    }
};


template <typename T>
[[nodiscard]] inline Local<Value> ConvertToJs(T&& value) {
    static_assert(
        internal::IsTypeConverterAvailable_v<T>,
        "Cannot convert T to Js; there is no available TypeConverter."
    );
    return internal::RawTypeConverter<T>::toJs(std::forward<T>(value)).asValue();
}

template <typename T>
[[nodiscard]] inline decltype(auto) ConvertToCpp(Local<Value> const& value) {
    static_assert(
        internal::IsTypeConverterAvailable_v<T>,
        "Cannot convert Js to T; there is no available TypeConverter."
    );

    using RequestedT = T;                                                     // 可能为 T, T&, T*
    using BareT      = std::remove_cv_t<std::remove_reference_t<RequestedT>>; // T

    using Conv    = internal::RawTypeConverter<RequestedT>; // RawTypeConverter<T>
    using ConvRet = decltype(Conv::toCpp(std::declval<Local<Value>>()));

    if constexpr (std::is_lvalue_reference_v<RequestedT>) { // 需要 T&
        if constexpr (std::is_pointer_v<std::remove_reference_t<ConvRet>>) {
            auto p = Conv::toCpp(value); // 返回 T*
            if (p == nullptr) [[unlikely]] {
                throw std::runtime_error("TypeConverter::toCpp returned a null pointer.");
            }
            return static_cast<RequestedT&>(*p); // 返回 T&
        } else if constexpr (std::is_lvalue_reference_v<ConvRet>
                             || std::is_const_v<std::remove_reference_t<RequestedT>>) {
            return Conv::toCpp(value); // 已返回 T&，直接转发 或者 const T& 可以绑定临时
        } else {
            static_assert(
                std::is_pointer_v<std::remove_reference_t<ConvRet>> || std::is_lvalue_reference_v<ConvRet>,
                "TypeConverter::toCpp must return either T* or T& when ConvertToCpp<T&> is required. Returning T (by "
                "value) cannot bind to a non-const lvalue reference; change TypeConverter or request a value type."
            );
        }
    } else if constexpr (std::is_pointer_v<RequestedT>) { // 需要 T*
        if constexpr (std::is_pointer_v<std::remove_reference_t<ConvRet>>) {
            return Conv::toCpp(value); // 直接返回
        } else if constexpr (std::is_lvalue_reference_v<ConvRet>) {
            return std::addressof(Conv::toCpp(value)); // 返回 T& -> 可以取地址
        } else {
            static_assert(
                std::is_pointer_v<std::remove_reference_t<ConvRet>> || std::is_lvalue_reference_v<ConvRet>,
                "TypeConverter::toCpp must return T* or T& when ConvertToCpp<T*> is required. "
                "Returning T (by value) would produce pointer to temporary (unsafe)."
            );
        }
    } else {
        // 值类型 T
        using RawConvRet = std::remove_cv_t<std::remove_reference_t<ConvRet>>;
        if constexpr ((std::is_same_v<RawConvRet, BareT> || internal::CppValueTypeTransformer_v<RawConvRet, BareT>)
                      && !std::is_pointer_v<std::remove_reference_t<ConvRet>> && !std::is_lvalue_reference_v<ConvRet>) {
            return Conv::toCpp(value); // 按值返回 / 直接返回 (可能 NRVO)
        } else {
            static_assert(
                std::is_same_v<RawConvRet, BareT> && !std::is_pointer_v<std::remove_reference_t<ConvRet>>
                    && !std::is_lvalue_reference_v<ConvRet>,
                "TypeConverter::toCpp must return T (by value) for ConvertToCpp<T>. "
                "Other return forms (T* or T&) are not supported for value request."
            );
        }
    }
}


} // namespace v8wrap::bind
