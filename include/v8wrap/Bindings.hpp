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
    using OwnedHolderCtor    = std::function<void*(void* rawInstance)>;
    using OwnedHolderGetter  = std::function<void*(void* holder, JsRuntime*)>;
    using OwnedHolderDeleter = std::function<void(void* holder)>;
    using ViewHolderCtor     = OwnedHolderCtor;
    using ViewHolderGetter   = OwnedHolderGetter;
    using ViewHolderDeleter  = OwnedHolderDeleter;

    OwnedHolderCtor const    mOwnedHolderCtor{nullptr};    // required
    OwnedHolderGetter const  mOwnedHolderGetter{nullptr};  // required
    OwnedHolderDeleter const mOwnedHolderDeleter{nullptr}; // required
    ViewHolderCtor const     mViewHolderCtor{nullptr};     // optional(When not using the relevant API)
    ViewHolderGetter const   mViewHolderGetter{nullptr};   // optional(When not using the relevant API)
    ViewHolderDeleter const  mViewHolderDeleter{nullptr};  // optional(When not using the relevant API)

    [[nodiscard]] inline bool hasViewHelperFunc() const {
        return mViewHolderCtor != nullptr && mViewHolderGetter != nullptr;
    }

    [[nodiscard]] inline void* wrapOwned(void* rawInstance) const { return mOwnedHolderCtor(rawInstance); }
    [[nodiscard]] inline void* extractOwned(void* holder, JsRuntime* rt) const {
        return mOwnedHolderGetter(holder, rt);
    }
    inline void freeOwned(void* holder) const { mOwnedHolderDeleter(holder); }

    [[nodiscard]] inline void* wrapView(void* rawInstance) const { return mViewHolderCtor(rawInstance); }
    [[nodiscard]] inline void* extractView(void* holder, JsRuntime* rt) const { return mViewHolderGetter(holder, rt); }
    inline void                freeView(void* holder) const { mViewHolderDeleter(holder); }

private:
    explicit ClassBinding(
        std::string         name,
        StaticBinding       static_,
        InstanceBinding     instance,
        ClassBinding const* parent,
        OwnedHolderCtor     ownedHolderCtor,
        OwnedHolderGetter   ownedHolderGetter,
        OwnedHolderDeleter  ownedHolderDeleter,
        ViewHolderCtor      viewHolderCtor,
        ViewHolderGetter    viewHolderGetter,
        ViewHolderDeleter   viewHolderDeleter
    );

    template <typename, typename, typename>
    friend struct ClassBindingBuilder;
};


template <typename T>
constexpr size_t size_of_v = sizeof(T);
template <>
constexpr size_t size_of_v<void> = 0;


template <typename Class, typename OH = void, typename VH = void>
struct ClassBindingBuilder {
    // 确保 C 与 H 一致：要么都是 void（用于静态类），要么都不是 void（用于实例类）
    // Ensure C and H are either both void (for static class) or both non-void (for instance class)
    static_assert(
        std::is_void_v<Class> == std::is_void_v<OH>,
        "C and H must both be void (for static class) or both non-void (for instance class)"
    );

    // 实例类要求 H 满足 Holder<H, C> 的概念约束
    // Instance classes require H to satisfy the Holder<H, C> concept
    static_assert(
        std::is_void_v<Class> || OwnedHolder<OH, Class>,
        "For instance class, H must satisfy Holder<H, C> concept"
    );

private:
    std::string                            mClassName;
    std::vector<StaticBinding::Property>   mStaticProperty;
    std::vector<StaticBinding::Function>   mStaticFunctions;
    JsInstanceConstructor                  mInstanceConstructor;
    std::vector<InstanceBinding::Property> mInstanceProperty;
    std::vector<InstanceBinding::Method>   mInstanceFunctions;
    ClassBinding const*                    mExtends         = nullptr;
    bool const                             mIsInstanceClass = !std::is_void_v<Class> && !std::is_void_v<OH>;

public:
    explicit ClassBindingBuilder(std::string className) : mClassName(std::move(className)) {}

    // 注册静态方法（已包装的 JsFunctionCallback） / Register static function (already wrapped)
    template <typename Fn>
        requires(IsJsFunctionCallback<Fn>)
    ClassBindingBuilder<Class, OH, VH>& function(std::string name, Fn&& fn) {
        mStaticFunctions.emplace_back(std::move(name), std::forward<Fn>(fn));
        return *this;
    }

