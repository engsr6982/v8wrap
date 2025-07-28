#pragma once
#include "v8wrap/JsReference.hpp"
#include "v8wrap/Types.hpp"
#include <vector>

V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-local-handle.h>
#include <v8-value.h>
V8_WRAP_WARNING_GUARD_END

namespace v8wrap::internal {


v8::Local<v8::Value>* extractArgs(std::vector<Local<JsValue>> const& args);


} // namespace v8wrap::internal