#pragma once
#include "v8wrap/JsReference.hpp"
#include "v8wrap/Types.hpp"
#include <v8-local-handle.h>
#include <v8-value.h>
#include <vector>


namespace v8wrap::internal {


v8::Local<v8::Value>* extractArgs(std::vector<Local<JsValue>> const& args);


} // namespace v8wrap::internal