#pragma once
#include "v8wrap/Concepts.hpp"
#include "v8wrap/Native.hpp"
#include "v8wrap/Types.hpp"
#include <cstddef>
#include <stdexcept>
#include <type_traits>

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


template <typename C, typename... Args>
JsInstanceConstructor bindInstanceConstructor();

template <typename C, typename Func>
JsInstanceMethodCallback bindInstanceMethod(Func&& fn);

template <typename C, typename... Func>
JsInstanceMethodCallback bindInstanceOverloadedMethod(Func&&... funcs);

template <typename C, typename Fn>
JsInstanceGetterCallback bindInstanceGetter(Fn&& fn);

template <typename C, typename Fn>
JsInstanceSetterCallback bindInstanceSetter(Fn&& fn);

template <typename C, typename Ty>
std::pair<JsInstanceGetterCallback, JsInstanceSetterCallback> bindInstanceProperty(Ty C::* prop);


} // namespace internal


struct StaticBinding {
    struct Property {
        std::string const      mName;
        JsGetterCallback const mGetter;
        JsSetterCallback const mSetter;

        explicit Property(std::string name, JsGetterCallback getter, JsSetterCallback setter);
    };

    struct Function {
        std::string const        mName;
        JsFunctionCallback const mCallback;

        explicit Function(std::string name, JsFunctionCallback callback);
    };

    std::vector<Property> const mProperty;
    std::vector<Function> const mFunctions;

    explicit StaticBinding(std::vector<Property> property, std::vector<Function> functions);
};

struct InstanceBinding {
    struct Property {
        std::string const              mName;
        JsInstanceGetterCallback const mGetter;
        JsInstanceSetterCallback const mSetter;

        explicit Property(std::string name, JsInstanceGetterCallback getter, JsInstanceSetterCallback setter);
    };

    struct Method {
        std::string const              mName;
        JsInstanceMethodCallback const mCallback;

        explicit Method(std::string name, JsInstanceMethodCallback callback);
    };

    JsInstanceConstructor const mConstructor;
    std::vector<Property> const mProperty;
    std::vector<Method> const   mMethods;
    size_t const                mClassSize; // sizeof(C) for instance class

    explicit InstanceBinding(
        JsInstanceConstructor constructor,
        std::vector<Property> property,
        std::vector<Method>   functions,
        size_t                classSize
    );
};


class ClassBinding {
public:
    std::string const     mClassName;
    StaticBinding const   mStaticBinding;
    InstanceBinding const mInstanceBinding;
    ClassBinding const*   mExtends{nullptr};

    [[nodiscard]] bool hasInstanceConstructor() const;

    // For storage needs, the function erasure Holder is used here.
    using HolderCtor    = std::function<void*(void* rawInstance)>;        // required
    using HolderGetter  = std::function<void*(void* holder, JsRuntime*)>; // required
    using HolderDeleter = std::function<void(void* holder)>;              // optional

    HolderCtor const    mHolderCtor;
    HolderGetter const  mHolderGetter;
    HolderDeleter const mHolderDeleter;

    [[nodiscard]] inline void* wrapInstance(void* rawInstance) const { return mHolderCtor(rawInstance); }
    [[nodiscard]] inline void* unwrapInstance(void* holder, JsRuntime* rt) const { return mHolderGetter(holder, rt); }
    inline void                deleteHolderAndInstance(void* holder) const { mHolderDeleter(holder); }

private:
    explicit ClassBinding(
        std::string         name,
        StaticBinding       static_,
        InstanceBinding     instance,
        ClassBinding const* parent,
        HolderCtor          holderCtor,
        HolderGetter        holderGetter,
        HolderDeleter       holderDeleter
    );

    template <typename, typename>
    friend struct ClassBindingBuilder;
};


template <typename T>
constexpr size_t size_of_v = sizeof(T);
template <>
constexpr size_t size_of_v<void> = 0;


template <typename C = void, typename H = void>
struct ClassBindingBuilder {
    // 确保 C 与 H 一致：要么都是 void（用于静态类），要么都不是 void（用于实例类）
    // Ensure C and H are either both void (for static class) or both non-void (for instance class)
    static_assert(
        std::is_void_v<C> == std::is_void_v<H>,
        "C and H must both be void (for static class) or both non-void (for instance class)"
    );

    // 实例类要求 H 满足 Holder<H, C> 的概念约束
    // Instance classes require H to satisfy the Holder<H, C> concept
    static_assert(std::is_void_v<C> || Holder<H, C>, "For instance class, H must satisfy Holder<H, C> concept");

private:
    std::string                            mClassName;
    std::vector<StaticBinding::Property>   mStaticProperty;
    std::vector<StaticBinding::Function>   mStaticFunctions;
    JsInstanceConstructor                  mInstanceConstructor;
    std::vector<InstanceBinding::Property> mInstanceProperty;
    std::vector<InstanceBinding::Method>   mInstanceFunctions;
    ClassBinding const*                    mExtends         = nullptr;
    bool const                             mIsInstanceClass = !std::is_void_v<C> && !std::is_void_v<H>;

public:
    explicit ClassBindingBuilder(std::string className) : mClassName(std::move(className)) {}

