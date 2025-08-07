#pragma once
#include "JsRuntime.hpp"
#include "v8wrap/Types.hpp"


namespace v8wrap {


template <typename T>
std::shared_ptr<T> JsRuntime::getData() const {
    return std::static_pointer_cast<T>(mUserData);
}

template <typename T>
    requires StringLike<T>
Local<JsValue> JsRuntime::eval(T const& str) {
    return eval(JsString::newString(str));
}

template <typename T>
T* JsRuntime::getNativeInstanceOf(Local<JsObject> const& obj) const {
    return static_cast<T*>(this->getNativeInstanceOf(obj));
}


} // namespace v8wrap