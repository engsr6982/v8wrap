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
     * 为C++实例创建JavaScript包装器。
     * Creates a JavaScript wrapper for a C++ instance.
     *
     * @param binding 类绑定的元数据 / Class binding metadata.
     * @param instance 要包装的C++实例指针 / Pointer to the C++ instance to be wrapped.
     * @return 新创建的JavaScript对象的本地句柄 / Local handle to the newly created JavaScript object.
     *
     * @warning C++实例必须分配在堆上（使用 `new` 操作符）。
     *          栈分配的实例会导致悬垂引用和 GC 崩溃。
     *          The C++ instance **must** be allocated on the heap (using `new`).
     *          Stack-allocated instances will cause dangling references and GC crashes.
     *
     * @note 此重载创建的实例会使用绑定时提供的 `Holder` 存储资源。
     *       是否在 GC 时释放实例由 `Holder` 控制。
     *       The instance created by this overload will use the `Holder` provided by the binding to manage resources.
     *       Whether the instance is released during GC is determined by the `Holder`.
     */
    Local<JsObject> newInstanceOf(ClassBinding const& binding, void* instance);

    /**
     * 通过类名为C++实例创建JavaScript包装器。
     * Creates a JavaScript wrapper for a C++ instance using the class name.
     *
     * @param className 要实例化的类名 / Name of the class to instantiate.
     * @param instance 要包装的C++实例指针 / Pointer to the C++ instance to be wrapped.
     * @return 新创建的JavaScript对象的本地句柄 / Local handle to the newly created JavaScript object.
     *
     * @warning 内存要求与 `newInstanceOf(ClassBinding const&, void*)` 相同。
     *          The memory requirements are the same as `newInstanceOf(ClassBinding const&, void*)`.
     * @see newInstanceOf(ClassBinding const&, void*)
     */
    Local<JsObject> newInstanceOf(std::string const& className, void* instance);

    /**
     * 为C++实例创建JavaScript视图，该视图会保持对其所有者JavaScript对象的强引用。
     * Creates a JavaScript view of a C++ instance that maintains a strong reference to its owner JavaScript object.
     *
     * 通常用于需要与父/所有者对象保持关联的成员对象，以防止过早的垃圾回收。
     * Typically used for member objects that must maintain association with their parent/owner object to prevent
     * premature GC.
     *
     * @param binding 类绑定的元数据 / Class binding metadata.
     * @param instance 要包装的C++实例指针 / Pointer to the C++ instance to be wrapped.
     * @param ownerJs 持有该实例的所有者JavaScript对象 / Owner JavaScript object that holds this instance.
     * @return 新创建的JavaScript对象的本地句柄 / Local handle to the newly created JavaScript object.
     *
     * @note 内部会创建对 `ownerJs` 的 Global 引用，以防止其被 GC。
     *       Internally, a Global reference to `ownerJs` is created to prevent it from being GC'ed.
     *
     * @note 该视图对象销毁时，Global 引用才会被释放，在此之前会阻止 `ownerJs` 被回收。
     *       The Global reference will be released only when this view object is destroyed,
     *       preventing `ownerJs` from being collected until then.
     *
     * @see 使用示例请参考 `test/BindingTest.cc`（PlayerBind）。
     *      See `test/BindingTest.cc` (PlayerBind) for usage examples.
     */
    Local<JsObject> newInstanceOfView(ClassBinding const& binding, void* instance, Local<JsObject> ownerJs);

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
    static constexpr int kInternalFieldCount    = 1;
    static constexpr int kInternalField_IHolder = 0;

    v8::Isolate*            mIsolate{nullptr};
    v8::Global<v8::Context> mContext{};
    std::shared_ptr<void>   mUserData{nullptr};
    JsPlatform*             mPlatform{nullptr}; // nullptr if this runtime is created from outside isolate and context

    bool mDestroying{false};
    bool mIsExternalIsolate{false};

    // This symbol is used to mark the construction of objects from C++ (with special logic).
    v8::Global<v8::Symbol> mConstructorSymbol{};
    // Special behavior, used to mark that this construction belongs to view construction (holding only).
    // Correspondingly, during garbage collection (gc), the resource will not be destroyed, because its lifecycle is
    //  owned by the class that holds this resource (such as a class member).
    // In this scenario, a Global will be used to force a reference to the parent class instance, so as to avoid
    //  dangling references of the property.
    v8::Global<v8::Symbol> mViewConstructorSymbol{};


    std::unordered_map<ManagedResource*, v8::Global<v8::Value>>               mManagedResources;
    std::unordered_map<std::string, ClassBinding const*>                      mRegisteredBindings;
    std::unordered_map<ClassBinding const*, v8::Global<v8::FunctionTemplate>> mJsClassConstructor;
};


} // namespace v8wrap

#include "v8wrap/JsRuntime.inl" // include implementation
