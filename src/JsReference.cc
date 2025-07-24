#include "v8wrap/JsReference.hpp"
#include "v8-exception.h"
#include "v8-primitive.h"
#include "v8-value.h"
#include "v8wrap/JsException.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsValue.hpp"
#include "v8wrap/internal/ArgsHelper.hpp"
#include <algorithm>


namespace v8wrap {


// Local<JsValue>
Local<JsValue>::Local() noexcept : val(){};
bool Local<JsValue>::isNull() const { return val->IsNull(); }
bool Local<JsValue>::isUndefined() const { return val->IsUndefined(); }
bool Local<JsValue>::isNullOrUndefined() const { return val->IsNullOrUndefined(); }
bool Local<JsValue>::isBoolean() const { return !isNullOrUndefined() && val->IsBoolean(); }
bool Local<JsValue>::isNumber() const { return !isNullOrUndefined() && val->IsNumber(); }
bool Local<JsValue>::isBigInt() const { return !isNullOrUndefined() && val->IsBigInt(); }
bool Local<JsValue>::isString() const { return !isNullOrUndefined() && val->IsString(); }
bool Local<JsValue>::isSymbol() const { return !isNullOrUndefined() && val->IsSymbol(); }
bool Local<JsValue>::isObject() const { return !isNullOrUndefined() && val->IsObject(); }
bool Local<JsValue>::isArray() const { return !isNullOrUndefined() && val->IsArray(); }
bool Local<JsValue>::isFunction() const { return !isNullOrUndefined() && val->IsFunction(); }

Local<JsValue> Local<JsValue>::asValue() const { return *this; }
Local<JsNull>  Local<JsValue>::asNull() const {
    if (isNull()) return Local<JsNull>{val.As<v8::Primitive>()};
    throw JsException("cannot convert to JsNull");
}
Local<JsUndefined> Local<JsValue>::asUndefined() const {
    if (isUndefined()) return Local<JsUndefined>{val.As<v8::Primitive>()};
    throw JsException("cannot convert to JsUndefined");
}
Local<JsBoolean> Local<JsValue>::asBoolean() const {
    if (isBoolean()) return Local<JsBoolean>{val.As<v8::Boolean>()};
    throw JsException("cannot convert to JsBoolean");
}
Local<JsNumber> Local<JsValue>::asNumber() const {
    if (isNumber()) return Local<JsNumber>{val.As<v8::Number>()};
    throw JsException("cannot convert to JsNumber");
}
Local<JsBigInt> Local<JsValue>::asBigInt() const {
    if (isBigInt()) return Local<JsBigInt>{val.As<v8::BigInt>()};
    throw JsException("cannot convert to JsBigInt");
}
Local<JsString> Local<JsValue>::asString() const {
    if (isString()) return Local<JsString>{val.As<v8::String>()};
    throw JsException("cannot convert to JsString");
}
Local<JsSymbol> Local<JsValue>::asSymbol() const {
    if (isSymbol()) return Local<JsSymbol>{val.As<v8::Symbol>()};
    throw JsException("cannot convert to JsSymbol");
}
Local<JsObject> Local<JsValue>::asObject() const {
    if (isObject()) return Local<JsObject>{val.As<v8::Object>()};
    throw JsException("cannot convert to JsObject");
}
Local<JsArray> Local<JsValue>::asArray() const {
    if (isArray()) return Local<JsArray>{val.As<v8::Array>()};
    throw JsException("cannot convert to JsArray");
}
Local<JsFunction> Local<JsValue>::asFunction() const {
    if (isFunction()) return Local<JsFunction>{val.As<v8::Function>()};
    throw JsException("cannot convert to JsFunction");
}

void Local<JsValue>::clear() { val.Clear(); }

JsValueType Local<JsValue>::getType() const {
    if (isNull()) return JsValueType::Null;
    if (isUndefined()) return JsValueType::Undefined;
    if (isBoolean()) return JsValueType::Boolean;
    if (isNumber()) return JsValueType::Number;
    if (isBigInt()) return JsValueType::BigInt;
    if (isString()) return JsValueType::String;
    if (isSymbol()) return JsValueType::Symbol;
    if (isObject()) return JsValueType::Object;
    if (isArray()) return JsValueType::Array;
    if (isFunction()) return JsValueType::Function;
    throw JsException("Unknown type, did you forget to add if branch?");
}


#define IMPL_SPECIALIZATION_LOCAL(VALUE)                                                                               \
    Local<VALUE>::~Local() = default;                                                                                  \
    Local<VALUE>::Local(Local<VALUE> const& cp) : val(cp.val) {}                                                       \
    Local<VALUE>::Local(Local<VALUE>&& mv) noexcept : val(mv.val) { mv.val.Clear(); }                                  \
    Local<VALUE>& Local<VALUE>::operator=(Local const& cp) {                                                           \
        if (&cp != this) {                                                                                             \
            val = cp.val;                                                                                              \
        }                                                                                                              \
        return *this;                                                                                                  \
    }                                                                                                                  \
    Local<VALUE>& Local<VALUE>::operator=(Local&& mv) noexcept {                                                       \
        if (&mv != this) {                                                                                             \
            val = mv.val;                                                                                              \
            mv.val.Clear();                                                                                            \
        }                                                                                                              \
        return *this;                                                                                                  \
    }                                                                                                                  \
    Local<JsString> Local<VALUE>::toString() {                                                                         \
        if (asValue().isNull()) return JsString::newString("null");                                                    \
        if (asValue().isUndefined()) return JsString::newString("undefined");                                          \
        auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();                                     \
        v8::TryCatch vtry{isolate};                                                                                    \
        auto         maybe = val->ToString(ctx);                                                                       \
        return Local<JsString>{maybe.ToLocalChecked()};                                                                \
    }                                                                                                                  \
    bool Local<VALUE>::operator==(Local<JsValue> const& other) const { return val->StrictEquals(other.val); }

#define IMPL_SPECALIZATION_AS_VALUE(VALUE)                                                                             \
    Local<JsValue> Local<VALUE>::asValue() const { return Local<JsValue>{val.As<v8::Value>()}; }

#define IMPL_SPECALIZATION_V8_LOCAL_TYPE(VALUE)                                                                        \
    Local<VALUE>::Local(v8::Local<Type> v8Type) : val{v8Type} {                                                        \
        if (val.IsEmpty()) throw JsException("Incorrect reference, v8::Local<T> is empty");                            \
    }


IMPL_SPECIALIZATION_LOCAL(JsValue)
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsValue)

