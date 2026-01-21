# v8wrap - v8 封装库

- [中文](README.md) | [English](README_EN.md)

一个面向嵌入场景的 v8 绑定库，无侵入性，简单易用，支持 C++20，运行时友好。

## 功能

- [x] Platform.h - Js 平台,管理 `Engine` (`v8::Platform`)
- [x] Engine.h - Js 运行时 (`v8::Isolate` + `v8::Global<v8::Context>`)
  - [x] 资源管理
  - [x] 绑定类注册
  - [x] C++构造 Js 类
  - [x] isInstanceOf 类型判定
  - [x] 解包获取原生实例（从绑定的类实例获取 Local<Object>）
- [x] EngineScope.h - v8 作用域封装 (`v8::Locker` + `v8::Isolate::Scope` + `v8::HandleScope` + `v8::Context::Scope`)
- [x] Exception.h - v8 异常封装，双向异常转换
- [x] Value.h - v8 值封装，双向值转换
  - [x] `Value` -> `v8::Value`
  - [x] `Null` -> `v8::Primitive`
  - [x] `Undefined` -> `v8::Primitive`
  - [x] `Boolean` -> `v8::Boolean`
  - [x] `Number` -> `v8::Number`
  - [x] `BigInt` -> `v8::BigInt`
  - [x] `String` -> `v8::String`
  - [x] `Symbol` -> `v8::Symbol`
  - [x] `Function` -> `v8::Function`
  - [x] `Object` -> `v8::Object`
  - [x] `Array` -> `v8::Array`
- [x] Reference.h - v8 引用封装
  - [x] `Local<T>` -> `v8::Local<T>`
  - [x] `Global<T>` -> `v8::Global<T>`
  - [x] `Weak<T>` -> `v8::Global<T>`
- [x] TypeConverter.h - 类型转换器
  - [x] `bool` -> `Boolean`
  - [x] any number -> `Number`
  - [x] `int64_t`/`uint64_t` -> `BigInt`
  - [x] any string -> `String`
  - [x] any enum -> `Number`(cast to int)
  - [x] wrap type <-> Value
- [x] Bindings.h -> C++ 绑定
  - [x] 普通函数（重载支持）
  - [x] std::function（重载支持）
  - [x] lambda（重载支持）
  - [x] 可调用对象（重载支持）
  - [x] 类静态函数（重载支持）
  - [x] 类静态成员
  - [x] 类构造函数
  - [x] 类实例成员
  - [x] 类实例方法（重载支持）
  - [x] 类继承（基于 Js 原型链，不支持多继承）

## 参考项目

- [v8pp](https://github.com/pmed/v8pp)
- [ScriptX](https://github.com/Tencent/ScriptX)
- [quickjspp](https://github.com/ftk/quickjspp)
- [puerts_node](https://github.com/puerts/puerts_node)
