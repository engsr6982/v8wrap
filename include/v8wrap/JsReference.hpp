#pragma once
#include "v8wrap/Global.hpp"
#include "v8wrap/Types.hpp"
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <v8-function.h>
#include <v8-local-handle.h>
#include <v8-primitive.h>
#include <vector>


namespace v8wrap {

#define SPECIALIZATION_LOCAL(VALUE)                                                                                    \
public:                                                                                                                \
    Local<JsString> toString();                                                                                        \
    V8WRAP_DISALLOW_NEW();                                                                                             \
    ~Local();                                                                                                          \
    Local(Local<VALUE> const&);                                                                                        \
    Local(Local<VALUE>&&) noexcept;                                                                                    \
    Local<VALUE>& operator=(Local const&);                                                                             \
    Local<VALUE>& operator=(Local&&) noexcept;                                                                         \
                                                                                                                       \
    bool operator==(Local<JsValue> const& other) const;                                                                \
                                                                                                                       \
private:                                                                                                               \
    friend class JsRuntime;                                                                                            \
    friend class JsException;                                                                                          \
    template <typename>                                                                                                \
    friend class Global;                                                                                               \
    template <typename>                                                                                                \
    friend class Weak;

#define SPECALIZATION_NON_VALUE(VALUE)                                                                                 \
public:                                                                                                                \
    Local<JsValue> asValue() const;                                                                                    \
    operator Local<JsValue>() const { return asValue(); }                                                              \
    bool operator==(Local<VALUE> const& other) const { return operator==(other.asValue()); }


template <typename T>
class Local final {
    static_assert(std::is_base_of_v<JsValue, T>, "T must be derived from JsValue");
};

template <>
class Local<JsValue> {
public:
    // default v8::Undefined -> undefined
    Local() noexcept;

    JsValueType getType();

    [[nodiscard]] bool isNull() const;
    [[nodiscard]] bool isUndefined() const;
    [[nodiscard]] bool isBoolean() const;
    [[nodiscard]] bool isNumber() const;
    [[nodiscard]] bool isBigInt() const;
    [[nodiscard]] bool isString() const;
    [[nodiscard]] bool isSymbol() const;
    [[nodiscard]] bool isObject() const;
    [[nodiscard]] bool isArray() const;
    [[nodiscard]] bool isFunction() const;

    [[nodiscard]] Local<JsValue>     asValue() const;
    [[nodiscard]] Local<JsNull>      asNull() const;
    [[nodiscard]] Local<JsUndefined> asUndefined() const;
    [[nodiscard]] Local<JsBoolean>   asBoolean() const;
    [[nodiscard]] Local<JsNumber>    asNumber() const;
    [[nodiscard]] Local<JsBigInt>    asBigInt() const;
    [[nodiscard]] Local<JsString>    asString() const;
    [[nodiscard]] Local<JsSymbol>    asSymbol() const;
    [[nodiscard]] Local<JsObject>    asObject() const;
    [[nodiscard]] Local<JsArray>     asArray() const;
    [[nodiscard]] Local<JsFunction>  asFunction() const;

    void reset();

    SPECIALIZATION_LOCAL(JsValue)
};

template <>
class Local<JsNull> {
    SPECIALIZATION_LOCAL(JsNull);
    SPECALIZATION_NON_VALUE(JsNull);

private:
    v8::Local<v8::Primitive> val;
};

template <>
class Local<JsUndefined> {
    SPECIALIZATION_LOCAL(JsUndefined);
    SPECALIZATION_NON_VALUE(JsUndefined);

private:
    v8::Local<v8::Primitive> val;
};

template <>
class Local<JsBoolean> {
    SPECIALIZATION_LOCAL(JsBoolean);
    SPECALIZATION_NON_VALUE(JsBoolean);

public:
    [[nodiscard]] bool getValue() const;

private:
    v8::Local<v8::Boolean> val;
};

template <>
class Local<JsNumber> {
    SPECIALIZATION_LOCAL(JsNumber);
    SPECALIZATION_NON_VALUE(JsNumber);

public:
    [[nodiscard]] int    getInt32() const;
    [[nodiscard]] float  getFloat() const;
    [[nodiscard]] double getDouble() const;

private:
    v8::Local<v8::Number> val;
};

template <>
class Local<JsBigInt> {
    SPECIALIZATION_LOCAL(JsBigInt);
    SPECALIZATION_NON_VALUE(JsBigInt);

public:
    [[nodiscard]] int64_t getValue() const;

private:
    v8::Local<v8::BigInt> val;
};

template <>
class Local<JsString> {
    SPECIALIZATION_LOCAL(JsString);
    SPECALIZATION_NON_VALUE(JsString);

public:
    [[nodiscard]] std::string getValue() const;

private:
    v8::Local<v8::String> val;
};

template <>
class Local<JsSymbol> {
    SPECIALIZATION_LOCAL(JsSymbol);
    SPECALIZATION_NON_VALUE(JsSymbol);

public:
    Local<JsValue> getDescription(); // maybe undefined

private:
    v8::Local<v8::Symbol> val;
};

template <>
class Local<JsObject> {
    SPECIALIZATION_LOCAL(JsObject);
    SPECALIZATION_NON_VALUE(JsObject);

public:
    [[nodiscard]] bool has(Local<JsString> const& key) const;