IMPL_SPECIALIZATION_LOCAL(JsNull);
IMPL_SPECALIZATION_AS_VALUE(JsNull);
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsNull);

IMPL_SPECIALIZATION_LOCAL(JsUndefined);
IMPL_SPECALIZATION_AS_VALUE(JsUndefined);
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsUndefined);

IMPL_SPECIALIZATION_LOCAL(JsBoolean);
IMPL_SPECALIZATION_AS_VALUE(JsBoolean);
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsBoolean);
bool Local<JsBoolean>::getValue() const { return val->Value(); }

IMPL_SPECIALIZATION_LOCAL(JsNumber);
IMPL_SPECALIZATION_AS_VALUE(JsNumber);
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsNumber);
int    Local<JsNumber>::getInt32() const { return static_cast<int>(val->Value()); }
float  Local<JsNumber>::getFloat() const { return static_cast<float>(val->Value()); }
double Local<JsNumber>::getDouble() const { return static_cast<double>(val->Value()); }


IMPL_SPECIALIZATION_LOCAL(JsBigInt);
IMPL_SPECALIZATION_AS_VALUE(JsBigInt);
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsBigInt);
int64_t Local<JsBigInt>::getValue() const { return val->Int64Value(/* lossless? */); }


IMPL_SPECIALIZATION_LOCAL(JsString);
IMPL_SPECALIZATION_AS_VALUE(JsString);
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsString);
int         Local<JsString>::length() const { return val->Length(); }
std::string Local<JsString>::getValue() const {
    auto                  isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    v8::String::Utf8Value utf8(isolate, val);
    if (*utf8 == nullptr) {
        throw JsException("Cannot convert v8::String to std::string");
    }
    return std::string{*utf8, static_cast<size_t>(utf8.length())};
}


IMPL_SPECIALIZATION_LOCAL(JsSymbol);
IMPL_SPECALIZATION_AS_VALUE(JsSymbol);
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsSymbol);
Local<JsValue> Local<JsSymbol>::getDescription() {
    auto isolate = JsRuntimeScope::currentRuntimeIsolateChecked();
    auto maybe   = val->Description(isolate);
    return Local<JsValue>{maybe};
}


IMPL_SPECIALIZATION_LOCAL(JsObject);
IMPL_SPECALIZATION_AS_VALUE(JsObject);
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsObject);
bool Local<JsObject>::has(Local<JsString> const& key) const {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};
    auto         maybe = val->Has(ctx, key.val);
    JsException::rethrow(vtry);
    return maybe.ToChecked();
}

