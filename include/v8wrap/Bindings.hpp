#pragma once
#include "v8wrap/Concepts.hpp"
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


struct WrappedResource final {
    using ResGetter  = std::function<void*(void* resource)>; // return instance (T* -> void*)
    using ResDeleter = std::function<void(void* resource)>;

private:
    void*            resource{nullptr};
    ResGetter const  getter{nullptr};
    ResDeleter const deleter{nullptr};

    // internal use only
    ClassBinding const* binding{nullptr};
    JsRuntime const*    runtime{nullptr};
    bool const          constructFromJs{false};
    friend class JsRuntime;

public:
    inline void* operator()() const { return getter(resource); }

    V8WRAP_DISALLOW_COPY(WrappedResource);
    explicit WrappedResource() = delete;
    explicit WrappedResource(void* resource, ResGetter getter, ResDeleter deleter);
    ~WrappedResource(); // Call the deleter and pass in the resource.

    template <typename... Args>
        requires std::constructible_from<WrappedResource, Args...>
    static inline std::unique_ptr<WrappedResource> make(Args&&... args) {
        return std::make_unique<WrappedResource>(std::forward<Args>(args)...);
    }
};

class ClassBinding {
public:
    std::string const     mClassName;
    StaticBinding const   mStaticBinding;
    InstanceBinding const mInstanceBinding;
    ClassBinding const*   mExtends{nullptr};

    [[nodiscard]] bool hasInstanceConstructor() const;

    // 由于采用 void* 提升了运行时的灵活性，但缺少了类型信息。
    // delete void* 是不安全的，所以需要此辅助方法，以生成合理的 deleter。
    // 此回调仅在 JavaScript new 时调用，用于包装 JsInstanceConstructor 返回的实例 (T*)
    // The use of void* enhances runtime flexibility but lacks type information.
    // Deleting a void* is unsafe, so this helper method is needed to generate a reasonable deleter.
    // This callback is only invoked when using JavaScript's `new` operator, and it is used to wrap the instance (T*)
    //  returned by JsInstanceConstructor.
    using TypedWrappedResourceFactory = std::function<std::unique_ptr<WrappedResource>(void* instance)>;
    TypedWrappedResourceFactory const mJsNewInstanceWrapFactory{nullptr};

    [[nodiscard]] inline auto wrap(void* instance) const { return mJsNewInstanceWrapFactory(instance); }

private:
    explicit ClassBinding(
        std::string                 name,
        StaticBinding               static_,
        InstanceBinding             instance,
        ClassBinding const*         parent,
        TypedWrappedResourceFactory factory
    );

    template <typename>
    friend struct ClassBindingBuilder;
};


template <typename T>
constexpr size_t size_of_v = sizeof(T);
template <>
constexpr size_t size_of_v<void> = 0;


template <typename Class>
struct ClassBindingBuilder {
private:
    std::string                            mClassName;
    std::vector<StaticBinding::Property>   mStaticProperty;
    std::vector<StaticBinding::Function>   mStaticFunctions;
    JsInstanceConstructor                  mInstanceConstructor;
    std::vector<InstanceBinding::Property> mInstanceProperty;
    std::vector<InstanceBinding::Method>   mInstanceFunctions;
    ClassBinding const*                    mExtends         = nullptr;
    bool const                             mIsInstanceClass = !std::is_void_v<Class>;

public:
    explicit ClassBindingBuilder(std::string className) : mClassName(std::move(className)) {}

    // 注册静态方法（已包装的 JsFunctionCallback） / Register static function (already wrapped)
    template <typename Fn>
        requires(IsJsFunctionCallback<Fn>)
    ClassBindingBuilder<Class>& function(std::string name, Fn&& fn) {
        mStaticFunctions.emplace_back(std::move(name), std::forward<Fn>(fn));
        return *this;
    }

    // 注册静态方法（自动包装） / Register static function (wrap C++ callable)
    template <typename Fn>
        requires(!IsJsFunctionCallback<Fn>)
    ClassBindingBuilder<Class>& function(std::string name, Fn&& fn) {
        mStaticFunctions.emplace_back(std::move(name), internal::bindStaticFunction(std::forward<Fn>(fn)));
        return *this;
    }

    // 注册重载静态方法 / Register overloaded static functions
    template <typename... Fn>
        requires(sizeof...(Fn) > 1 && (!IsJsFunctionCallback<Fn> && ...))
    ClassBindingBuilder<Class>& function(std::string name, Fn&&... fn) {
        mStaticFunctions.emplace_back(std::move(name), internal::bindStaticOverloadedFunction(std::forward<Fn>(fn)...));
        return *this;
    }

