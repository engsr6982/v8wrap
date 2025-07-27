#pragma once
#include "v8wrap/JsException.hpp"
#include "v8wrap/JsValue.hpp"
#include "v8wrap/TypeConverter.hpp"
#include "v8wrap/Types.hpp"
#include <cstddef>
#include <tuple>


namespace v8wrap {


namespace internal {


// 主模板：对任意类型 operator()
template <typename T>
struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};

// 普通函数、函数指针
template <typename R, typename... Args>
struct FunctionTraits<R (*)(Args...)> {
    using ReturnType          = R;
    using ArgsTuple           = std::tuple<Args...>;
    static constexpr size_t N = sizeof...(Args);
};

template <typename R, typename... Args>
struct FunctionTraits<R(Args...)> {
    using ReturnType          = R;
    using ArgsTuple           = std::tuple<Args...>;
    static constexpr size_t N = sizeof...(Args);
};

template <typename R, typename... Args>
struct FunctionTraits<std::function<R(Args...)>> {
    using ReturnType          = R;
    using ArgsTuple           = std::tuple<Args...>;
    static constexpr size_t N = sizeof...(Args);
};

// 成员函数
template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const> { // const for lambdas
    using ReturnType          = R;
    using ArgsTuple           = std::tuple<Args...>;
    static constexpr size_t N = sizeof...(Args);
};

// 非 const 成员函数
template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...)> {
    using ReturnType          = R;
    using ArgsTuple           = std::tuple<Args...>;
    static constexpr size_t N = sizeof...(Args);
};

// const noexcept 成员函数
template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const noexcept> {
    using ReturnType          = R;
    using ArgsTuple           = std::tuple<Args...>;
    static constexpr size_t N = sizeof...(Args);
};


// 获取参数个数
template <typename T>
constexpr size_t ArgsCount_v = FunctionTraits<T>::N;

// 获取第N个参数类型
template <typename T, size_t N>
using ArgNType = std::tuple_element_t<N, typename FunctionTraits<T>::ArgsTuple>;

// 获取首个参数类型
template <typename T>
using ArgType_t = ArgNType<T, 0>;


// 转换参数类型
template <typename Tuple, std::size_t... Is>
decltype(auto) ConvertArgsToTuple(const Arguments& args, std::index_sequence<Is...>) {
    return std::make_tuple(ConvertToCpp<std::tuple_element_t<Is, Tuple>>(args[Is])...);
}


// static
template <typename Func>
JsFunctionCallback bindStaticFunction(Func&& func) {
    return [f = std::forward<Func>(func)](Arguments const& args) -> Local<JsValue> {
        using Traits       = FunctionTraits<std::decay_t<Func>>;
        using R            = typename Traits::ReturnType;
        using Tuple        = typename Traits::ArgsTuple;
        constexpr size_t N = std::tuple_size_v<Tuple>;

        if (args.length() != N) {
            throw JsException("argument count mismatch");
        }

        if constexpr (std::is_void_v<R>) {
            std::apply(f, ConvertArgsToTuple<Tuple>(args, std::make_index_sequence<N>()));
            return JsUndefined::newUndefined();
        } else {
            auto ret = std::apply(f, ConvertArgsToTuple<Tuple>(args, std::make_index_sequence<N>()));
            return ConvertToJs(ret);
        }
    };
}

template <typename Fn>
JsGetterCallback bindStaticGetter(Fn&& fn) {}

template <typename Fn>
JsSetterCallback bindStaticSetter(Fn&& fn) {}


// Instance
template <typename C, typename Fn>
JsInstanceConstructor bindInstanceConstructor(Fn&& fn) {}

template <typename C, typename Fn>
JsInstanceFunctionCallback bindInstanceMethod(Fn&& fn) {}

template <typename C, typename Fn>
JsInstanceGetterCallback bindInstanceGetter(Fn&& fn) {}

template <typename C, typename Fn>
JsInstanceSetterCallback bindInstanceSetter(Fn&& fn) {}


} // namespace internal


} // namespace v8wrap