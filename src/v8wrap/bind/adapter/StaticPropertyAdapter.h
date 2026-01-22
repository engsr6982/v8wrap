#pragma once
#include "v8wrap/Types.h"
#include "v8wrap/traits/FunctionTraits.h"
#include "v8wrap/traits/TypeTraits.h"
#include "v8wrap/types/Value.h"

#include "v8wrap/bind/TypeConverter.h"

namespace v8wrap::bind::adapter {

// Fn: () -> Ty
template <typename Fn>
GetterCallback bindStaticGetter(Fn&& fn) {
    return [f = std::forward<Fn>(fn)]() -> Local<Value> { return ConvertToJs(std::invoke(f)); };
}

// Fn: (Ty) -> void
template <typename Fn>
SetterCallback bindStaticSetter(Fn&& fn) {
    using Ty = traits::ArgumentType_t<Fn, 0>;
    return [f = std::forward<Fn>(fn)](Local<Value> const& value) { std::invoke(f, ConvertToCpp<Ty>(value)); };
}

template <typename Ty>
std::pair<GetterCallback, SetterCallback> bindStaticProperty(Ty* p) {
    // 原则上，我们需要实现引用机制，来保证 JavaScript 中的对象引用机制。
    // 但是，对于静态属性，我们没有可以与之关联的Js对象(Object)，这就无法创建引用。
    // 所以对于静态属性，qjspp 只能对其进行拷贝传递。
    static_assert(
        std::copyable<traits::RawType_t<Ty>>,
        "Static property must be copyable; otherwise, a getter/setter must be specified."
    );
    if constexpr (std::is_const_v<Ty>) {
        return {
            bindStaticGetter([p]() -> Ty { return *p; }),
            nullptr // const
        };
    } else {
        return {bindStaticGetter([p]() -> Ty { return *p; }), bindStaticSetter([p](Ty val) { *p = val; })};
    }
}


} // namespace v8wrap::bind::adapter