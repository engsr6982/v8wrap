#pragma once
#include "Global.hpp"
#include <exception>
#include <string>
#include <v8-exception.h>
#include <v8-persistent-handle.h>


namespace v8wrap {


class JsException final : public std::exception {
public:
    enum class Type {
        Unknown = -1, // JavaScript 侧抛出的异常为 Unknown
        Error,
        RangeError,
        ReferenceError,
        SyntaxError,
        TypeError
    };

    V8WRAP_DISALLOW_COPY(JsException)

    explicit JsException(v8::TryCatch const& tryCatch);
    explicit JsException(std::string message, Type type = Type::Error);

    JsException(JsException&&) noexcept;
    JsException& operator=(JsException&&) noexcept;

    [[nodiscard]] Type type() const noexcept;

    [[nodiscard]] char const* what() const noexcept override;

    [[nodiscard]] std::string message() const noexcept;

    [[nodiscard]] std::string stacktrace() const noexcept;

    /**
     * Throw this exception to v8 (JavaScript).
     * Normally we don't need to call this method, the package library handles exceptions internally.
     * You just need to re-throw 'throw e;`
     * Or throw JsException inside the JsFunction callback.
     */
    void rethrowToRuntime() const;

public:
    /**
     * Re-throw the exception in v8::TryCatch as a JsException
     */
    static void rethrow(v8::TryCatch const& tryCatch);

private:
    void extractMessage() const noexcept;
    void makeException() const;

    Type                          mType{Type::Unknown};
    mutable std::string           mMessage{};
    mutable v8::Global<v8::Value> mException{};
};


} // namespace v8wrap