    // 注册静态属性（回调形式）/ Static property with raw callback
    ClassBindingBuilder<Class>& property(std::string name, JsGetterCallback getter, JsSetterCallback setter = nullptr) {
        mStaticProperty.emplace_back(std::move(name), std::move(getter), std::move(setter));
        return *this;
    }

    // 静态属性（变量指针）/ Static property from global/static variable pointer
    template <typename Ty>
    ClassBindingBuilder<Class>& property(std::string name, Ty* member) {
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
    ClassBindingBuilder<Class>& constructor() {
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
    ClassBindingBuilder<Class>& customConstructor(JsInstanceConstructor ctor) {
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
    ClassBindingBuilder<Class>& disableConstructor() {
        if (mInstanceConstructor) throw std::logic_error("Constructor has already been registered!");
        mInstanceConstructor = [](Arguments const&) { return nullptr; };
        return *this;
    }

    // 注册实例方法（已包装）/ Instance method with JsInstanceMethodCallback
    template <typename Fn>
        requires(!std::is_void_v<Class> && IsJsInstanceMethodCallback<Fn>)
    ClassBindingBuilder<Class>& instanceMethod(std::string name, Fn&& fn) {
        mInstanceFunctions.emplace_back(std::move(name), std::forward<Fn>(fn));
        return *this;
    }

    // 实例方法（自动包装）/ Instance method with automatic binding
    template <typename Fn>
        requires(!std::is_void_v<Class> && !IsJsInstanceMethodCallback<Fn> && std::is_member_function_pointer_v<Fn>)
    ClassBindingBuilder<Class>& instanceMethod(std::string name, Fn&& fn) {
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
    ClassBindingBuilder<Class>& instanceMethod(std::string name, Fn&&... fn) {
        mInstanceFunctions.emplace_back(
            std::move(name),
            internal::bindInstanceOverloadedMethod<Class>(std::forward<Fn>(fn)...)
        );
        return *this;
    }

    // 实例属性（回调）/ Instance property with callbacks
    ClassBindingBuilder<Class>&
    instanceProperty(std::string name, JsInstanceGetterCallback getter, JsInstanceSetterCallback setter = nullptr) {
        static_assert(!std::is_void_v<Class>, "Only instance class can have instanceProperty");
        mInstanceProperty.emplace_back(std::move(name), std::move(getter), std::move(setter));
        return *this;
    }

    // 实例属性（成员变量）/ Instance property from T C::* member
    template <typename Member>
        requires(!std::is_void_v<Class> && std::is_member_object_pointer_v<Member>)
    ClassBindingBuilder<Class>& instanceProperty(std::string name, Member member) {
        auto gs = internal::bindInstanceProperty<Class>(std::forward<Member>(member));
        mInstanceProperty.emplace_back(std::move(name), std::move(gs.first), std::move(gs.second));
        return *this;
    }

    // 设置继承关系 / Set base class
    ClassBindingBuilder<Class>& extends(ClassBinding const& parent) {
        static_assert(!std::is_void_v<Class>, "Only instance classes can set up inheritance.");
        mExtends = &parent;
        return *this;
    }

    ClassBinding build() {
        if (mIsInstanceClass && !mInstanceConstructor) {
            throw std::logic_error("Instance class must have a constructor!");
        }

        ClassBinding::TypedWrappedResourceFactory factory = nullptr;
        if constexpr (!std::is_void_v<Class>) {
            factory = [](void* instance) -> std::unique_ptr<WrappedResource> {
                return WrappedResource::make(
                    instance,
                    [](void* res) -> void* { return res; },
                    [](void* res) -> void { delete static_cast<Class*>(res); }
                );
            };
        }

        return ClassBinding{
            std::move(mClassName),
            StaticBinding{std::move(mStaticProperty), std::move(mStaticFunctions)},
            InstanceBinding{std::move(mInstanceConstructor), std::move(mInstanceProperty), std::move(mInstanceFunctions), size_of_v<Class>},
            mExtends,
            std::move(factory)
        };
    }
};


template <typename C>
inline ClassBindingBuilder<C> bindingClass(std::string className) {
    return ClassBindingBuilder<C>(std::move(className));
}


} // namespace v8wrap

#include "v8wrap/Bindings.inl" // include implementation
