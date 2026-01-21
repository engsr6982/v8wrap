#pragma once
#include "Global.h"
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
enum class ValueType;

class Value;
class Null;
class Undefined;
class Boolean;
class Number;
class BigInt;
class String;
class Symbol;
class Function;
class Object;
class Array;

class Arguments;

// 引用类型
template <typename>
class Local;

template <typename>
class Global;

template <typename>
class Weak;


using FunctionCallback = std::function<Local<Value>(Arguments const&)>;
using GetterCallback   = std::function<Local<Value>()>;
using SetterCallback   = std::function<void(Local<Value> const&)>;

using InstanceConstructor    = std::function<void*(Arguments const& args)>;
using InstanceMethodCallback = std::function<Local<Value>(void*, Arguments const& args)>;
using InstanceGetterCallback = std::function<Local<Value>(void*, Arguments const& args)>;
using InstanceSetterCallback = std::function<void(void*, Arguments const& args)>;


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