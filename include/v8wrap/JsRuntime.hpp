#pragma once
#include "Global.hpp"
#include "Types.hpp"
#include "v8-local-handle.h"
#include "v8-persistent-handle.h"
#include "v8-value.h"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsValue.hpp"
#include "v8wrap/internal/V8TypeAlias.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>
#include <v8-context.h>
#include <v8-isolate.h>


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
    [[nodiscard]] inline std::shared_ptr<T> getData() const {
        return std::static_pointer_cast<T>(mUserData);
    }

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

    void loadFile(std::filesystem::path const& path);

    Local<JsValue> get(Local<JsString> key);

    void set(Local<JsString> key, Local<JsValue> value, bool readOnly = false);

    /**
     * Add a managed resource to the runtime.
     * The managed resource will be destroyed when the runtime is destroyed.
     * @param resource Resources that need to be managed
     * @param value The v8 object associated with this resource.
     * @param deleter The deleter function to be called when the resource is destroyed.
     */
    void addManagedResource(void* resource, v8::Local<v8::Value> value, std::function<void(void*)>&& deleter);

    template <typename T>
    [[nodiscard]] inline static v8::Local<internal::V8Type<T>> unwrap(Local<T> const& value) {
        return value.val; // friend
    }

    template <typename T>
    [[nodiscard]] inline static Local<T> wrap(v8::Local<internal::V8Type<T>> const& value) {
        return Local<T>{value};
    }

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
};


} // namespace v8wrap