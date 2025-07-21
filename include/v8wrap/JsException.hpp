#pragma once
#include "Global.hpp"
#include "v8wrap/JsValue.hpp"
#include <stdexcept>
#include <v8-exception.h>


namespace v8wrap {


class JsException : public std::runtime_error {
public:
    enum class Type {
        Unknown = -1, // JavaScript 侧抛出的异常为 Unknown
        Error,
        RangeError,
        ReferenceError,
        SyntaxError,
        TypeError
    };

    V8WRAP_DISALLOW_COPY_AND_MOVE(JsException)

    explicit JsException(v8::TryCatch const& tryCatch);
    explicit JsException(Type type, std::string const& message = {});
    explicit JsException(std::string message, Type = Type::Error);

    [[nodiscard]] char const* what() const noexcept override;

    [[nodiscard]] Type type() const noexcept;

    [[nodiscard]] std::string message() const noexcept;

    [[nodiscard]] std::string stackTrace() const noexcept;

public:
    static void rethrow(v8::TryCatch const& tryCatch);

private:
    Type                  mType;
    v8::Local<v8::String> mMessage;
    v8::Local<v8::String> mStackTrace;
};


} // namespace v8wrap