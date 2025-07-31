#pragma once
#include "v8wrap/Concepts.hpp"
#include "v8wrap/Types.hpp"


namespace v8wrap {


namespace internal {

// Forward statement

template <typename Func>
JsFunctionCallback bindStaticFunction(Func&& func);

template <typename... Func>
JsFunctionCallback bindStaticOverloadedFunction(Func&&... funcs);

template <typename Fn>
JsGetterCallback bindStaticGetter(Fn&& fn);

template <typename Fn>
JsSetterCallback bindStaticSetter(Fn&& fn);

template <typename Ty>
std::pair<JsGetterCallback, JsSetterCallback> bindStaticProperty(Ty* p);


} // namespace internal


struct StaticBinding {
    struct Property {
        std::string const      mName;
        JsGetterCallback const mGetter;
        JsSetterCallback const mSetter;

        explicit Property(std::string name, JsGetterCallback getter, JsSetterCallback setter)
        : mName(std::move(name)),
          mGetter(std::move(getter)),
          mSetter(std::move(setter)) {}
    };

    struct Function {
        std::string const        mName;
        JsFunctionCallback const mCallback;

        explicit Function(std::string name, JsFunctionCallback callback)
        : mName(std::move(name)),
          mCallback(std::move(callback)) {}
    };

    std::vector<Property> const mProperty;
    std::vector<Function> const mFunctions;

    explicit StaticBinding(std::vector<Property> property, std::vector<Function> functions)
    : mProperty(std::move(property)),
      mFunctions(std::move(functions)) {}
};

struct InstanceBinding {
    struct Property {
        std::string const              mName;
        JsInstanceGetterCallback const mGetter;
        JsInstanceSetterCallback const mSetter;

        explicit Property(std::string name, JsInstanceGetterCallback getter, JsInstanceSetterCallback setter)
        : mName(std::move(name)),
          mGetter(std::move(getter)),
          mSetter(std::move(setter)) {}
    };

    struct Function {
        std::string const                mName;
        JsInstanceFunctionCallback const mCallback;

        explicit Function(std::string name, JsInstanceFunctionCallback callback)
        : mName(std::move(name)),
          mCallback(std::move(callback)) {}
    };

    JsInstanceConstructor const mConstructor;
    std::vector<Property> const mProperty;
    std::vector<Function> const mFunctions;

    explicit InstanceBinding(
        JsInstanceConstructor constructor,
        std::vector<Property> property,
        std::vector<Function> functions
    )
    : mConstructor(std::move(constructor)),
      mProperty(std::move(property)),
      mFunctions(std::move(functions)) {}
};


class ClassBinding {
public:
    std::string const     mClassName;
    StaticBinding const   mStaticBinding;
    InstanceBinding const mInstanceBinding;
    ClassBinding const*   mExtends;

    explicit ClassBinding(std::string name, StaticBinding static_, InstanceBinding instance, ClassBinding const* parent)
    : mClassName(std::move(name)),
      mStaticBinding(std::move(static_)),
      mInstanceBinding(std::move(instance)),
      mExtends(parent) {}
};


template <typename T = void>
struct ClassBindingBuilder {
private:
    std::string                            mClassName;
    std::vector<StaticBinding::Property>   mStaticProperty;
    std::vector<StaticBinding::Function>   mStaticFunctions;
    JsInstanceConstructor                  mInstanceConstructor;
    std::vector<InstanceBinding::Property> mInstanceProperty;
    std::vector<InstanceBinding::Function> mInstanceFunctions;

public:
    explicit ClassBindingBuilder(std::string className) : mClassName(std::move(className)) {}

    template <typename Fn>
        requires(IsJsFunctionCallback<Fn>)
    ClassBindingBuilder& function(std::string name, Fn&& fn) {
        mStaticFunctions.emplace_back(std::move(name), std::forward<Fn>(fn));
        return *this;
    }

    template <typename Fn>
        requires(!IsJsFunctionCallback<Fn>)
    ClassBindingBuilder& function(std::string name, Fn&& fn) {
        mStaticFunctions.emplace_back(std::move(name), v8wrap::internal::bindStaticFunction(std::forward<Fn>(fn)));
        return *this;
    }

    template <typename... Fn>
        requires(sizeof...(Fn) > 1 && (!IsJsFunctionCallback<Fn> && ...))
    ClassBindingBuilder& function(std::string name, Fn&&... fn) {
        mStaticFunctions.emplace_back(
            std::move(name),
            v8wrap::internal::bindStaticOverloadedFunction(std::forward<Fn>(fn)...)
        );
        return *this;
    }

    ClassBindingBuilder& property(std::string name, JsGetterCallback getter, JsSetterCallback setter) {
        mStaticProperty.emplace_back(std::move(name), std::move(getter), std::move(setter));
        return *this;
    }

    ClassBindingBuilder& property(std::string name, JsGetterCallback getter) {
        mStaticProperty.emplace_back(std::move(name), std::move(getter), nullptr);
        return *this;
    }

    template <typename Ty>
    ClassBindingBuilder& property(std::string name, Ty* mem) {
        auto gs = internal::bindStaticProperty<Ty>(mem);
        mStaticProperty.emplace_back(std::move(name), std::move(gs.first), std::move(gs.second));
        return *this;
    }

    // template <typename>
    // ClassBindingBuilder* constructor() {}

    // template <typename>
    // ClassBindingBuilder& instanceProperty() {}

    // template <typename>
    // ClassBindingBuilder& instanceFunction() {}

    // ClassBindingBuilder& extends(ClassBinding<P> const& parent) {}

    ClassBinding build() {
        return ClassBinding{
            std::move(mClassName),
            StaticBinding{std::move(mStaticProperty), std::move(mStaticFunctions)},
            InstanceBinding{
                          std::move(mInstanceConstructor),
                          std::move(mInstanceProperty),
                          std::move(mInstanceFunctions)
            },
            nullptr
        };
    }
};


template <typename T>
inline ClassBindingBuilder<T> bindingClass(std::string className) {
    return ClassBindingBuilder<T>(std::move(className));
}


} // namespace v8wrap

#include "v8wrap/Bindings.inl" // include implementation
