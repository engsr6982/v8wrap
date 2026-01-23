#pragma once
#include "v8wrap/Global.h"
#include "v8wrap/Types.h"
#include "v8wrap/bind/JsManagedResource.h"
#include "v8wrap/bind/meta/MemberDefine.h"
#include "v8wrap/concepts/BasicConcepts.h"
#include "v8wrap/reference/Local.h"
#include "v8wrap/types/Value.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>


V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-context.h>
#include <v8-isolate.h>
#include <v8-local-handle.h>
#include <v8-persistent-handle.h>
#include <v8-value.h>
V8_WRAP_WARNING_GUARD_END


namespace v8wrap {

namespace internal {
class V8EscapeScope;
}


class Engine {
    ~Engine();

public:
    V8WRAP_DISALLOW_COPY_AND_MOVE(Engine);

    /**
     * Create a Js runtime from the Platform.
     * Make sure that the Js platform is initialized before calling.
     */
    explicit Engine();

    /**
     * To create a Js runtime, using sources from outside is isolate and context.
     * This overload is commonly used in NodeJs Addons.
     * When using isolate and contexts from outside (e.g. NodeJs), the Platform is not required.
     */
    explicit Engine(v8::Isolate* isolate, v8::Local<v8::Context> context);

    [[nodiscard]] v8::Isolate* isolate() const;

    [[nodiscard]] v8::Local<v8::Context> context() const;

    void setData(std::shared_ptr<void> data);

    template <typename T>
    [[nodiscard]] inline std::shared_ptr<T> getData() const;

    /**
     * Stop and destroy the current Js runtime.
     * The call frees up all resources and destroys the isolate and context.
     * After you destroy this runtime, you can no longer access any methods for this runtime because delete this is done
     * internally.
     *
     * If this runtime is the default construct, it removes itself from Platform internally
     */
    void destroy();

    [[nodiscard]] bool isDestroying() const;

    Local<Value> eval(Local<String> const& code);

    Local<Value> eval(Local<String> const& code, Local<String> const& source);

    template <typename T>
        requires concepts::StringLike<T>
    Local<Value> eval(T const& str);

    void loadFile(std::filesystem::path const& path);

    [[nodiscard]] Local<Object> getGlobalThis() const;

    [[nodiscard]] Local<Value> getVauleFromGlobalThis(Local<String> const& key) const;

    void setVauleToGlobalThis(Local<String> const& key, Local<Value> const& value) const;

    /**
     * Add a managed resource to the runtime.
     * The managed resource will be destroyed when the runtime is destroyed.
     * @param resource Resources that need to be managed
     * @param value The v8 object associated with this resource.
     * @param deleter The deleter function to be called when the resource is destroyed.
     */
    void addManagedResource(void* resource, v8::Local<v8::Value> value, std::function<void(void*)>&& deleter);

    /**
     * Register a binding class and mount it to globalThis
     */
    void registerClass(bind::meta::ClassDefine const& binding);

    // TODO: add more register functions
    /**
     * 注册一个枚举
     * @param def 枚举定义
     * @note 行为如下:
     *       C++ -> Js  对 enum 进行 static_cast 到 int32(number) 传给 Js (TypeConverter)
     *       Js  -> C++ 因为 Js 没有枚举类型，手动传递 num 值麻烦，所以使用本API，创建一个静态的 Object
     *        对象，将枚举值映射到对象属性上，方便 Js 获取枚举值
     * @note 每个 enum object 会设置一个 $name 属性，值为 enum 的名字
     */
    // bool registerEnum(bind::meta::EnumDefine const& def);


    /**
     * 创建一个新的 JavaScript 类实例
     * Creates a new JavaScript class instance.
     */
    Local<Object>
    newInstance(bind::meta::ClassDefine const& bind, std::unique_ptr<bind::JsManagedResource>&& wrappedResource);

    /**
     * 创建一个新的 JavaScript 类实例
     * @warning C++实例必须分配在堆上(使用 `new` 操作符), 栈分配的实例会导致悬垂引用和 GC 崩溃。
     * @note v8wrap 会接管实例的生命周期，GC 时自动销毁
     */
    template <typename T>
    Local<Object> newInstanceOfRaw(bind::meta::ClassDefine const& bind, T* instance);

