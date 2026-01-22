#pragma once
#include "v8wrap/Types.h"
#include "v8wrap/traits/FunctionTraits.h"
#include "v8wrap/traits/TypeTraits.h"

#include "v8wrap/bind/TypeConverter.h"

namespace v8wrap::bind::adapter {

// Fn: (C*) -> Ty
template <typename C, typename Fn>
InstanceGetterCallback bindInstanceGetter(Fn&& fn) {
    return [f = std::forward<Fn>(fn)](void* inst, Arguments const& /* args */) -> Local<Value> {
        return ConvertToJs(std::invoke(f, static_cast<C*>(inst)));
    };
}

// Fn: (void* inst, Ty val) -> void
template <typename C, typename Fn>
InstanceSetterCallback bindInstanceSetter(Fn&& fn) {
    using Ty = traits::ArgumentType_t<Fn, 1>; // (void* inst, Ty val)
    return [f = std::forward<Fn>(fn)](void* inst, Arguments const& args) -> void {
        std::invoke(f, static_cast<C*>(inst), ConvertToCpp<Ty>(args[0]));
    };
}

template <typename C, typename Ty>
std::pair<InstanceGetterCallback, InstanceSetterCallback> bindInstanceProperty(Ty C::* member) {
    static_assert(
        std::copyable<traits::RawType_t<Ty>>,
        "bindInstanceProperty only supports copying properties, Ty does not support copying."
    );
    if constexpr (std::is_const_v<Ty>) {
        return {bindInstanceGetter<C>([member](C* inst) -> Ty { return inst->*member; }), nullptr};
    } else {
        return {
            bindInstanceGetter<C>([member](C* inst) -> Ty { return inst->*member; }),
            bindInstanceSetter<C>([member](C* inst, Ty val) -> void { inst->*member = val; })
        };
    }
}

template <typename C, typename Fn>
InstanceGetterCallback bindInstanceGetterRef(Fn&& fn, meta::ClassDefine const* def) {
    return [f = std::forward<Fn>(fn), def](void* inst, Arguments const& arguments) -> Local<Value> {
        using Ret = traits::FunctionTraits<std::decay_t<Fn>>::ReturnType;
        static_assert(std::is_pointer_v<Ret>, "InstanceGetterRef must return a pointer");

        // TODO: use RTTI to check type
        // auto typeId = reflection::getTypeId<traits::RawType_t<Ret>>();
        // if (typeId != def->typeId_) [[unlikely]] {
        //     throw JsException{
        //         JsException::Type::InternalError,
        //         "Type mismatch, ClassDefine::typeId_ and lambda return value are not the same type"
        //     };
        // }
        if (!arguments.hasThiz()) [[unlikely]] {
            throw Exception{
                "Cannot access class member; the current access does not have a valid 'this' reference.",
                Exception::Type::TypeError
            };
        }

        decltype(auto) result = std::invoke(f, static_cast<C*>(inst)); // const T* / T*

        void* unk = nullptr;
        if constexpr (std::is_const_v<Ret>) {
            unk = const_cast<void*>(static_cast<const void*>(result));
        } else {
            unk = static_cast<void*>(result);
        }
        // TODO: fix
        // return arguments.runtime()->newInstanceOfView(*def, unk, arguments.thiz());
        return {};
    };
}

template <typename C, typename Ty>
std::pair<InstanceGetterCallback, InstanceSetterCallback>
bindInstancePropertyRef(Ty C::* member, meta::ClassDefine const* def) {
    using Raw = traits::RawType_t<Ty>;
    if constexpr (concepts::JsPrimitiveConvertible<Raw> && std::copyable<Raw>) {
        return bindInstanceProperty<C>(std::forward<Ty C::*>(member)); // Value type, can be copied directly
    } else {
        if constexpr (std::is_const_v<Ty>) {
            return {
                bindInstanceGetterRef<C>([member](C* inst) -> Ty const* { return &(inst->*member); }, def),
                nullptr
            };
        } else {
            return {
                bindInstanceGetterRef<C>([member](C* inst) -> Ty* { return &(inst->*member); }, def),
                bindInstanceSetter<C>([member](C* inst, Ty* val) -> void { inst->*member = *val; })
            };
        }
    }
}


} // namespace v8wrap::bind::adapter