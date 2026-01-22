#pragma once
#include "v8wrap/Types.h"
#include "v8wrap/concepts/ScriptConcepts.h"
#include "v8wrap/runtime/Exception.h"
#include "v8wrap/traits/FunctionTraits.h"
#include "v8wrap/types/Value.h"

#include <cassert>

namespace v8wrap::bind::adapter {


template <typename C, typename Func>
InstanceMethodCallback bindInstanceMethod(Func&& fn) {
    if constexpr (concepts::JsInstanceMethodCallback<std::remove_cvref_t<Func>>) {
        return std::forward<Func>(fn); // 已是标准的回调，直接转发不需要进行绑定
    }
    return [f = std::forward<Func>(fn)](void* inst, const Arguments& args) -> Local<Value> {
        using Traits       = traits::FunctionTraits<std::decay_t<Func>>;
        using R            = typename Traits::ReturnType;
        using Tuple        = typename Traits::ArgsTuple;
        constexpr size_t N = std::tuple_size_v<Tuple>;

        if (args.length() != N) [[unlikely]] {
            throw Exception("argument count mismatch", Exception::Type::TypeError);
        }

        auto typedInstance = static_cast<C*>(inst);

        if constexpr (std::is_void_v<R>) {
            std::apply(
                [typedInstance, &f](auto&&... unpackedArgs) {
                    (typedInstance->*f)(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
                },
                ConvertArgsToTuple<Tuple>(args, std::make_index_sequence<N>())
            );
            return {}; // undefined
        } else {
            decltype(auto) ret = std::apply(
                [typedInstance, &f](auto&&... unpackedArgs) -> R {
                    return (typedInstance->*f)(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
                },
                ConvertArgsToTuple<Tuple>(args, std::make_index_sequence<N>())
            );
            // 特殊情况，对于 Builder 模式，返回 this
            if constexpr (std::is_same_v<R, C&>) {
                assert(args.hasThiz() && "this is required for Builder pattern");
                return args.thiz();
            } else {
                return ConvertToJs(ret);
            }
        }
    };
}

template <typename C, typename... Func>
InstanceMethodCallback bindInstanceOverloadedMethod(Func&&... funcs) {
    std::vector functions = {bindInstanceMethod<C>(std::forward<Func>(funcs))...};

    // TODO: consider optimizing overload dispatch (e.g. arg-count lookup)
    // if we ever hit cases with >3 overloads. Current linear dispatch is ideal
    // for small sets and keeps the common path fast.

    return [fs = std::move(functions)](void* inst, Arguments const& args) -> Local<Value> {
        for (size_t i = 0; i < sizeof...(Func); ++i) {
            try {
                return std::invoke(fs[i], inst, args);
            } catch (Exception const&) {
                if (i == sizeof...(Func) - 1) [[unlikely]] {
                    throw Exception{"no overload found", Exception::Type::TypeError};
                }
            }
        }
        return {}; // undefined
    };
}


} // namespace v8wrap::bind::adapter