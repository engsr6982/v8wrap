#pragma once
#include "Global.hpp"
#include "Types.hpp"
#include "v8-local-handle.h"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsValue.hpp"
#include "v8wrap/internal/V8TypeAlias.hpp"
#include <filesystem>
#include <memory>
#include <v8-context.h>
#include <v8-isolate.h>


namespace v8wrap {

namespace internal {
class V8EscapeScope;
}


class JsRuntime {
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

    virtual ~JsRuntime();

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

    v8::Isolate*            mIsolate{nullptr};
    v8::Global<v8::Context> mContext{};
    std::shared_ptr<void>   mUserData{nullptr};
    JsPlatform*             mPlatform{nullptr};

    bool mDestroying{false};
    bool mIsExternalIsolate{false};
};


} // namespace v8wrap