    // 注册静态方法（已包装的 JsFunctionCallback） / Register static function (already wrapped)
    template <typename Fn>
        requires(IsJsFunctionCallback<Fn>)
    ClassBindingBuilder<C, H>& function(std::string name, Fn&& fn) {
        mStaticFunctions.emplace_back(std::move(name), std::forward<Fn>(fn));
        return *this;
    }

    // 注册静态方法（自动包装） / Register static function (wrap C++ callable)
    template <typename Fn>
        requires(!IsJsFunctionCallback<Fn>)
    ClassBindingBuilder<C, H>& function(std::string name, Fn&& fn) {
        mStaticFunctions.emplace_back(std::move(name), internal::bindStaticFunction(std::forward<Fn>(fn)));
        return *this;
    }

    // 注册重载静态方法 / Register overloaded static functions
    template <typename... Fn>
        requires(sizeof...(Fn) > 1 && (!IsJsFunctionCallback<Fn> && ...))
    ClassBindingBuilder<C, H>& function(std::string name, Fn&&... fn) {
        mStaticFunctions.emplace_back(std::move(name), internal::bindStaticOverloadedFunction(std::forward<Fn>(fn)...));
        return *this;
    }

    // 注册静态属性（回调形式）/ Static property with raw callback
    ClassBindingBuilder<C, H>& property(std::string name, JsGetterCallback getter, JsSetterCallback setter = nullptr) {
        mStaticProperty.emplace_back(std::move(name), std::move(getter), std::move(setter));
        return *this;
    }

    // 静态属性（变量指针）/ Static property from global/static variable pointer
    template <typename Ty>
    ClassBindingBuilder<C, H>& property(std::string name, Ty* member) {
        auto gs = internal::bindStaticProperty<Ty>(member);
        mStaticProperty.emplace_back(std::move(name), std::move(gs.first), std::move(gs.second));
        return *this;
    }

    // 静态属性（仅 getter）/ Static property with only getter
    template <typename G>
        requires(!IsJsGetterCallback<G>)
    ClassBindingBuilder<C, H>& property(std::string name, G&& getter) {
        mStaticProperty.emplace_back(std::move(name), internal::bindStaticGetter(std::forward<G>(getter)), nullptr);
        return *this;
    }

    // 静态属性（getter + setter）/ Static property with getter and setter
    template <typename G, typename S>
        requires(!IsJsGetterCallback<G> && !IsJsSetterCallback<S>)
    ClassBindingBuilder<C, H>& property(std::string name, G&& getter, S&& setter) {
        mStaticProperty.emplace_back(
            std::move(name),
            internal::bindStaticGetter(std::forward<G>(getter)),
            internal::bindStaticSetter(std::forward<S>(setter))
        );
        return *this;
    }


    /* Instance Interface */
    /**
     * 绑定默认构造函数。必须可被指定参数调用。
     * Bind a default constructor. C must be constructible with specified arguments.
     */
    template <typename... Args>
        requires(!std::is_void_v<C>)
    ClassBindingBuilder<C, H>& constructor() {
        static_assert(
            !std::is_aggregate_v<C> && std::is_constructible_v<C, Args...>,
            "Constructor must be callable with the specified arguments"
        );
        if (mInstanceConstructor) throw std::logic_error("Constructor has already been registered!");
        mInstanceConstructor = internal::bindInstanceConstructor<C, Args...>();
        return *this;
    }

    /**
     * 自定义构造逻辑。返回对象指针。
     * Register a custom constructor. Should return a pointer to the instance.
     */
    template <typename T = C>
        requires(!std::is_void_v<T>)
    ClassBindingBuilder<C, H>& customConstructor(JsInstanceConstructor ctor) {
        if (mInstanceConstructor) throw std::logic_error("Constructor has already been registered!");
        mInstanceConstructor = std::move(ctor);
        return *this;
    }

    /**
     * 禁用构造函数，使 JavaScript 无法构造此类。
     * Disable constructor from being called in JavaScript.
     */
    template <typename T = C>
        requires(!std::is_void_v<T>)
    ClassBindingBuilder<C, H>& disableConstructor() {
        if (mInstanceConstructor) throw std::logic_error("Constructor has already been registered!");
        mInstanceConstructor = [](Arguments const&) { return nullptr; };
        return *this;
    }

    // 注册实例方法（已包装）/ Instance method with JsInstanceMethodCallback
    template <typename Fn>
        requires(!std::is_void_v<C> && IsJsInstanceMethodCallback<Fn>)
    ClassBindingBuilder<C, H>& instanceMethod(std::string name, Fn&& fn) {
        mInstanceFunctions.emplace_back(std::move(name), std::forward<Fn>(fn));
        return *this;
    }

