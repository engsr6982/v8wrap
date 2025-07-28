#pragma once
#include "V8TypeAlias.hpp"
#include "v8wrap/Global.hpp"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsRuntime.hpp"
#include "v8wrap/Types.hpp"


namespace v8wrap::internal {


template <typename T>
class V8GlobalRef final {
    using V8GlobalType = v8::Global<internal::V8Type_v<T>>;

    JsRuntime*   mRuntime{nullptr};
    V8GlobalType mHandle{};

public:
    V8WRAP_DISALLOW_COPY(V8GlobalRef);

    V8GlobalRef() = default;
    explicit V8GlobalRef(JsRuntime* rt, Local<T> local)
    : mRuntime(rt),
      mHandle(rt->mIsolate, v8wrap::JsRuntime::unwrap(local)) {}

    V8GlobalRef(V8GlobalRef&& other) noexcept {
        mRuntime       = other.mRuntime;
        mHandle        = std::move(other.mHandle);
        other.mRuntime = nullptr;
    }
    V8GlobalRef& operator=(V8GlobalRef&& other) noexcept {
        if (this != &other) {
            mRuntime       = other.mRuntime;
            mHandle        = std::move(other.mHandle);
            other.mRuntime = nullptr;
        }
        return *this;
    }
    ~V8GlobalRef() { reset(); }

    void makeWeak() {
        if (!mHandle.IsEmpty()) {
            mHandle.SetWeak();
        }
    }

    Local<T> get() const { return Local<T>{mHandle.Get(mRuntime->mIsolate)}; }

    Local<JsValue> getValue() const { return Local<JsValue>{mHandle.Get(mRuntime->mIsolate).template As<v8::Value>()}; }

    bool isEmpty() const { return mHandle.IsEmpty(); }

    void reset() {
        mHandle.Reset();
        mRuntime = nullptr;
    }
};


} // namespace v8wrap::internal