#include "v8wrap/JsException.hpp"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsValue.hpp"
#include <algorithm>
#include <exception>

V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-exception.h>
#include <v8-local-handle.h>
#include <v8-persistent-handle.h>
#include <v8-primitive.h>
#include <v8-value.h>
#include <v8.h>
V8_WRAP_WARNING_GUARD_END

namespace v8wrap {


JsException::JsException(v8::TryCatch const& tryCatch) : std::exception() {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    mException   = v8::Global<v8::Value>(isolate, tryCatch.Exception());
}

JsException::JsException(std::string message, Type type) : std::exception(), mType(type), mMessage(std::move(message)) {
    makeException(); // null exception, make it
}

JsException::JsException(JsException&& other) noexcept
: mType(other.mType),
  mMessage(std::move(other.mMessage)),
  mException(std::move(other.mException)) {}

JsException& JsException::operator=(JsException&& other) noexcept {
    if (&other != this) {
        mType      = other.mType;
        mMessage   = std::move(other.mMessage);
        mException = std::move(other.mException);
    }
    return *this;
}

JsException::Type JsException::type() const noexcept { return mType; }

char const* JsException::what() const noexcept {
    extractMessage();
    return mMessage.c_str();
}

std::string JsException::message() const noexcept {
    extractMessage();
    return mMessage;
}

std::string JsException::stacktrace() const noexcept {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();

    auto vtry = v8::TryCatch{isolate}; // noexcept

    auto stack = v8::TryCatch::StackTrace(ctx, mException.Get(isolate));
    if (!stack.IsEmpty()) {
        v8::String::Utf8Value ut8{isolate, stack.ToLocalChecked()};
        if (auto str = *ut8) {
            return str;
        }
    }
    return "[ERROR: Could not get stacktrace]";
}

void JsException::rethrowToRuntime() const {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    isolate->ThrowException(mException.Get(isolate));
}

void JsException::extractMessage() const noexcept {
    if (!mMessage.empty()) {
        return;
    }
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    auto vtry    = v8::TryCatch{isolate};

    auto msg = v8::Exception::CreateMessage(isolate, mException.Get(isolate));
    if (!msg.IsEmpty()) {
        Local<JsString> jsStr{msg->Get()};
        mMessage = jsStr.toString().getValue();
        return;
    }
    mMessage = "[ERROR: Could not get exception message]";
}

void JsException::makeException() const {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();

    v8::Local<v8::Value> exception;
    {
        switch (mType) {
        case Type::Unknown:
        case Type::Error:
            exception = v8::Exception::Error(v8::String::NewFromUtf8(isolate, mMessage.c_str()).ToLocalChecked());
            break;
        case Type::RangeError:
            exception = v8::Exception::RangeError(v8::String::NewFromUtf8(isolate, mMessage.c_str()).ToLocalChecked());
            break;
        case Type::ReferenceError:
            exception =
                v8::Exception::ReferenceError(v8::String::NewFromUtf8(isolate, mMessage.c_str()).ToLocalChecked());
            break;
        case Type::SyntaxError:
            exception = v8::Exception::SyntaxError(v8::String::NewFromUtf8(isolate, mMessage.c_str()).ToLocalChecked());
            break;
        case Type::TypeError:
            exception = v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, mMessage.c_str()).ToLocalChecked());
            break;
        }
    }
    mException = v8::Global<v8::Value>(isolate, exception);
}

void JsException::rethrow(v8::TryCatch const& tryCatch) {
    if (tryCatch.HasCaught()) {
        throw JsException(tryCatch);
    }
}


} // namespace v8wrap