    // 实例方法（自动包装）/ Instance method with automatic binding
    template <typename Fn>
        requires(!std::is_void_v<C> && !IsJsInstanceMethodCallback<Fn>)
    ClassBindingBuilder<C, H>& instanceMethod(std::string name, Fn&& fn) {
        mInstanceFunctions.emplace_back(std::move(name), internal::bindInstanceMethod<C>(std::forward<Fn>(fn)));
        return *this;
    }

    // 实例重载方法 / Overloaded instance methods
    template <typename... Fn>
        requires(!std::is_void_v<C> && (sizeof...(Fn) > 1 && (!IsJsInstanceMethodCallback<Fn> && ...)))
    ClassBindingBuilder<C, H>& instanceMethod(std::string name, Fn&&... fn) {
        mInstanceFunctions.emplace_back(
            std::move(name),
            internal::bindInstanceOverloadedMethod<C>(std::forward<Fn>(fn)...)
        );
        return *this;
    }

    // 实例属性（回调）/ Instance property with callbacks
    ClassBindingBuilder<C, H>&
    instanceProperty(std::string name, JsInstanceGetterCallback getter, JsInstanceSetterCallback setter = nullptr) {
        static_assert(!std::is_void_v<C>, "Only instance class can have instanceProperty");
        mInstanceProperty.emplace_back(std::move(name), std::move(getter), std::move(setter));
        return *this;
    }

    // 实例属性（成员变量）/ Instance property from C::* member
    template <typename Member>
        requires(!std::is_void_v<C> && std::is_member_object_pointer_v<Member>)
    ClassBindingBuilder<C, H>& instanceProperty(std::string name, Member member) {
        auto gs = internal::bindInstanceProperty<C>(member);
        mInstanceProperty.emplace_back(std::move(name), std::move(gs.first), std::move(gs.second));
        return *this;
    }

    // 实例属性（仅 getter）/ Instance property with only getter
    template <typename G>
        requires(!std::is_void_v<C> && !IsJsInstanceGetterCallback<G> && !std::is_member_object_pointer_v<G>)
    ClassBindingBuilder<C, H>& instanceProperty(std::string name, G&& getter) {
        mInstanceProperty.emplace_back(std::move(name), internal::bindInstanceGetter(std::forward<G>(getter)), nullptr);
        return *this;
    }

    // 实例属性（getter + setter）/ Instance property with getter and setter
    template <typename G, typename S>
        requires(!std::is_void_v<C> && !IsJsInstanceGetterCallback<G> && !IsJsInstanceSetterCallback<S>)
    ClassBindingBuilder<C, H>& instanceProperty(std::string name, G&& getter, S&& setter) {
        mInstanceProperty.emplace_back(
            std::move(name),
            internal::bindInstanceGetter(std::forward<G>(getter)),
            internal::bindInstanceSetter(std::forward<S>(setter))
        );
        return *this;
    }

    // 设置继承关系 / Set base class
    ClassBindingBuilder<C, H>& extends(ClassBinding const& parent) {
        static_assert(!std::is_void_v<C>, "Only instance classes can set up inheritance.");
        mExtends = &parent;
        return *this;
    }

    ClassBinding build() {
        if (mIsInstanceClass && !mInstanceConstructor) {
            throw std::logic_error("Instance class must have a constructor!");
        }

        ClassBinding::HolderCtor    ctor    = nullptr;
        ClassBinding::HolderGetter  getter  = nullptr;
        ClassBinding::HolderDeleter deleter = nullptr;

        if constexpr (!std::is_void_v<H> && !std::is_void_v<C>) {
            ctor = [](void* instance) -> void* {
                C*    typedInstance = static_cast<C*>(instance);
                H*    holder        = new H(typedInstance);
                void* anyHolder     = static_cast<void*>(holder);
                return anyHolder;
            };
            getter  = [](void* holder, JsRuntime* rt) -> void* { return static_cast<H*>(holder)->operator()(rt); };
            deleter = [](void* holder) { delete static_cast<H*>(holder); };
        }

        return ClassBinding{
            std::move(mClassName),
            StaticBinding{std::move(mStaticProperty), std::move(mStaticFunctions)},
            InstanceBinding{std::move(mInstanceConstructor), std::move(mInstanceProperty), std::move(mInstanceFunctions), size_of_v<C>},
            mExtends,
            std::move(ctor),
            std::move(getter),
            std::move(deleter),
        };
    }
};


template <typename C, typename H = void>
inline ClassBindingBuilder<C, H> bindingClass(std::string className) {
    return ClassBindingBuilder<C, H>(std::move(className));
}


} // namespace v8wrap

#include "v8wrap/Bindings.inl" // include implementation
