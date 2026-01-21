#pragma once
#include "v8wrap/Concepts.h"
#include "v8wrap/Global.h"
#include "v8wrap/JsException.h"
#include "v8wrap/TypeConverter.h"
#include "v8wrap/Types.h"
#include "v8wrap/types/Value.h"
#include <array>
#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>



namespace v8wrap {


namespace internal {


// Primary template: redirect to operator()
template <typename T>
struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};

// 普通函数 / 函数指针
template <typename R, typename... Args>
struct FunctionTraits<R (*)(Args...)> {
    using ReturnType          = R;
    using ArgsTuple           = std::tuple<Args...>;
    static constexpr size_t N = sizeof...(Args);
};

template <typename R, typename... Args>
struct FunctionTraits<R(Args...)> : FunctionTraits<R (*)(Args...)> {};

// std::function
template <typename R, typename... Args>
struct FunctionTraits<std::function<R(Args...)>> : FunctionTraits<R (*)(Args...)> {};

// 成员函数指针（包括 const / noexcept 等）
template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...)> {
    using ReturnType          = R;
    using ArgsTuple           = std::tuple<Args...>;
    static constexpr size_t N = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const> : FunctionTraits<R (C::*)(Args...)> {};

template <typename C, typename R, typename... Args>
struct FunctionTraits<R (C::*)(Args...) const noexcept> : FunctionTraits<R (C::*)(Args...) const> {};

template <typename T>
struct DecayedFunctionTraits : FunctionTraits<std::remove_cvref_t<T>> {};


// 获取参数个数
template <typename T>
constexpr size_t ArgsCount_v = FunctionTraits<T>::N;

// 获取第N个参数类型
template <typename T, size_t N>
using ArgNType = std::tuple_element_t<N, typename DecayedFunctionTraits<T>::ArgsTuple>;

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
            decltype(auto) ret = std::apply(f, ConvertArgsToTuple<Tuple>(args, std::make_index_sequence<N>()));
            return ConvertToJs(ret);
        }
    };
}

template <typename... Func>
JsFunctionCallback bindStaticOverloadedFunction(Func&&... funcs) {
    std::vector functions = {bindStaticFunction(std::forward<Func>(funcs))...};
    return [fs = std::move(functions)](Arguments const& args) -> Local<JsValue> {
        for (size_t i = 0; i < sizeof...(Func); ++i) {
            try {
                return std::invoke(fs[i], args);
            } catch (JsException const&) {
                if (i == sizeof...(Func) - 1) {
                    throw JsException{"no overload found"};
                }
            }
        }
        return {}; // undefined
    };
}

// Fn: () -> Ty
template <typename Fn>
JsGetterCallback bindStaticGetter(Fn&& fn) {
    return [f = std::forward<Fn>(fn)]() -> Local<JsValue> { return ConvertToJs(std::invoke(f)); };
}

// Fn: (Ty) -> void
template <typename Fn>
JsSetterCallback bindStaticSetter(Fn&& fn) {
    using Ty = ArgType_t<Fn>;
    return [f = std::forward<Fn>(fn)](Local<JsValue> const& value) { std::invoke(f, ConvertToCpp<Ty>(value)); };
}

template <typename Ty>
std::pair<JsGetterCallback, JsSetterCallback> bindStaticProperty(Ty* p) {
    if constexpr (std::is_const_v<Ty>) {
        return {
            bindStaticGetter([p]() -> Ty { return *p; }),
            nullptr // const
        };
    } else {
        return {bindStaticGetter([p]() -> Ty { return *p; }), bindStaticSetter([p](Ty val) { *p = val; })};
    }
}


