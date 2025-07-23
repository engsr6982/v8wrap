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

    V8WRAP_DISALLOW_COPY_AND_MOVE(JsException)

    explicit JsException(v8::TryCatch const& tryCatch);
    explicit JsException(std::string message, Type type = Type::Error);

    [[nodiscard]] Type type() const noexcept;

    [[nodiscard]] char const* what() const noexcept override;

    [[nodiscard]] std::string message() const noexcept;

    [[nodiscard]] std::string stacktrace() const noexcept;

    /**
     * @brief 将异常重新抛出到 JavaScript 侧
     */
    void rethrowToRuntime() const;

public:
    /**
     * @brief 将 v8::TryCatch 中的异常重新抛出为 JsException
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