# v8wrap - v8 binding library

- [中文](README.md) | [English](README_EN.md)

A non-intrusive, easy-to-use V8 binding library for embedded scenarios, supporting C++20 and friendly to runtime integration.

## Features

- [x] JsPlatform.hpp – JavaScript platform manager, manages `JsRuntime` (`v8::Platform`)
- [x] JsRuntime.hpp – JavaScript runtime (`v8::Isolate` + `v8::Global<v8::Context>`)
  - [x] Resource management
  - [x] Registered bound classes
  - [x] Construct JavaScript classes from C++
  - [x] `isInstanceOf` type checking
  - [x] Unwrap native instances (obtain `Local<JsObject>` from a bound class instance)
- [x] JsRuntimeScope.hpp – V8 scope encapsulation (`v8::Locker` + `v8::Isolate::Scope` + `v8::HandleScope` + `v8::Context::Scope`)
- [x] JsException.hpp – V8 exception wrapper, bi-directional exception conversion
- [x] JsValue.hpp – V8 value wrapper, bi-directional value conversion
  - [x] `JsValue` -> `v8::Value`
  - [x] `JsNull` -> `v8::Primitive`
  - [x] `JsUndefined` -> `v8::Primitive`
  - [x] `JsBoolean` -> `v8::Boolean`
  - [x] `JsNumber` -> `v8::Number`
  - [x] `JsBigInt` -> `v8::BigInt`
  - [x] `JsString` -> `v8::String`
  - [x] `JsSymbol` -> `v8::Symbol`
  - [x] `JsFunction` -> `v8::Function`
  - [x] `JsObject` -> `v8::Object`
  - [x] `JsArray` -> `v8::Array`
- [x] JsReference.hpp – V8 reference wrapper
  - [x] `Local<T>` -> `v8::Local<T>`
  - [x] `Global<T>` -> `v8::Global<T>`
  - [x] `Weak<T>` -> `v8::Global<T>`
- [x] TypeConverter.hpp – Type converter
  - [x] `bool` -> `JsBoolean`
  - [x] any number -> `JsNumber`
  - [x] `int64_t` / `uint64_t` -> `JsBigInt`
  - [x] any string -> `JsString`
  - [x] any enum -> `JsNumber` (cast to int)
  - [x] wrap type <-> JsValue
- [x] Bindings.hpp – C++ bindings
  - [x] Regular functions (overload supported)
  - [x] `std::function` (overload supported)
  - [x] Lambda (overload supported)
  - [x] Callable objects (overload supported)
  - [x] Class static functions (overload supported)
  - [x] Class static members
  - [x] Class constructors
  - [x] Class instance members
  - [x] Class instance methods (overload supported)
  - [x] Class inheritance (based on JavaScript prototype chain, no multiple inheritance support)

## References

- [v8pp](https://github.com/pmed/v8pp)
- [ScriptX](https://github.com/Tencent/ScriptX)
- [quickjspp](https://github.com/ftk/quickjspp)
- [puerts_node](https://github.com/puerts/puerts_node)