    // 注册静态方法（自动包装） / Register static function (wrap C++ callable)
    template <typename Fn>
        requires(!IsJsFunctionCallback<Fn>)
    ClassBindingBuilder<Class, OH, VH>& function(std::string name, Fn&& fn) {
        mStaticFunctions.emplace_back(std::move(name), internal::bindStaticFunction(std::forward<Fn>(fn)));
        return *this;
    }

    // 注册重载静态方法 / Register overloaded static functions
    template <typename... Fn>
        requires(sizeof...(Fn) > 1 && (!IsJsFunctionCallback<Fn> && ...))
    ClassBindingBuilder<Class, OH, VH>& function(std::string name, Fn&&... fn) {
        mStaticFunctions.emplace_back(std::move(name), internal::bindStaticOverloadedFunction(std::forward<Fn>(fn)...));
        return *this;
    }

    // 注册静态属性（回调形式）/ Static property with raw callback
    ClassBindingBuilder<Class, OH, VH>&
    property(std::string name, JsGetterCallback getter, JsSetterCallback setter = nullptr) {
        mStaticProperty.emplace_back(std::move(name), std::move(getter), std::move(setter));
        return *this;
    }

    // 静态属性（变量指针）/ Static property from global/static variable pointer
    template <typename Ty>
    ClassBindingBuilder<Class, OH, VH>& property(std::string name, Ty* member) {
        auto gs = internal::bindStaticProperty<Ty>(member);
        mStaticProperty.emplace_back(std::move(name), std::move(gs.first), std::move(gs.second));
        return *this;
    }


    /* Instance Interface */
    /**
     * 绑定默认构造函数。必须可被指定参数调用。
     * Bind a default constructor. C must be constructible with specified arguments.
     */
    template <typename... Args>
        requires(!std::is_void_v<Class>)
    ClassBindingBuilder<Class, OH, VH>& constructor() {
        static_assert(
            !std::is_aggregate_v<Class> && std::is_constructible_v<Class, Args...>,
            "Constructor must be callable with the specified arguments"
        );
        if (mInstanceConstructor) throw std::logic_error("Constructor has already been registered!");
        mInstanceConstructor = internal::bindInstanceConstructor<Class, Args...>();
        return *this;
    }

    /**
     * 自定义构造逻辑。返回对象指针。
     * Register a custom constructor. Should return a pointer to the instance.
     */
    template <typename T = Class>
        requires(!std::is_void_v<T>)
    ClassBindingBuilder<Class, OH, VH>& customConstructor(JsInstanceConstructor ctor) {
        if (mInstanceConstructor) throw std::logic_error("Constructor has already been registered!");
        mInstanceConstructor = std::move(ctor);
        return *this;
    }

    /**
     * 禁用构造函数，使 JavaScript 无法构造此类。
     * Disable constructor from being called in JavaScript.
     */
    template <typename T = Class>
        requires(!std::is_void_v<T>)
    ClassBindingBuilder<Class, OH, VH>& disableConstructor() {
        if (mInstanceConstructor) throw std::logic_error("Constructor has already been registered!");
        mInstanceConstructor = [](Arguments const&) { return nullptr; };
        return *this;
    }

    // 注册实例方法（已包装）/ Instance method with JsInstanceMethodCallback
    template <typename Fn>
        requires(!std::is_void_v<Class> && IsJsInstanceMethodCallback<Fn>)
    ClassBindingBuilder<Class, OH, VH>& instanceMethod(std::string name, Fn&& fn) {
        mInstanceFunctions.emplace_back(std::move(name), std::forward<Fn>(fn));
        return *this;
    }

    // 实例方法（自动包装）/ Instance method with automatic binding
    template <typename Fn>
        requires(!std::is_void_v<Class> && !IsJsInstanceMethodCallback<Fn> && std::is_member_function_pointer_v<Fn>)
    ClassBindingBuilder<Class, OH, VH>& instanceMethod(std::string name, Fn&& fn) {
        mInstanceFunctions.emplace_back(std::move(name), internal::bindInstanceMethod<Class>(std::forward<Fn>(fn)));
        return *this;
    }

