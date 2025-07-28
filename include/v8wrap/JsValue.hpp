#pragma once
#include "Types.hpp"
#include "v8wrap/Concepts.hpp"
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
    [[nodiscard]] static Local<JsNull> newNull();
};

class JsUndefined : public JsValue {
public:
    [[nodiscard]] static Local<JsUndefined> newUndefined();
};

class JsBoolean : public JsValue {
public:
    [[nodiscard]] static Local<JsBoolean> newBoolean(bool b);
};

class JsNumber : public JsValue {
public:
    [[nodiscard]] static Local<JsNumber> newNumber(double d);
    [[nodiscard]] static Local<JsNumber> newNumber(int i);
    [[nodiscard]] static Local<JsNumber> newNumber(float f);
};

class JsBigInt : public JsValue {
public:
    template <typename T>
        requires IsI64<T>
    [[nodiscard]] static Local<JsBigInt> newBigInt(T i);

    template <typename T>
        requires IsU64<T>
    [[nodiscard]] static Local<JsBigInt> newBigInt(T u);
};

class JsString : public JsValue {
public:
    [[nodiscard]] static Local<JsString> newString(const char* str);
    [[nodiscard]] static Local<JsString> newString(std::string const& str);
    [[nodiscard]] static Local<JsString> newString(std::string_view str);
};

class JsSymbol : public JsValue {
public:
    [[nodiscard]] static Local<JsSymbol> newSymbol();
    [[nodiscard]] static Local<JsSymbol> newSymbol(std::string_view description);
    [[nodiscard]] static Local<JsSymbol> newSymbol(const char* description);
    [[nodiscard]] static Local<JsSymbol> newSymbol(std::string const& description);

    [[nodiscard]] static Local<JsSymbol> forKey(Local<JsString> const& str); // JavaScript: Symbol.for
};

class JsFunction : public JsValue {
public:
    /**
     * Create a JavaScript callable function.
     */
    template <typename T = JsFunctionCallback>
        requires IsJsFunctionCallback<T>
    [[nodiscard]] static Local<JsFunction> newFunction(T&& cb);

    /**
     * Binding wrapping arbitrary C++ functions, function pointers, lambdas, and callable objects.
     */
    template <typename Fn>
        requires(!IsJsFunctionCallback<Fn>)
    [[nodiscard]] static Local<JsFunction> newFunction(Fn&& func);

    

    /**
     * Function creation implementation.
     */
    [[nodiscard]] static Local<JsFunction> newFunctionImpl(JsFunctionCallback cb);
};

class JsObject : public JsValue {
public:
    [[nodiscard]] static Local<JsObject> newObject();
};

class JsArray : public JsValue {
public:
    [[nodiscard]] static Local<JsArray> newArray(size_t length = 0);
};


class Arguments {
    JsRuntime*                          mRuntime;
    v8::FunctionCallbackInfo<v8::Value> mArgs;

    explicit Arguments(JsRuntime* runtime, v8::FunctionCallbackInfo<v8::Value> const& args);

    friend class JsRuntime;
    friend class JsFunction;

public:
    V8WRAP_DISALLOW_COPY_AND_MOVE(Arguments);

    [[nodiscard]] JsRuntime* runtime() const;

    [[nodiscard]] bool hasThiz() const;

    [[nodiscard]] Local<JsObject> thiz() const; // this

    [[nodiscard]] size_t length() const;

    Local<JsValue> operator[](size_t index) const;
};


} // namespace v8wrap

#include "v8wrap/JsValue.inl"