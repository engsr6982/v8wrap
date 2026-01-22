#pragma once
#include "v8wrap/Global.h"
#include "v8wrap/Types.h"
#include "v8wrap/concepts/ScriptConcepts.h"
#include "v8wrap/types/internal/V8TypeAlias.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>


V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-function-callback.h>
V8_WRAP_WARNING_GUARD_END


namespace v8wrap {


enum class ValueType {
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

class Value {
public:
    Value() = delete;
};

class Null : public Value {
public:
    Null() = delete;
    [[nodiscard]] static Local<Null> newNull();
};

class Undefined : public Value {
public:
    Undefined() = delete;
    [[nodiscard]] static Local<Undefined> newUndefined();
};

class Boolean : public Value {
public:
    Boolean() = delete;
    [[nodiscard]] static Local<Boolean> newBoolean(bool b);
};

class Number : public Value {
public:
    Number() = delete;
    [[nodiscard]] static Local<Number> newNumber(double d);
    [[nodiscard]] static Local<Number> newNumber(int i);
    [[nodiscard]] static Local<Number> newNumber(float f);
};

class BigInt : public Value {
public:
    BigInt() = delete;

    template <typename T>
        requires std::same_as<T, int64_t>
    [[nodiscard]] static Local<BigInt> newBigInt(T i);

    template <typename T>
        requires std::same_as<T, uint64_t>
    [[nodiscard]] static Local<BigInt> newBigInt(T u);
};

class String : public Value {
public:
    String() = delete;
    [[nodiscard]] static Local<String> newString(const char* str);
    [[nodiscard]] static Local<String> newString(std::string const& str);
    [[nodiscard]] static Local<String> newString(std::string_view str);
};

class Symbol : public Value {
public:
    Symbol() = delete;
    [[nodiscard]] static Local<Symbol> newSymbol();
    [[nodiscard]] static Local<Symbol> newSymbol(std::string_view description);
    [[nodiscard]] static Local<Symbol> newSymbol(const char* description);
    [[nodiscard]] static Local<Symbol> newSymbol(std::string const& description);

    [[nodiscard]] static Local<Symbol> forKey(Local<String> const& str); // JavaScript: Symbol.for
};

class Function : public Value {
public:
    Function() = delete;
    /**
     * Create a JavaScript callable function.
     */
    template <typename T = FunctionCallback>
        requires concepts::JsFunctionCallback<T>
    [[nodiscard]] static Local<Function> newFunction(T&& cb);

    /**
     * Binding wrapping arbitrary C++ functions, function pointers, lambdas, and callable objects.
     */
    template <typename Fn>
        requires(!concepts::JsFunctionCallback<Fn>)
    [[nodiscard]] static Local<Function> newFunction(Fn&& func);

    /**
     * Bind any C++ overload function.
     */
    template <typename... Fn>
        requires(sizeof...(Fn) > 1 && (!concepts::JsFunctionCallback<Fn> && ...))
    [[nodiscard]] static Local<Function> newFunction(Fn&&... func);

    /**
     * Function creation implementation.
     */
    [[nodiscard]] static Local<Function> newFunctionImpl(FunctionCallback cb);
};

class Object : public Value {
public:
    Object() = delete;
    [[nodiscard]] static Local<Object> newObject();
};

class Array : public Value {
public:
    Array() = delete;
    [[nodiscard]] static Local<Array> newArray(size_t length = 0);
};

class Engine; // forward declaration
class Arguments {
    Engine*                             mRuntime;
    v8::FunctionCallbackInfo<v8::Value> mArgs;

    explicit Arguments(Engine* runtime, v8::FunctionCallbackInfo<v8::Value> const& args);

    friend class Engine;
    friend class Function;

public:
    V8WRAP_DISALLOW_COPY_AND_MOVE(Arguments);

    [[nodiscard]] Engine* runtime() const;

    [[nodiscard]] bool hasThiz() const;

    [[nodiscard]] Local<Object> thiz() const; // this

    [[nodiscard]] size_t length() const;

    Local<Value> operator[](size_t index) const;
};

struct ValueHelper {
    ValueHelper() = delete;

    template <typename T>
        requires concepts::JsValueType<T>
    [[nodiscard]] inline static v8::Local<internal::V8Type_v<T>> unwrap(Local<T> const& value);

    template <typename T>
    [[nodiscard]] inline static Local<T> wrap(v8::Local<internal::V8Type_v<T>> const& value);
};

} // namespace v8wrap

#include "Value.inl"