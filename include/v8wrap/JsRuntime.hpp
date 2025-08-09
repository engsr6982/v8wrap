#pragma once
#include "Global.hpp"
#include "Types.hpp"
#include "v8wrap/Bindings.hpp"
#include "v8wrap/Concepts.hpp"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsValue.hpp"
#include "v8wrap/internal/V8TypeAlias.hpp"
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


class JsRuntime {
    ~JsRuntime();

public:
    V8WRAP_DISALLOW_COPY_AND_MOVE(JsRuntime);

    /**
     * Create a Js runtime from the JsPlatform.
     * Make sure that the Js platform is initialized before calling.
     */
    explicit JsRuntime();

    /**
     * To create a Js runtime, using sources from outside is isolate and context.
     * This overload is commonly used in NodeJs Addons.
     * When using isolate and contexts from outside (e.g. NodeJs), the JsPlatform is not required.
     */
    explicit JsRuntime(v8::Isolate* isolate, v8::Local<v8::Context> context);

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
     * If this runtime is the default construct, it removes itself from JsPlatform internally
     */
    void destroy();

    [[nodiscard]] bool isDestroying() const;

    Local<JsValue> eval(Local<JsString> const& code);

    Local<JsValue> eval(Local<JsString> const& code, Local<JsString> const& source);

    template <typename T>
        requires StringLike<T>
    Local<JsValue> eval(T const& str);

    void loadFile(std::filesystem::path const& path);

    [[nodiscard]] Local<JsObject> getGlobalThis() const;

    [[nodiscard]] Local<JsValue> getVauleFromGlobalThis(Local<JsString> const& key) const;

    void setVauleToGlobalThis(Local<JsString> const& key, Local<JsValue> const& value) const;

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
    void registerBindingClass(ClassBinding const& binding);


    /**
     * 创建一个新的 JavaScript 类实例
     * Creates a new JavaScript class instance.
     */
    Local<JsObject> newInstance(ClassBinding const& bind, std::unique_ptr<WrappedResource>&& wrappedResource);

    /**
     * 创建一个新的 JavaScript 类实例
     * @warning C++实例必须分配在堆上(使用 `new` 操作符), 栈分配的实例会导致悬垂引用和 GC 崩溃。
     * @note v8wrap 会接管实例的生命周期，GC 时自动销毁
     */
    template <typename T>
    Local<JsObject> newInstanceOfRaw(ClassBinding const& bind, T* instance);

    /**
     * 创建一个新的 JavaScript 类实例
     * @note v8wrap 不接管实例的生命周期，由外部管理实例的生命周期 (不自动销毁)
     */
    template <typename T>
    Local<JsObject> newInstanceOfView(ClassBinding const& bind, T* instance);

    /**
     * 创建一个新的 JavaScript 类实例
     * @note v8wrap 不接管实例的生命周期，对子资源创建 Global 引用关联生命周期(常见于对类成员创建Js实例，防止主实例 GC)
     */
    template <typename T>
    Local<JsObject> newInstanceOfView(ClassBinding const& bind, T* instance, Local<JsObject> const& ownerJs);

    /**
     * 创建一个新的 JavaScript 类实例
     * @note v8wrap 接管实例的生命周期，GC 时自动销毁
     */
    template <typename T>
    Local<JsObject> newInstanceOfUnique(ClassBinding const& bind, std::unique_ptr<T>&& instance);

    /**
     * 创建一个新的 JavaScript 类实例
     * @note v8wrap 共享对此实例的引用，仅在 Gc 时重置引用
     */
    template <typename T>
    Local<JsObject> newInstanceOfShared(ClassBinding const& bind, std::shared_ptr<T>&& instance);

    /**
     * 创建一个新的 JavaScript 类实例
     * @note v8wrap 仅在运行时尝试获取资源，如果获取不到资源则返回 null
     */
    template <typename T>
    Local<JsObject> newInstanceOfWeak(ClassBinding const& bind, std::weak_ptr<T>&& instance);

    [[nodiscard]] bool isInstanceOf(Local<JsObject> const& obj, ClassBinding const& binding) const;

    [[nodiscard]] void* getNativeInstanceOf(Local<JsObject> const& obj) const;

    template <typename T>
    [[nodiscard]] inline T* getNativeInstanceOf(Local<JsObject> const& obj) const;

    void gc() const;

private:
    void implStaticRegister(v8::Local<v8::FunctionTemplate>& ctor, StaticBinding const& staticBinding);
    v8::Local<v8::FunctionTemplate> createInstanceClassCtor(ClassBinding const& binding);
    void implInstanceRegister(v8::Local<v8::FunctionTemplate>& ctor, InstanceBinding const& instanceBinding);

    friend class JsRuntimeScope;
    friend class ExitJsRuntimeScope;
    friend class internal::V8EscapeScope;

    template <typename>
    friend class internal::V8GlobalRef;

    struct ManagedResource {
        JsRuntime*                 runtime;
        void*                      resource;
        std::function<void(void*)> deleter;
    };

    // v8: AlignedPointerInInternalField
    static constexpr int kInternalFieldCount            = 1;
    static constexpr int kInternalField_WrappedResource = 0;

    v8::Isolate*            mIsolate{nullptr};
    v8::Global<v8::Context> mContext{};
    std::shared_ptr<void>   mUserData{nullptr};
    JsPlatform*             mPlatform{nullptr}; // nullptr if this runtime is created from outside isolate and context

    bool mDestroying{false};
    bool mIsExternalIsolate{false};

    // This symbol is used to mark the construction of objects from C++ (with special logic).
    v8::Global<v8::Symbol> mConstructorSymbol{};

    std::unordered_map<ManagedResource*, v8::Global<v8::Value>>               mManagedResources;
    std::unordered_map<std::string, ClassBinding const*>                      mRegisteredBindings;
    std::unordered_map<ClassBinding const*, v8::Global<v8::FunctionTemplate>> mJsClassConstructor;
};


} // namespace v8wrap

#include "v8wrap/JsRuntime.inl" // include implementation