    [[nodiscard]] Local<JsValue> get(Local<JsString> const& key) const;

    void set(Local<JsString> const& key, Local<JsValue> const& value);

    void remove(Local<JsString> const& key);

    [[nodiscard]] std::vector<Local<JsString>> getOwnPropertyNames() const;

    [[nodiscard]] std::vector<std::string> getOwnPropertyNamesAsString() const;

    [[nodiscard]] bool isInstanceOf(Local<JsValue> const& type) const;

private:
    v8::Local<v8::Object> val;
};

template <>
class Local<JsArray> {
    SPECIALIZATION_LOCAL(JsArray);
    SPECALIZATION_NON_VALUE(JsArray);

public:
    [[nodiscard]] size_t length() const;

    [[nodiscard]] Local<JsValue> get(size_t index) const;

    void set(size_t index, Local<JsValue> const& value);

    void push(Local<JsValue> const& value);

    void clear();

private:
    v8::Local<v8::Array> val;
};

template <>
class Local<JsFunction> {
    SPECIALIZATION_LOCAL(JsFunction);
    SPECALIZATION_NON_VALUE(JsFunction);

public:
    Local<JsValue> call() const;

    Local<JsValue> call(Local<JsValue> const& thiz, std::vector<Local<JsValue>> const& args) const;

    Local<JsValue> call(Local<JsValue> const& thiz, std::initializer_list<Local<JsValue>> const& args) const;

    [[nodiscard]] bool isAsyncFunction() const; // JavaScript: async function

private:
    v8::Local<v8::Function> val;
};

#undef SPECIALIZATION_LOCAL
#undef SPECALIZATION_NON_VALUE


template <typename T>
class Global final {
    static_assert(std::is_base_of_v<JsValue, T>, "T must be derived from JsValue");

public:
    V8WRAP_DISALLOW_COPY(Global); // v8::Global is not copyable

    Global() noexcept; // empty

    explicit Global(Local<T> const& val);
    explicit Global(Weak<T> const& val);

    Global(Global<T>&& other) noexcept;
    Global& operator=(Global<T>&& other) noexcept;

    ~Global();

    [[nodiscard]] Local<T> get() const;

    [[nodiscard]] Local<JsValue> getValue() const;

    [[nodiscard]] bool isEmpty() const;

    void reset();

private:
    internal::V8GlobalRef<T> impl;

    friend class JsRuntime;

    template <typename>
    friend class Local;
    template <typename>
    friend class Weak;
};

template <typename T>
class Weak final {
    static_assert(std::is_base_of_v<JsValue, T>, "T must be derived from JsValue");

public:
    V8WRAP_DISALLOW_COPY(Weak); // v8::Global is not copyable

    Weak() noexcept; // empty

    explicit Weak(Local<T> const& val);
    explicit Weak(Global<T> const& val);

    Weak(Weak<T>&& other) noexcept;
    Weak& operator=(Weak<T>&& other) noexcept;

    ~Weak();

    [[nodiscard]] Local<T> get() const;

    [[nodiscard]] Local<JsValue> getValue() const;

    [[nodiscard]] bool isEmpty() const;

    void reset();

private:
    internal::V8GlobalRef<T> impl;

    friend class JsRuntime;

    template <typename>
    friend class Local;
    template <typename>
    friend class Weak;
};


} // namespace v8wrap
