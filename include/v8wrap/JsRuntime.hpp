#pragma once
#include "Global.hpp"
#include "v8-local-handle.h"
#include "v8-persistent-handle.h"
#include "v8-value.h"
#include "v8wrap/JsValue.hpp"
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

    explicit JsRuntime(v8::Isolate* isolate);
    explicit JsRuntime(v8::Isolate* isolate, v8::Local<v8::Context> context);
    virtual ~JsRuntime();

    [[nodiscard]] v8::Isolate* isolate() const;

    [[nodiscard]] v8::Local<v8::Context> context() const;

    void setData(std::shared_ptr<void> data);

    template <typename T>
    [[nodiscard]] inline std::shared_ptr<T> getData() const {
        return std::static_pointer_cast<T>(mUserData);
    }

    // eval(Local<JsString> code);
    // eval(Local<JsString> code, Local<JsString> source);
    // loadFile(std::filesystem::path path);

    // get(Local<JsString> key);

    // set(Local<JsString> key, Local<JsValue> value, bool readOnly = false);

private:
    void initalizeContext();

    v8::Local<v8::Value> unwrap(Local<JsValue> const& value);
    Local<JsValue>       wrap(v8::Local<v8::Value> const& value);

    friend class JsRuntimeScope;
    friend class ExitJsRuntimeScope;
    friend class internal::V8EscapeScope;

    v8::Isolate*            mIsolate{nullptr};
    v8::Global<v8::Context> mContext{};
    std::shared_ptr<void>   mUserData{nullptr};
};


} // namespace v8wrap