    // 实例重载方法 / Overloaded instance methods
    template <typename... Fn>
        requires(
            !std::is_void_v<Class>
            && (sizeof...(Fn) > 1 && (!IsJsInstanceMethodCallback<Fn> && ...)
                && (std::is_member_function_pointer_v<Fn> && ...))
        )
    ClassBindingBuilder<Class, OH, VH>& instanceMethod(std::string name, Fn&&... fn) {
        mInstanceFunctions.emplace_back(
            std::move(name),
            internal::bindInstanceOverloadedMethod<Class>(std::forward<Fn>(fn)...)
        );
        return *this;
    }

    // 实例属性（回调）/ Instance property with callbacks
    ClassBindingBuilder<Class, OH, VH>&
    instanceProperty(std::string name, JsInstanceGetterCallback getter, JsInstanceSetterCallback setter = nullptr) {
        static_assert(!std::is_void_v<Class>, "Only instance class can have instanceProperty");
        mInstanceProperty.emplace_back(std::move(name), std::move(getter), std::move(setter));
        return *this;
    }

    // 实例属性（成员变量）/ Instance property from T C::* member
    template <typename Member>
        requires(!std::is_void_v<Class> && std::is_member_object_pointer_v<Member>)
    ClassBindingBuilder<Class, OH, VH>& instanceProperty(std::string name, Member member) {
        auto gs = internal::bindInstanceProperty<Class>(std::forward<Member>(member));
        mInstanceProperty.emplace_back(std::move(name), std::move(gs.first), std::move(gs.second));
        return *this;
    }

    // 设置继承关系 / Set base class
    ClassBindingBuilder<Class, OH, VH>& extends(ClassBinding const& parent) {
        static_assert(!std::is_void_v<Class>, "Only instance classes can set up inheritance.");
        mExtends = &parent;
        return *this;
    }

    ClassBinding build() {
        if (mIsInstanceClass && !mInstanceConstructor) {
            throw std::logic_error("Instance class must have a constructor!");
        }

        ClassBinding::OwnedHolderCtor    ownedCtor    = nullptr;
        ClassBinding::OwnedHolderGetter  ownedGetter  = nullptr;
        ClassBinding::OwnedHolderDeleter ownedDeleter = nullptr;
        ClassBinding::ViewHolderCtor     viewCtor     = nullptr;
        ClassBinding::ViewHolderGetter   viewGetter   = nullptr;
        ClassBinding::ViewHolderDeleter  viewDeleter  = nullptr;

        if constexpr (!std::is_void_v<Class> && !std::is_void_v<OH>) {
            ownedCtor = [](void* instance) -> void* {
                auto typed  = static_cast<Class*>(instance);
                auto holder = new OH(typed); // create holder
                return static_cast<void*>(holder);
            };
            ownedGetter = [](void* holder, JsRuntime* rt) -> void* { return static_cast<OH*>(holder)->operator()(rt); };
            ownedDeleter = [](void* holder) { delete static_cast<OH*>(holder); };
            if constexpr (!std::is_void_v<VH>) {
                viewCtor = [](void* instance) -> void* {
                    auto typed  = static_cast<Class*>(instance);
                    auto holder = new VH(typed); // create holder
                    return static_cast<void*>(holder);
                };
                viewGetter = [](void* holder, JsRuntime* rt) -> void* {
                    return static_cast<VH*>(holder)->operator()(rt);
                };
                viewDeleter = [](void* holder) { delete static_cast<VH*>(holder); };
            }
        }


        return ClassBinding{
            std::move(mClassName),
            StaticBinding{std::move(mStaticProperty), std::move(mStaticFunctions)},
            InstanceBinding{std::move(mInstanceConstructor), std::move(mInstanceProperty), std::move(mInstanceFunctions), size_of_v<Class>},
            mExtends,
            std::move(ownedCtor),
            std::move(ownedGetter),
            std::move(ownedDeleter),
            std::move(viewCtor),
            std::move(viewGetter),
            std::move(viewDeleter),
        };
    }
};


template <typename C, typename H = void, typename V = void>
inline ClassBindingBuilder<C, H, V> bindingClass(std::string className) {
    return ClassBindingBuilder<C, H, V>(std::move(className));
}


} // namespace v8wrap

#include "v8wrap/Bindings.inl" // include implementation
