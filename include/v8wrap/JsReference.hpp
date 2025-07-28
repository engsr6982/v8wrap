#pragma once
#include "v8wrap/Concepts.hpp"
#include "v8wrap/Global.hpp"
#include "v8wrap/Types.hpp"
#include "v8wrap/internal/V8TypeAlias.hpp"
#include <cstdint>
#include <string>
#include <type_traits>
#include <v8-function.h>
#include <v8-local-handle.h>
#include <v8-primitive.h>
#include <vector>


namespace v8wrap {

#define SPECIALIZATION_LOCAL(VALUE)                                                                                    \
public:                                                                                                                \
    V8WRAP_DISALLOW_NEW();                                                                                             \
    ~Local();                                                                                                          \
    Local(Local<VALUE> const&);                                                                                        \
    Local(Local<VALUE>&&) noexcept;                                                                                    \
    Local<VALUE>& operator=(Local const&);                                                                             \
    Local<VALUE>& operator=(Local&&) noexcept;                                                                         \
                                                                                                                       \
    Local<JsString> toString();                                                                                        \
    bool            operator==(Local<JsValue> const& other) const;                                                     \
                                                                                                                       \
private:                                                                                                               \
    friend class JsRuntime;                                                                                            \
    friend class JsException;                                                                                          \
    friend class VALUE;                                                                                                \
    template <typename>                                                                                                \
    friend class Local;                                                                                                \
    template <typename>                                                                                                \
    friend class Global;                                                                                               \
    template <typename>                                                                                                \
    friend class Weak;

#define SPECALIZATION_AS_VALUE(VALUE)                                                                                  \
public:                                                                                                                \
    Local<JsValue> asValue() const;                                                                                    \
    operator Local<JsValue>() const { return asValue(); }                                                              \
    bool operator==(Local<VALUE> const& other) const { return operator==(other.asValue()); }

#define SPECALIZATION_V8_LOCAL_TYPE(VALUE)                                                                             \
private:                                                                                                               \
    using Type = internal::V8Type_v<VALUE>;                                                                            \
    explicit Local(v8::Local<Type>);                                                                                   \
    v8::Local<Type> val


template <typename T>
class Local final {
    static_assert(std::is_base_of_v<JsValue, T>, "T must be derived from JsValue");
};

template <>
class Local<JsValue> {
    SPECIALIZATION_LOCAL(JsValue);
    SPECALIZATION_V8_LOCAL_TYPE(JsValue);

    friend class Arguments;

public:
    // default v8::Undefined -> undefined
    Local() noexcept;

    [[nodiscard]] JsValueType getType() const;

    [[nodiscard]] bool isNull() const;
    [[nodiscard]] bool isUndefined() const;
    [[nodiscard]] bool isNullOrUndefined() const; // null or undefined
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

    /**
     * @tparam T must be the type of as described above
     */
    template <typename T>
        requires IsWrappedV8Type<T>
    [[nodiscard]] Local<T> as() const;

    void clear();
};

template <>
class Local<JsNull> {
    SPECIALIZATION_LOCAL(JsNull);
    SPECALIZATION_AS_VALUE(JsNull);
    SPECALIZATION_V8_LOCAL_TYPE(JsNull);
};

template <>
class Local<JsUndefined> {
    SPECIALIZATION_LOCAL(JsUndefined);
    SPECALIZATION_AS_VALUE(JsUndefined);
    SPECALIZATION_V8_LOCAL_TYPE(JsUndefined);
};

template <>
class Local<JsBoolean> {
    SPECIALIZATION_LOCAL(JsBoolean);
    SPECALIZATION_AS_VALUE(JsBoolean);
    SPECALIZATION_V8_LOCAL_TYPE(JsBoolean);

public:
    [[nodiscard]] bool getValue() const;
};

template <>
class Local<JsNumber> {
    SPECIALIZATION_LOCAL(JsNumber);
    SPECALIZATION_AS_VALUE(JsNumber);
    SPECALIZATION_V8_LOCAL_TYPE(JsNumber);

public:
    [[nodiscard]] int    getInt32() const;
    [[nodiscard]] float  getFloat() const;
    [[nodiscard]] double getDouble() const;

    template <typename T>
        requires std::is_integral_v<T> || std::is_floating_point_v<T>
    [[nodiscard]] T getValueAs() const {
        return static_cast<T>(getDouble());
    }
};

template <>
class Local<JsBigInt> {
    SPECIALIZATION_LOCAL(JsBigInt);
    SPECALIZATION_AS_VALUE(JsBigInt);
    SPECALIZATION_V8_LOCAL_TYPE(JsBigInt);

public:
    [[nodiscard]] int64_t  getInt64() const;
    [[nodiscard]] uint64_t getUint64() const;
};

template <>
class Local<JsString> {
    SPECIALIZATION_LOCAL(JsString);
    SPECALIZATION_AS_VALUE(JsString);
    SPECALIZATION_V8_LOCAL_TYPE(JsString);

public:
    [[nodiscard]] int         length() const;
    [[nodiscard]] std::string getValue() const;
};

template <>
class Local<JsSymbol> {
    SPECIALIZATION_LOCAL(JsSymbol);
    SPECALIZATION_AS_VALUE(JsSymbol);
    SPECALIZATION_V8_LOCAL_TYPE(JsSymbol);

public:
    Local<JsValue> getDescription(); // maybe undefined
};

template <>
class Local<JsObject> {
    SPECIALIZATION_LOCAL(JsObject);
    SPECALIZATION_AS_VALUE(JsObject);
    SPECALIZATION_V8_LOCAL_TYPE(JsObject);

    friend class Arguments;

public:
    [[nodiscard]] bool has(Local<JsString> const& key) const;

    [[nodiscard]] Local<JsValue> get(Local<JsString> const& key) const;

    void set(Local<JsString> const& key, Local<JsValue> const& value);

    void remove(Local<JsString> const& key);

    [[nodiscard]] std::vector<Local<JsString>> getOwnPropertyNames() const;

    [[nodiscard]] std::vector<std::string> getOwnPropertyNamesAsString() const;

    [[nodiscard]] bool instanceof(Local<JsValue> const& type) const;

    [[nodiscard]] bool defineOwnProperty(
        Local<JsString> const& key,
        Local<JsValue> const&  value,
        PropertyAttribute      attrs = PropertyAttribute::None
    ) const;

    [[nodiscard]] bool defineProperty(Local<JsString> const& key, PropertyDescriptor& desc) const;
};

template <>
class Local<JsArray> {
    SPECIALIZATION_LOCAL(JsArray);
    SPECALIZATION_AS_VALUE(JsArray);
    SPECALIZATION_V8_LOCAL_TYPE(JsArray);

public:
    [[nodiscard]] size_t length() const;

    [[nodiscard]] Local<JsValue> get(size_t index) const;

    void set(size_t index, Local<JsValue> const& value);

    void push(Local<JsValue> const& value);

    void clear();
};

template <>
class Local<JsFunction> {
    SPECIALIZATION_LOCAL(JsFunction);
    SPECALIZATION_AS_VALUE(JsFunction);
    SPECALIZATION_V8_LOCAL_TYPE(JsFunction);

public:
    [[nodiscard]] bool isAsyncFunction() const; // JavaScript: async function

    Local<JsValue> call() const;

    Local<JsValue> call(Local<JsValue> const& thiz, std::vector<Local<JsValue>> const& args) const;
};

#undef SPECIALIZATION_LOCAL
#undef SPECALIZATION_AS_VALUE
#undef SPECALIZATION_V8_LOCAL_TYPE


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

#include "v8wrap/JsReference.inl"
