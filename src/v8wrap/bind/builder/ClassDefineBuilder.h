#pragma once
#include "v8wrap/Types.h"
#include "v8wrap/bind/adapter/ConstructorAdapter.h"
#include "v8wrap/bind/adapter/EqualsAdapter.h"
#include "v8wrap/bind/adapter/FunctionAdapter.h"
#include "v8wrap/bind/adapter/InstancePropertyAdapter.h"
#include "v8wrap/bind/adapter/MethodAdapter.h"
#include "v8wrap/bind/adapter/StaticPropertyAdapter.h"
#include "v8wrap/bind/meta/ClassDefine.h"
#include "v8wrap/bind/meta/MemberDefine.h"
#include "v8wrap/concepts/ScriptConcepts.h"
#include "v8wrap/traits/TypeTraits.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>


namespace v8wrap::bind {

enum class ConstructorState {
    None,    // 默认状态, 未设置状态
    Normal,  // 默认绑定构造
    Custom,  // 自定义/自行处理构造逻辑
    Disabled // 禁止Js构造,自动生成空构造回调
};

template <typename Class, ConstructorState State = ConstructorState::None>
struct ClassDefineBuilder {
private:
    std::string                                       className_;
    std::vector<meta::StaticMemberDefine::Property>   staticProperty_;
    std::vector<meta::StaticMemberDefine::Function>   staticFunctions_;
    std::vector<meta::InstanceMemberDefine::Property> instanceProperty_;
    std::vector<meta::InstanceMemberDefine::Method>   instanceFunctions_;
    meta::ClassDefine const*                          base_ = nullptr;

    InstanceConstructor                                          userDefinedConstructor_ = nullptr;
    std::unordered_map<size_t, std::vector<InstanceConstructor>> constructors_           = {};

    static constexpr bool isInstanceClass = !std::is_void_v<Class>;

    template <ConstructorState OtherState>
    explicit ClassDefineBuilder(ClassDefineBuilder<Class, OtherState>&& other) noexcept
    : className_(std::move(other.className_)),
      staticProperty_(std::move(other.staticProperty_)),
      staticFunctions_(std::move(other.staticFunctions_)),
      instanceProperty_(std::move(other.instanceProperty_)),
      instanceFunctions_(std::move(other.instanceFunctions_)),
      base_(other.base_),
      userDefinedConstructor_(std::move(other.userDefinedConstructor_)),
      constructors_(std::move(other.constructors_)) {
        // note: other may be in moved-from state
    }

    template <typename, ConstructorState>
    friend struct ClassDefineBuilder;

public:
    explicit ClassDefineBuilder(std::string className) : className_(std::move(className)) {}

    // 注册静态方法（已包装的 JsFunctionCallback） / Register static function (already wrapped)
    template <typename Fn>
    auto& function(std::string name, Fn&& fn)
        requires(concepts::JsFunctionCallback<Fn>)
    {
        staticFunctions_.emplace_back(std::move(name), std::forward<Fn>(fn));
        return *this;
    }

    // 注册静态方法（自动包装） / Register static function (wrap C++ callable)
    template <typename Fn>
    auto& function(std::string name, Fn&& fn)
        requires(!concepts::JsFunctionCallback<Fn>)
    {
        staticFunctions_.emplace_back(std::move(name), adapter::bindStaticFunction(std::forward<Fn>(fn)));
        return *this;
    }

    // 注册重载静态方法 / Register overloaded static functions
    template <typename... Fn>
    auto& function(std::string name, Fn&&... fn)
        requires(sizeof...(Fn) > 1)
    {
        staticFunctions_.emplace_back(std::move(name), adapter::bindStaticOverloadedFunction(std::forward<Fn>(fn)...));
        return *this;
    }

    // 注册静态属性（回调形式）/ Static property with raw callback
    auto& property(std::string name, GetterCallback getter, SetterCallback setter = nullptr) {
        staticProperty_.emplace_back(std::move(name), std::move(getter), std::move(setter));
        return *this;
    }

