#pragma once
#include "Types.hpp"
#include <string>
#include <string_view>


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
    static Local<JsSymbol> newSymbol(std::string_view str);
    static Local<JsSymbol> newSymbol(const char* str);
    static Local<JsSymbol> newSymbol(std::string const& str);

    static Local<JsSymbol> forKey(Local<JsString> const& str); // JavaScript: Symbol.for
    static Local<JsString> keyFor(Local<JsSymbol> const& sym); // JavaScript: Symbol.keyFor
};

class JsFunction : public JsValue {
public:
    static Local<JsFunction> newFunction(JsFunctionCallback cb, bool isAsync = false);

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
    // TODO: implement
};


} // namespace v8wrap