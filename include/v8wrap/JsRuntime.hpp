#pragma once
#include "Global.hpp"
#include "Types.hpp"
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

    /**
     * 销毁JsRuntime，释放所有资源
     */
    void destroy();

    [[nodiscard]] bool isDestroying() const;

    void eval(Local<JsString> code);

    void eval(Local<JsString> code, Local<JsString> source);

    void loadFile(std::filesystem::path path);

    Local<JsValue> get(Local<JsString> key);

    void set(Local<JsString> key, Local<JsValue> value, bool readOnly = false);

    static v8::Local<v8::Value> unwrap(Local<JsValue> const& value);
    static Local<JsValue>       wrap(v8::Local<v8::Value> const& value);

private:
    void initalizeContext();


    friend class JsRuntimeScope;
    friend class ExitJsRuntimeScope;
    friend class internal::V8EscapeScope;

    template <typename>
    friend class internal::V8GlobalRef;

    v8::Isolate*            mIsolate{nullptr};
    v8::Global<v8::Context> mContext{};
    std::shared_ptr<void>   mUserData{nullptr};

    bool mDestroying{false};
};


} // namespace v8wrap