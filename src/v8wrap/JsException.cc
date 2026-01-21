#include "v8wrap/JsException.h"
#include "v8wrap/JsRuntimeScope.h"
#include "v8wrap/reference/Reference.h"
#include "v8wrap/types/Value.h"
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


JsException::JsException(v8::TryCatch const& tryCatch)
: std::exception(),
  mExceptionCtx(std::make_shared<ExceptionContext>()) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();

    mExceptionCtx->exception = v8::Global<v8::Value>(isolate, tryCatch.Exception());
}

JsException::JsException(std::string message, Type type)
: std::exception(),
  mExceptionCtx(std::make_shared<ExceptionContext>()) {
    mExceptionCtx->type    = type;
    mExceptionCtx->message = std::move(message);
    makeException(); // null exception, make it
}


JsException::Type JsException::type() const noexcept { return mExceptionCtx->type; }

char const* JsException::what() const noexcept {
    extractMessage();
    return mExceptionCtx->message.c_str();
}

std::string JsException::message() const noexcept {
    extractMessage();
    return mExceptionCtx->message;
}

std::string JsException::stacktrace() const noexcept {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();

    auto vtry = v8::TryCatch{isolate}; // noexcept

    auto stack = v8::TryCatch::StackTrace(ctx, mExceptionCtx->exception.Get(isolate));
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
    isolate->ThrowException(mExceptionCtx->exception.Get(isolate));
}

void JsException::extractMessage() const noexcept {
    if (!mExceptionCtx->message.empty()) {
        return;
    }
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    auto vtry    = v8::TryCatch{isolate};

    auto msg = v8::Exception::CreateMessage(isolate, mExceptionCtx->exception.Get(isolate));
    if (!msg.IsEmpty()) {
        Local<JsString> jsStr{msg->Get()};
        mExceptionCtx->message = jsStr.toString().getValue();
        return;
    }
    mExceptionCtx->message = "[ERROR: Could not get exception message]";
}

void JsException::makeException() const {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();

    v8::Local<v8::Value> exception;
    {
        switch (mExceptionCtx->type) {
        case Type::Unknown:
        case Type::Error:
            exception =
                v8::Exception::Error(v8::String::NewFromUtf8(isolate, mExceptionCtx->message.c_str()).ToLocalChecked());
            break;
        case Type::RangeError:
            exception = v8::Exception::RangeError(
                v8::String::NewFromUtf8(isolate, mExceptionCtx->message.c_str()).ToLocalChecked()
            );
            break;
        case Type::ReferenceError:
            exception = v8::Exception::ReferenceError(
                v8::String::NewFromUtf8(isolate, mExceptionCtx->message.c_str()).ToLocalChecked()
            );
            break;
        case Type::SyntaxError:
            exception = v8::Exception::SyntaxError(
                v8::String::NewFromUtf8(isolate, mExceptionCtx->message.c_str()).ToLocalChecked()
            );
            break;
        case Type::TypeError:
            exception = v8::Exception::TypeError(
                v8::String::NewFromUtf8(isolate, mExceptionCtx->message.c_str()).ToLocalChecked()
            );
            break;
        }
    }
    mExceptionCtx->exception = v8::Global<v8::Value>(isolate, exception);
}

void JsException::rethrow(v8::TryCatch const& tryCatch) {
    if (tryCatch.HasCaught()) {
        throw JsException(tryCatch);
    }
}


} // namespace v8wrap
