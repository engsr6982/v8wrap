#pragma once
#include "Global.hpp"
#include "Types.hpp"
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

    // Local<JsValue> newNativeClass(ClassBinding const& binding, T* instance);

private:
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

    v8::Isolate*            mIsolate{nullptr};
    v8::Global<v8::Context> mContext{};
    std::shared_ptr<void>   mUserData{nullptr};
    JsPlatform*             mPlatform{nullptr}; // nullptr if this runtime is created from outside isolate and context

    bool mDestroying{false};
    bool mIsExternalIsolate{false};

    std::unordered_map<ManagedResource*, v8::Global<v8::Value>> mManagedResources;
    std::unordered_map<std::string, ClassBinding const*>        mRegisteredBindings;
};


} // namespace v8wrap

#include "v8wrap/JsRuntime.inl" // include implementation
