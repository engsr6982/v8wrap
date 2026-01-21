#include "v8wrap/types/Value.h"
#include "v8wrap/JsException.h"
#include "v8wrap/JsRuntime.h"
#include "v8wrap/JsRuntimeScope.h"
#include "v8wrap/reference/Reference.h"
#include <string_view>


V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-exception.h>
#include <v8-external.h>
#include <v8-function-callback.h>
#include <v8-local-handle.h>
#include <v8-primitive.h>
#include <v8-template.h>
#include <v8-value.h>
V8_WRAP_WARNING_GUARD_END

namespace v8wrap {


Local<Null> Null::newNull() {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<Null>{v8::Null(isolate)};
}


Local<Undefined> Undefined::newUndefined() {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<Undefined>{v8::Undefined(isolate)};
}


Local<Boolean> Boolean::newBoolean(bool b) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<Boolean>{v8::Boolean::New(isolate, b)};
}


Local<Number> Number::newNumber(double d) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<Number>{v8::Number::New(isolate, d)};
}
Local<Number> Number::newNumber(int i) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<Number>{v8::Number::New(isolate, i)};
}
Local<Number> Number::newNumber(float f) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<Number>{v8::Number::New(isolate, f)};
}


Local<String> String::newString(const char* str) { return newString(std::string_view{str}); }
Local<String> String::newString(std::string const& str) { return newString(str.c_str()); }
Local<String> String::newString(std::string_view str) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();

    v8::TryCatch vtry{isolate};

    auto v8Str =
        v8::String::NewFromUtf8(isolate, str.data(), v8::NewStringType::kNormal, static_cast<int>(str.length()));
    JsException::rethrow(vtry);
    return Local<String>{v8Str.ToLocalChecked()};
}

Local<Symbol> Symbol::newSymbol() {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<Symbol>{v8::Symbol::New(isolate)};
}
Local<Symbol> Symbol::newSymbol(std::string_view description) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    auto v8Sym   = v8::Symbol::New(isolate, JsValueHelper::unwrap(String::newString(description)));
    return Local<Symbol>{v8Sym};
}
Local<Symbol> Symbol::newSymbol(const char* description) { return newSymbol(std::string_view{description}); }
Local<Symbol> Symbol::newSymbol(std::string const& description) { return newSymbol(description.c_str()); }

Local<Symbol> Symbol::forKey(Local<String> const& str) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<Symbol>{v8::Symbol::For(isolate, JsValueHelper::unwrap(str))};
}


Local<Function> Function::newFunctionImpl(FunctionCallback cb) {
    struct AssociateResources {
        JsRuntime*       runtime{nullptr};
        FunctionCallback cb;
    };

    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();

    auto vtry = v8::TryCatch{isolate};
    auto data = std::make_unique<AssociateResources>(JsRuntimeScope::currentRuntime(), std::move(cb));

    auto external = v8::External::New(isolate, static_cast<void*>(data.get())).As<v8::Value>();
    auto temp     = v8::FunctionTemplate::New(
        isolate,
        [](v8::FunctionCallbackInfo<v8::Value> const& info) {
            auto data = reinterpret_cast<AssociateResources*>(info.Data().As<v8::External>()->Value());
            auto args = Arguments{data->runtime, info};
            try {
                auto returnValue = data->cb(args); // call native
                info.GetReturnValue().Set(JsValueHelper::unwrap(returnValue));
            } catch (JsException const& e) {
                e.rethrowToRuntime(); // throw to v8 (js)
            }
        },
        external
    );
    temp->RemovePrototype();

    auto v8Func = temp->GetFunction(ctx);
    JsException::rethrow(vtry);

    JsRuntimeScope::currentRuntimeChecked().addManagedResource(data.release(), v8Func.ToLocalChecked(), [](void* data) {
        delete reinterpret_cast<AssociateResources*>(data);
    });

    return Local<Function>{v8Func.ToLocalChecked()};
}


Local<Object> Object::newObject() {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<Object>{v8::Object::New(isolate)};
}


Local<Array> Array::newArray(size_t length) {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    return Local<Array>{v8::Array::New(isolate, static_cast<int>(length))};
}


Arguments::Arguments(JsRuntime* runtime, v8::FunctionCallbackInfo<v8::Value> const& args)
: mRuntime(runtime),
  mArgs(args) {}

JsRuntime* Arguments::runtime() const { return mRuntime; }

bool Arguments::hasThiz() const { return mArgs.This()->IsObject(); }

Local<Object> Arguments::thiz() const {
    if (!hasThiz()) {
        throw JsException{"Arguments::thiz(): no thiz"};
    }
    return Local<Object>{mArgs.This()};
}

size_t Arguments::length() const { return static_cast<size_t>(mArgs.Length()); }

Local<Value> Arguments::operator[](size_t index) const {
    auto value = mArgs[static_cast<int>(index)];
    return Local<Value>{value};
}


} // namespace v8wrap