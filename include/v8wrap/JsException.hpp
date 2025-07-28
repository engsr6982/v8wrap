#pragma once
#include "Global.hpp"
#include "v8-value.h"
#include <exception>
#include <memory>
#include <string>


V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-exception.h>
#include <v8-persistent-handle.h>
V8_WRAP_WARNING_GUARD_END


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

    explicit JsException(v8::TryCatch const& tryCatch);
    explicit JsException(std::string message, Type type = Type::Error);

    // The C++ standard requires exception classes to be reproducible
    JsException(JsException const&)                = default;
    JsException& operator=(JsException const&)     = default;
    JsException(JsException&&) noexcept            = default;
    JsException& operator=(JsException&&) noexcept = default;

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

    /**
     * v8::Global does not allow copying of resources,
     * but the C++ standard requires exception classes to be replicated
     */
    struct ExceptionContext {
        Type                          type{Type::Unknown};
        mutable std::string           message{};
        mutable v8::Global<v8::Value> exception{};
    };

    std::shared_ptr<ExceptionContext> mExceptionCtx{nullptr};
};


} // namespace v8wrap