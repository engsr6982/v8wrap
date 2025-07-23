#pragma once
#include "Types.hpp"
#include "v8wrap/Global.hpp"
#include <cstddef>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <v8-function-callback.h>


namespace v8wrap {


enum class JsValueType {
    Null = 0,
    Undefined,
    Boolean,
    Number,
    BigInt,
    String,
    Symbol,
    Object,
    Array,
    Function,
    // TODO: Promise、(ArrayBuffer/TypedArray)、(Map/Set)、Date、RegExp、Proxy
};

class JsValue {};

class JsNull : public JsValue {
public:
    static Local<JsNull> newNull();
};

class JsUndefined : public JsValue {
public:
    static Local<JsUndefined> newUndefined();
};

class JsBoolean : public JsValue {
public:
    static Local<JsBoolean> newBoolean(bool b);
};

class JsNumber : public JsValue {
public:
    static Local<JsNumber> newNumber(double d);
    static Local<JsNumber> newNumber(int i);
    static Local<JsNumber> newNumber(float f);
};

class JsBigInt : public JsValue {
public:
    static Local<JsBigInt> newBigInt(int64_t i);
};

class JsString : public JsValue {
public:
    static Local<JsString> newString(const char* str);
    static Local<JsString> newString(std::string const& str);
    static Local<JsString> newString(std::string_view str);
};

class JsSymbol : public JsValue {
public:
    static Local<JsSymbol> newSymbol();
    static Local<JsSymbol> newSymbol(std::string_view description);
    static Local<JsSymbol> newSymbol(const char* description);
    static Local<JsSymbol> newSymbol(std::string const& description);

    static Local<JsSymbol> forKey(Local<JsString> const& str); // JavaScript: Symbol.for
};

class JsFunction : public JsValue {
public:
    static Local<JsFunction> newFunction(JsFunctionCallback cb);

    template <typename Fn>
    static Local<JsFunction> newFunction(Fn&& func);
};

class JsObject : public JsValue {
public:
    static Local<JsObject> newObject();
};

class JsArray : public JsValue {
public:
    static Local<JsArray> newArray(size_t length = 0);
};


class Arguments {
    JsRuntime*                          mRuntime;
    v8::FunctionCallbackInfo<v8::Value> mArgs;

    explicit Arguments(JsRuntime* runtime, v8::FunctionCallbackInfo<v8::Value> const& args);

    friend class JsRuntime;

public:
    V8WRAP_DISALLOW_COPY_AND_MOVE(Arguments);

    [[nodiscard]] JsRuntime* runtime() const;

    [[nodiscard]] bool hasThiz() const;

    [[nodiscard]] Local<JsObject> thiz() const; // this

    [[nodiscard]] size_t length() const;

    Local<JsValue> operator[](size_t index) const;
};


} // namespace v8wrap