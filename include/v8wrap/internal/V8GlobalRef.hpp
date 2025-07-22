#pragma once
#include "V8TypeAlias.hpp"
#include "v8wrap/Global.hpp"
#include "v8wrap/JsRuntime.hpp"
#include "v8wrap/Types.hpp"


namespace v8wrap::internal {


template <typename T>
class V8GlobalRef final {
    using V8GlobalType = v8::Global<internal::V8Type<T>>;

    JsRuntime*   mRuntime{nullptr};
    V8GlobalType mHandle{};

public:
    V8WRAP_DISALLOW_COPY(V8GlobalRef);

    V8GlobalRef() = default;
    explicit V8GlobalRef(JsRuntime* rt, Local<T> local) : mRuntime(rt), mHandle(rt->mIsolate, rt->unwrap(local)) {}

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

    ~V8GlobalRef() = default;

    void makeWeak() {
        if (!mHandle.IsEmpty()) {
            mHandle.SetWeak();
        }
    }
};


} // namespace v8wrap::internal