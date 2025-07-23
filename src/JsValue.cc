#include "v8wrap/JsValue.hpp"
#include "v8wrap/JsException.hpp"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsRuntime.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include <string_view>
#include <v8-exception.h>
#include <v8-local-handle.h>
#include <v8-primitive.h>
#include <v8-template.h>


namespace v8wrap {


Local<JsNull> JsNull::newNull() {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsNull>{v8::Null(isolate)};
}


Local<JsUndefined> JsUndefined::newUndefined() {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsUndefined>{v8::Undefined(isolate)};
}


Local<JsBoolean> JsBoolean::newBoolean(bool b) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsBoolean>{v8::Boolean::New(isolate, b)};
}


Local<JsNumber> JsNumber::newNumber(double d) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsNumber>{v8::Number::New(isolate, d)};
}
Local<JsNumber> JsNumber::newNumber(int i) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsNumber>{v8::Number::New(isolate, i)};
}
Local<JsNumber> JsNumber::newNumber(float f) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsNumber>{v8::Number::New(isolate, f)};
}


Local<JsBigInt> JsBigInt::newBigInt(int64_t i) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsBigInt>{v8::BigInt::New(isolate, i)};
}


Local<JsString> JsString::newString(const char* str) { return newString(std::string_view{str}); }
Local<JsString> JsString::newString(std::string const& str) { return newString(str.c_str()); }
Local<JsString> JsString::newString(std::string_view str) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();

    v8::TryCatch vtry{isolate};

    auto v8Str =
        v8::String::NewFromUtf8(isolate, str.data(), v8::NewStringType::kNormal, static_cast<int>(str.length()));
    JsException::rethrow(vtry);
    return Local<JsString>{v8Str.ToLocalChecked()};
}

Local<JsSymbol> JsSymbol::newSymbol() {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsSymbol>{v8::Symbol::New(isolate)};
}
Local<JsSymbol> JsSymbol::newSymbol(std::string_view description) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    auto v8Sym   = v8::Symbol::New(isolate, JsRuntime::unwrap(JsString::newString(description)));
    return Local<JsSymbol>{v8Sym};
}
Local<JsSymbol> JsSymbol::newSymbol(const char* description) { return newSymbol(std::string_view{description}); }
Local<JsSymbol> JsSymbol::newSymbol(std::string const& description) { return newSymbol(description.c_str()); }

Local<JsSymbol> JsSymbol::forKey(Local<JsString> const& str) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsSymbol>{v8::Symbol::For(isolate, JsRuntime::unwrap(str))};
}


Local<JsFunction> JsFunction::newFunction(JsFunctionCallback cb) {
    // auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    // TODO: implement this
    throw std::runtime_error{"JsFunction::newFunction not implemented"};
}


Local<JsObject> JsObject::newObject() {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsObject>{v8::Object::New(isolate)};
}


Local<JsArray> JsArray::newArray(size_t length) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<JsArray>{v8::Array::New(isolate, static_cast<int>(length))};
}


} // namespace v8wrap