Local<JsValue> Local<JsObject>::get(Local<JsString> const& key) const {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};
    auto         maybe = val->Get(ctx, key.val);
    JsException::rethrow(vtry);
    return Local<JsValue>{maybe.ToLocalChecked()};
}

void Local<JsObject>::set(Local<JsString> const& key, Local<JsValue> const& value) {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};
    val->Set(ctx, key.val, value.val).ToChecked();
    JsException::rethrow(vtry);
}

void Local<JsObject>::remove(Local<JsString> const& key) {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};
    val->Delete(ctx, key.val).ToChecked();
    JsException::rethrow(vtry);
}

std::vector<Local<JsString>> Local<JsObject>::getOwnPropertyNames() const {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};

    auto maybe = val->GetOwnPropertyNames(ctx);
    JsException::rethrow(vtry);
    auto array = maybe.ToLocalChecked();

    std::vector<Local<JsString>> result;
    result.reserve(array->Length());

    for (uint32_t i = 0; i < array->Length(); ++i) {
        internal::V8EscapeScope scope{isolate};

        auto maybeVal = array->Get(ctx, i);
        JsException::rethrow(vtry);
        auto value = maybeVal.ToLocalChecked();
        if (value->IsString()) {
            result.push_back(Local<JsString>{scope.escape(value.As<v8::String>())});
        }
    }
    return result;
}

std::vector<std::string> Local<JsObject>::getOwnPropertyNamesAsString() const {
    auto keys = getOwnPropertyNames();

    std::vector<std::string> result;
    result.reserve(keys.size());
    std::transform(keys.begin(), keys.end(), std::back_inserter(result), [](Local<JsString> const& key) {
        return key.getValue();
    });
    return result;
}

bool Local<JsObject>::isInstanceOf(Local<JsValue> const& type) const {
    if (!type.isObject()) {
        return false;
    }
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};

    auto maybe = val->InstanceOf(ctx, type.asObject().val);
    JsException::rethrow(vtry);
    return maybe.ToChecked();
}


IMPL_SPECIALIZATION_LOCAL(JsArray);
IMPL_SPECALIZATION_AS_VALUE(JsArray);
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsArray);
size_t Local<JsArray>::length() const { return static_cast<size_t>(val->Length()); }

Local<JsValue> Local<JsArray>::get(size_t index) const {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};
    auto         maybe = val->Get(ctx, index);
    JsException::rethrow(vtry);
    return Local<JsValue>{maybe.ToLocalChecked()};
}

void Local<JsArray>::set(size_t index, Local<JsValue> const& value) {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};
    val->Set(ctx, index, value.val).ToChecked();
    JsException::rethrow(vtry);
}

void Local<JsArray>::push(Local<JsValue> const& value) {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};
    val->Set(ctx, val->Length(), value.val).ToChecked();
    JsException::rethrow(vtry);
}

void Local<JsArray>::clear() {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};

    // Method 1: Set length = 0
    auto len_str = v8::String::NewFromUtf8Literal(isolate, "length");
    val->Set(ctx, len_str, v8::Integer::New(isolate, 0)).ToChecked();

    JsException::rethrow(vtry);
}


IMPL_SPECIALIZATION_LOCAL(JsFunction);
IMPL_SPECALIZATION_AS_VALUE(JsFunction);
IMPL_SPECALIZATION_V8_LOCAL_TYPE(JsFunction);
bool Local<JsFunction>::isAsyncFunction() const { return val->IsAsyncFunction(); }

Local<JsValue> Local<JsFunction>::call() const { return call({}, {}); }

Local<JsValue> Local<JsFunction>::call(Local<JsValue> const& thiz, std::vector<Local<JsValue>> const& args) const {
    auto&& [isolate, ctx] = JsRuntimeScope::currentIsolateAndContextChecked();
    v8::TryCatch vtry{isolate};

    int  argc   = static_cast<int>(args.size());
    auto argv   = internal::extractArgs(args);
    auto result = val->Call(ctx, thiz.val, argc, argv);
    JsException::rethrow(vtry);
    return Local<JsValue>{result.ToLocalChecked()};
}


#undef IMPL_SPECALIZATION_LOCAL
#undef IMPL_SPECALIZATION_AS_VALUE
#undef IMPL_SPECALIZATION_V8_LOCAL_TYPE

} // namespace v8wrap