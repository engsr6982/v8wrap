#pragma once
#include "v8wrap/Global.h"
#include "v8wrap/Types.h"

V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-function.h>
#include <v8-object.h>
#include <v8-primitive.h>
#include <v8-value.h>
V8_WRAP_WARNING_GUARD_END


namespace v8wrap::internal {


template <typename T>
struct V8TypeAlias {
    static_assert(sizeof(T) == 0, "V8TypeAlias not defined for this type");
};

template <typename T>
using V8Type_v = typename V8TypeAlias<T>::type;


#define TYPE_ALIAS(WRAP_TYPE, V8_TYPE)                                                                                 \
    template <>                                                                                                        \
    struct V8TypeAlias<WRAP_TYPE> {                                                                                    \
        using type = V8_TYPE;                                                                                          \
    };

TYPE_ALIAS(JsValue, v8::Value);
TYPE_ALIAS(JsNull, v8::Primitive);
TYPE_ALIAS(JsUndefined, v8::Primitive);
TYPE_ALIAS(JsBoolean, v8::Boolean);
TYPE_ALIAS(JsNumber, v8::Number);
TYPE_ALIAS(JsBigInt, v8::BigInt);
TYPE_ALIAS(JsString, v8::String);
TYPE_ALIAS(JsSymbol, v8::Symbol);
TYPE_ALIAS(JsFunction, v8::Function);
TYPE_ALIAS(JsObject, v8::Object);
TYPE_ALIAS(JsArray, v8::Array);


#undef TYPE_ALIAS

} // namespace v8wrap::internal