    /**
     * 创建一个新的 JavaScript 类实例
     * @note v8wrap 不接管实例的生命周期，由外部管理实例的生命周期 (不自动销毁)
     */
    template <typename T>
    Local<Object> newInstanceOfView(bind::meta::ClassDefine const& bind, T* instance);

    /**
     * 创建一个新的 JavaScript 类实例
     * @note v8wrap 不接管实例的生命周期，对子资源创建 Global 引用关联生命周期(常见于对类成员创建Js实例，防止主实例 GC)
     */
    template <typename T>
    Local<Object> newInstanceOfView(bind::meta::ClassDefine const& bind, T* instance, Local<Object> const& ownerJs);

    /**
     * 创建一个新的 JavaScript 类实例
     * @note v8wrap 接管实例的生命周期，GC 时自动销毁
     */
    template <typename T>
    Local<Object> newInstanceOfUnique(bind::meta::ClassDefine const& bind, std::unique_ptr<T>&& instance);

    /**
     * 创建一个新的 JavaScript 类实例
     * @note v8wrap 共享对此实例的引用，仅在 Gc 时重置引用
     */
    template <typename T>
    Local<Object> newInstanceOfShared(bind::meta::ClassDefine const& bind, std::shared_ptr<T>&& instance);

    /**
     * 创建一个新的 JavaScript 类实例
     * @note v8wrap 仅在运行时尝试获取资源，如果获取不到资源则返回 null
     */
    template <typename T>
    Local<Object> newInstanceOfWeak(bind::meta::ClassDefine const& bind, std::weak_ptr<T>&& instance);

    [[nodiscard]] bool isInstanceOf(Local<Object> const& obj, bind::meta::ClassDefine const& binding) const;

    [[nodiscard]] void* getNativeInstanceOf(Local<Object> const& obj) const;

    template <typename T>
    [[nodiscard]] inline T* getNativeInstanceOf(Local<Object> const& obj) const;

    void gc() const;

private:
    // TODO: implement
    // void setObjectToStringTag(Local<Object>& obj, std::string_view tag) const;

    void implStaticRegister(v8::Local<v8::FunctionTemplate>& ctor, bind::meta::StaticMemberDefine const& staticBinding);

    v8::Local<v8::FunctionTemplate> createInstanceClassCtor(bind::meta::ClassDefine const& binding);

    void implInstanceRegister(
        v8::Local<v8::FunctionTemplate>&        ctor,
        bind::meta::InstanceMemberDefine const& instanceBinding
    );

    friend class EngineScope;
    friend class ExitEngineScope;
    friend class internal::V8EscapeScope;

    template <typename>
    friend class Global;
    template <typename>
    friend class Weak;

    struct ManagedResource {
        Engine*                    runtime;
        void*                      resource;
        std::function<void(void*)> deleter;
    };

    // v8: AlignedPointerInInternalField
    static constexpr int kInternalFieldCount            = 1;
    static constexpr int kInternalField_WrappedResource = 0;

    v8::Isolate*            mIsolate{nullptr};
    v8::Global<v8::Context> mContext{};
    std::shared_ptr<void>   mUserData{nullptr};
    Platform*               mPlatform{nullptr}; // nullptr if this runtime is created from outside isolate and context

    bool mDestroying{false};
    bool mIsExternalIsolate{false};

    // This symbol is used to mark the construction of objects from C++ (with special logic).
    v8::Global<v8::Symbol> mConstructorSymbol{};

    std::unordered_map<ManagedResource*, v8::Global<v8::Value>>                          mManagedResources;
    std::unordered_map<std::string, bind::meta::ClassDefine const*>                      mRegisteredBindings;
    std::unordered_map<bind::meta::ClassDefine const*, v8::Global<v8::FunctionTemplate>> mJsClassConstructor;
};


} // namespace v8wrap

#include "Engine.inl" // include implementation
