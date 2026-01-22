# v8wrap - v8 binding library

- [中文](README.md) | [English](README_EN.md)

A non-intrusive, easy-to-use V8 binding library for embedded scenarios, supporting C++20 and friendly to runtime integration.

## Features

- [x] Platform.h – JavaScript platform manager, manages `Engine` (`v8::Platform`)
- [x] Engine.h – JavaScript runtime (`v8::Isolate` + `v8::Global<v8::Context>`)
  - [x] Resource management
  - [x] Registered bound classes
  - [x] Construct JavaScript classes from C++
  - [x] `isInstanceOf` type checking
  - [x] Unwrap native instances (obtain `Local<Object>` from a bound class instance)
- [x] EngineScope.h – V8 scope encapsulation (`v8::Locker` + `v8::Isolate::Scope` + `v8::HandleScope` + `v8::Context::Scope`)
- [x] Exception.h – V8 exception wrapper, bi-directional exception conversion
- [x] Value.h – V8 value wrapper, bi-directional value conversion
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
- [x] Local.h – V8 reference wrapper
  - [x] `Local<T>` -> `v8::Local<T>`
  - [x] `Global<T>` -> `v8::Global<T>`
  - [x] `Weak<T>` -> `v8::Global<T>`
- [x] TypeConverter.h – Type converter
  - [x] `bool` -> `Boolean`
  - [x] any number -> `Number`
  - [x] `int64_t` / `uint64_t` -> `BigInt`
  - [x] any string -> `String`
  - [x] any enum -> `Number` (cast to int)
  - [x] wrap type <-> Value
- [x] Bindings.h – C++ bindings
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