// Instance
template <typename C, typename... Args>
JsInstanceConstructor bindInstanceConstructor() {
    return [](Arguments const& args) -> void* {
        if constexpr (sizeof...(Args) == 0) {
            static_assert(
                HasDefaultConstructor<C>,
                "Class C must have a no-argument constructor; otherwise, a constructor must be specified."
            );
            if (args.length() != 0) return nullptr; // Parameter mismatch
            return new C();

        } else {
            constexpr size_t N = sizeof...(Args);
            if (args.length() != N) return nullptr; // Parameter mismatch

            using Tuple = std::tuple<Args...>;

            auto parameters = ConvertArgsToTuple<Tuple>(args, std::make_index_sequence<N>());
            return std::apply(
                [](auto&&... unpackedArgs) { return new C(std::forward<decltype(unpackedArgs)>(unpackedArgs)...); },
                std::move(parameters)
            );
        }
    };
}

template <typename C, typename Func>
JsInstanceMethodCallback bindInstanceMethod(Func&& fn) {
    return [f = std::forward<Func>(fn)](void* inst, const Arguments& args) -> Local<JsValue> {
        using Traits       = FunctionTraits<std::decay_t<Func>>;
        using R            = typename Traits::ReturnType;
        using Tuple        = typename Traits::ArgsTuple;
        constexpr size_t N = std::tuple_size_v<Tuple>;

        if (args.length() != N) {
            throw JsException("argument count mismatch");
        }

        auto typedInstance = static_cast<C*>(inst);

        if constexpr (std::is_void_v<R>) {
            std::apply(
                [typedInstance, &f](auto&&... unpackedArgs) {
                    (typedInstance->*f)(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
                },
                ConvertArgsToTuple<Tuple>(args, std::make_index_sequence<N>())
            );
            return JsUndefined::newUndefined();
        } else {
            decltype(auto) ret = std::apply(
                [typedInstance, &f](auto&&... unpackedArgs) -> R {
                    return (typedInstance->*f)(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
                },
                ConvertArgsToTuple<Tuple>(args, std::make_index_sequence<N>())
            );
            return ConvertToJs(ret);
        }
    };
}

template <typename C, typename... Func>
JsInstanceMethodCallback bindInstanceOverloadedMethod(Func&&... funcs) {
    std::vector functions = {bindInstanceMethod(std::forward<Func>(funcs))...};
    return [fs = std::move(functions)](void* inst, Arguments const& args) -> Local<JsValue> {
        for (size_t i = 0; i < sizeof...(Func); ++i) {
            try {
                return std::invoke(fs[i], inst, args);
            } catch (JsException const&) {
                if (i == sizeof...(Func) - 1) {
                    throw JsException{"no overload found"};
                }
            }
        }
        return {}; // undefined
    };
}

// Fn: (C*) -> Ty
template <typename C, typename Fn>
JsInstanceGetterCallback bindInstanceGetter(Fn&& fn) {
    return [f = std::forward<Fn>(fn)](void* inst, Arguments const& /* args */) -> Local<JsValue> {
        return ConvertToJs(std::invoke(f, static_cast<C*>(inst)));
    };
}

// Fn: (void* inst, Ty val) -> void
template <typename C, typename Fn>
JsInstanceSetterCallback bindInstanceSetter(Fn&& fn) {
    using Ty = ArgNType<Fn, 1>; // (void* inst, Ty val)
    return [f = std::forward<Fn>(fn)](void* inst, Arguments const& args) -> void {
        std::invoke(f, static_cast<C*>(inst), ConvertToCpp<Ty>(args[0]));
    };
}

template <typename C, typename Ty>
std::pair<JsInstanceGetterCallback, JsInstanceSetterCallback> bindInstanceProperty(Ty C::* prop) {
    if constexpr (std::is_const_v<Ty>) {
        return {
            bindInstanceGetter<C>([prop](C* inst) -> Ty { return inst->*prop; }),
            nullptr // const
        };
    } else {
        return {
            bindInstanceGetter<C>([prop](C* inst) -> Ty { return inst->*prop; }),
            bindInstanceSetter<C>([prop](C* inst, Ty val) -> void { inst->*prop = val; })
        };
    }
}

} // namespace internal


} // namespace v8wrap