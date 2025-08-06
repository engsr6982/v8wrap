#pragma once
#include "Global.hpp"
#include <functional>


V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-function.h>
V8_WRAP_WARNING_GUARD_END

namespace v8wrap {

/**
 * 前向声明头文件，防止循环依赖
 */

// 运行时
class JsPlatform;

class JsRuntime;

class JsException;

// 作用域
class JsRuntimeScope;

class ExitJsRuntimeScope;

// 值类型
enum class JsValueType;

class JsValue;

class JsNull;

class JsUndefined;

class JsBoolean;

class JsNumber;

class JsBigInt;

class JsString;

class JsSymbol;

class JsFunction;

class JsObject;

class JsArray;

class Arguments;

// 引用类型
template <typename T>
// requires std::is_base_of_v<JsValue, T>
class Local;

template <typename T>
// requires std::is_base_of_v<JsValue, T>
class Global;

template <typename T>
// requires std::is_base_of_v<JsValue, T>
class Weak;


using JsFunctionCallback = std::function<Local<JsValue>(Arguments const&)>;
using JsGetterCallback   = std::function<Local<JsValue>()>;
using JsSetterCallback   = std::function<void(Local<JsValue> const&)>;

// template <typename T>
using JsInstanceConstructor = std::function<void*(Arguments const& args)>;

// template <typename T>
using JsInstanceFunctionCallback = std::function<Local<JsValue>(void*, Arguments const& args)>;

// template <typename T>
using JsInstanceSetterCallback = std::function<void(void*, Local<JsValue> const& value)>;

// template <typename T>
using JsInstanceGetterCallback = std::function<Local<JsValue>(void*)>;


// 绑定相关
class ClassBinding;


namespace internal {

template <typename>
class V8GlobalRef;

}


// 类型别名
using PropertyAttribute  = v8::PropertyAttribute;
using PropertyDescriptor = v8::PropertyDescriptor;


} // namespace v8wrap