    // 静态属性（变量指针）/ Static property from global/static variable pointer
    // 注意：绑定生成的属性默认采用值传递（即：toJs、toCpp 均拷贝实例）
    // note: the default binding of the property uses value passing (i.e. toJs, toCpp all copy instances)
    template <typename Ty>
    auto& property(std::string name, Ty* member) {
        auto gs = adapter::bindStaticProperty<Ty>(member);
        staticProperty_.emplace_back(std::move(name), std::move(gs.first), std::move(gs.second));
        return *this;
    }


    /* Instance Interface */
    /**
     * 绑定默认构造函数。必须可被指定参数调用。
     * Bind a default constructor. C must be constructible with specified arguments.
     */
    template <typename... Args>
    auto constructor()
        requires(isInstanceClass && (State == ConstructorState::None || State == ConstructorState::Normal))
    {
        static_assert(
            // 非聚合类，用户传入的参数包必须能构造 Class，以避免意外的隐式行为
            !std::is_aggregate_v<Class> && std::is_constructible_v<Class, Args...>,
            "Constructor must be callable with the specified arguments"
        );
        constexpr size_t N = sizeof...(Args);
        constructors_[N].emplace_back(adapter::bindInstanceConstructor<Class, Args...>());

        if constexpr (State == ConstructorState::Normal) {
            return *this; // 对于多构造重载(多次调用constructor)直接返回引用
        } else {
            // Use an else statement to avoid the C4702 warning
            ClassDefineBuilder<Class, ConstructorState::Normal> newBuilder(std::move(*this));
            return newBuilder; // NRVO/move
        }
    }

    /**
     * 自定义构造逻辑。返回对象指针。
     * Register a custom constructor. Should return a pointer to the instance.
     * @note 对于构造失败，可以在函数内抛出 JsException 给脚本层，如果返回 nullptr 库会帮你抛出异常.
     * @note For construction failure, you can throw a JsException to the script layer in the function, and if you
     *       return nullptr, the library will help you throw an exception.
     */
    auto customConstructor(InstanceConstructor ctor)
        requires(isInstanceClass && State == ConstructorState::None)
    {
        userDefinedConstructor_ = std::move(ctor);
        ClassDefineBuilder<Class, ConstructorState::Custom> newBuilder(std::move(*this));
        return newBuilder; // NRVO/move
    }

    /**
     * 禁用构造函数，JavaScript 将无法通过 new 构造此类。
     * Disable the constructor, JavaScript will not be able to construct this class with new.
     * @note 设置为不可构造后，Builder 不会生成托管工厂(ManagedResourceFactory)
     * @note After setting it to non-constructible, the Builder will not generate a managed factory.
     */
    auto disableConstructor()
        requires(isInstanceClass && State == ConstructorState::None)
    {
        userDefinedConstructor_ = [](Arguments const&) { return nullptr; };
        ClassDefineBuilder<Class, ConstructorState::Disabled> newBuilder(std::move(*this));
        return newBuilder; // NRVO/move
    }

    // 注册实例方法（已包装）/ Instance method with JsInstanceMethodCallback
    template <typename Fn>
    auto& instanceMethod(std::string name, Fn&& fn)
        requires(isInstanceClass && concepts::JsInstanceMethodCallback<Fn>)
    {
        instanceFunctions_.emplace_back(std::move(name), std::forward<Fn>(fn));
        return *this;
    }

    // 实例方法（自动包装）/ Instance method with automatic binding
    template <typename Fn>
    auto& instanceMethod(std::string name, Fn&& fn)
        requires(isInstanceClass && !concepts::JsInstanceMethodCallback<Fn> && std::is_member_function_pointer_v<Fn>)
    {
        instanceFunctions_.emplace_back(std::move(name), adapter::bindInstanceMethod<Class>(std::forward<Fn>(fn)));
        return *this;
    }

    // 实例重载方法 / Overloaded instance methods
    template <typename... Fn>
    auto& instanceMethod(std::string name, Fn&&... fn)
        requires(isInstanceClass && sizeof...(Fn) > 1 && (std::is_member_function_pointer_v<Fn> && ...))
    {
        instanceFunctions_.emplace_back(
            std::move(name),
            adapter::bindInstanceOverloadedMethod<Class>(std::forward<Fn>(fn)...)
        );
        return *this;
    }

