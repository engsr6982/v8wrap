#include "v8wrap/JsException.hpp"

namespace v8wrap {


JsException::JsException(v8::TryCatch const& tryCatch) 
: mType(Type::Unknown),
mMessage(tryCatch.Message()),
mStackTrace(tryCatch.StackTrace())

{}

JsException::JsException(Type type, std::string const& message) {}

JsException::JsException(std::string message, Type) {}

char const* JsException::what() const noexcept {}

JsException::Type JsException::type() const noexcept {}

std::string JsException::message() const noexcept {}

std::string JsException::stackTrace() const noexcept {}


void rethrow(v8::TryCatch const& tryCatch) {
    if (tryCatch.HasCaught()) {
        throw JsException(tryCatch);
    }
}


} // namespace v8wrap
