#pragma once
#include <exception>
#include <v8-exception.h>

namespace v8wrap {


class JsException : public std::exception {


public:
    static void rethrow(v8::TryCatch const& tryCatch) {}
};


} // namespace v8wrap