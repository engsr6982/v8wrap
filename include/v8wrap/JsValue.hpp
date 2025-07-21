#pragma once


namespace v8wrap {


class JsValue {};

class JsNull : public JsValue {};

class JsUndefined : public JsValue {};

class JsBoolean : public JsValue {};

class JsNumber : public JsValue {};

class JsBigInt : public JsValue {};

class JsString : public JsValue {};

class JsSymbol : public JsValue {};

class JsObject : public JsValue {};

class JsArray : public JsValue {};

class JsFunction : public JsValue {};

class JsAsyncFunction : public JsFunction {};


template <typename T>
class Local;

template <>
class Local<JsValue> {};


} // namespace v8wrap