    // 实例属性（回调）/ Instance property with callbacks
    auto& instanceProperty(std::string name, InstanceGetterCallback getter, InstanceSetterCallback setter = nullptr)
        requires isInstanceClass
    {
        instanceProperty_.emplace_back(std::move(name), std::move(getter), std::move(setter));
        return *this;
    }

    // 实例属性（成员变量）/ Instance property from T C::* member
    // 注意：此回调适用于值类型成员，v8wrap 会进行拷贝传递
    template <typename Member>
    auto& instanceProperty(std::string name, Member member)
        requires(isInstanceClass && std::is_member_object_pointer_v<Member>)
    {
        auto gs = adapter::bindInstanceProperty<Class>(std::forward<Member>(member));
        instanceProperty_.emplace_back(std::move(name), std::move(gs.first), std::move(gs.second));
        return *this;
    }

    // 实例属性（成员变量，对象引用）/ Instance property from T C::* member with reference
    template <typename Member>
    auto& instancePropertyRef(std::string name, Member member, meta::ClassDefine const& def)
        requires(isInstanceClass && std::is_member_object_pointer_v<Member>)
    {
        auto gs = adapter::bindInstancePropertyRef<Class>(std::forward<Member>(member), &def);
        instanceProperty_.emplace_back(std::move(name), std::move(gs.first), std::move(gs.second));
        return *this;
    }

    /**
     * 设置继承关系 / Set base class
     * @note 基类必须为一个实例类
     */
    auto& extends(meta::ClassDefine const& parent)
        requires isInstanceClass
    {
        base_ = &parent;
        return *this;
    }

    [[nodiscard]] meta::ClassDefine build() {
        InstanceConstructor ctor = nullptr;
        if constexpr (isInstanceClass) {
            static_assert(State != ConstructorState::None, "No constructor provided");
            if constexpr (State == ConstructorState::Custom || State == ConstructorState::Disabled) {
                ctor = std::move(userDefinedConstructor_);
            } else {
                // Normal
                ctor = [fn = std::move(constructors_)](Arguments const& arguments) -> void* {
                    auto argc = arguments.length();
                    auto iter = fn.find(argc);
                    if (iter == fn.end()) {
                        return nullptr;
                    }
                    for (auto const& f : iter->second) {
                        try {
                            if (void* ptr = std::invoke(f, arguments)) {
                                return ptr;
                            }
                        } catch (...) {}
                    }
                    return nullptr;
                };
            }
        }

        // factory：对于不可构造类（或明确禁用构造），不生成factory（避免 pImpl 单例问题）
        meta::ClassDefine::ManagedResourceFactory factory = nullptr;
        if constexpr (isInstanceClass) {
            if constexpr (State != ConstructorState::Disabled) {
                factory = [](void* instance) -> std::unique_ptr<JsManagedResource> {
                    return JsManagedResource::make(
                        instance,
                        [](void* res) -> void* { return res; },
                        [](void* res) -> void { delete static_cast<Class*>(res); }
                    );
                };
            } // else: script cannot construct instances; do not provide factory (C++ owns lifetime)
        }

        // generate script helper function
        meta::InstanceMemberDefine::InstanceEqualsCallback equals = nullptr;
        if constexpr (isInstanceClass) {
            equals = adapter::bindInstanceEquals<Class>();
        }

        constexpr size_t size = traits::size_of_v<Class>;
        // TODO: fix RTTI typeid
        // constexpr auto   typeId = typeid(Class);

        return meta::ClassDefine{
            std::move(className_),
            meta::StaticMemberDefine{std::move(staticProperty_), std::move(staticFunctions_)},
            meta::InstanceMemberDefine{
                                     std::move(ctor),
                                     std::move(instanceProperty_),
                                     std::move(instanceFunctions_),
                                     size, equals
            },
            base_,
            // std::move(typeId),
            factory
        };
    }
};

template <typename C>
inline auto defineClass(std::string className) {
    return ClassDefineBuilder<C>(std::move(className));
}

} // namespace v8wrap::bind
