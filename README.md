# v8wrap - v8 封装库

- [x] JsPlatform.hpp - Js 平台,管理 `JsRuntime` (`v8::Platform`)
- [x] JsRuntime.hpp - Js 运行时 (`v8::Isolate` + `v8::Global<v8::Context>`)
- [x] JsRuntimeScope.hpp - v8 作用域封装 (`v8::Locker` + `v8::Isolate::Scope` + `v8::HandleScope` + `v8::Context::Scope`)
- [x] JsException.hpp - v8 异常封装，双向异常转换
- [x] JsValue.hpp - v8 值封装，双向值转换
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
  - [ ] ... (TODO)
- [x] JsReference.hpp - v8 引用封装
  - [x] `Local<T>` -> `v8::Local<T>`
  - [x] `Global<T>` -> `v8::Global<T>`
  - [x] `Weak<T>` -> `v8::Global<T>`
- [ ] TypeConverter.hpp - 类型转换器 (TODO)
  - [x] `bool` -> `JsBoolean`
  - [x] any number -> `JsNumber`
  - [x] `int64_t`/`uint64_t` -> `JsBigInt`
  - [x] any string -> `JsString`
  - [x] any enum -> `JsNumber`(cast to int)
  - [ ] ... (TODO)
- [ ] Native.hpp -> C++ 原生类型包装 (TODO)
- [ ] Bindings.hpp -> C++ 原生类型绑定 (TODO)

## 参考项目

- [v8pp](https://github.com/pmed/v8pp)
- [ScriptX](https://github.com/Tencent/ScriptX)
- [quickjspp](https://github.com/ftk/quickjspp)
- [puerts_node](https://github.com/puerts/puerts_node)
