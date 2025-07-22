#pragma once
#include "v8wrap/Global.hpp"
#include <memory>


namespace v8wrap {

/**
    PointerWrapper 智能指针包装类
*/
template <typename T>
class PointerWrapper;

template <typename T>
class PointerWrapper<std::shared_ptr<T>> {
    std::shared_ptr<T> ptr;

    V8WRAP_DISALLOW_COPY(PointerWrapper)
};

template <typename T>
class PointerWrapper<std::unique_ptr<T>> {
    std::unique_ptr<T> ptr;

    V8WRAP_DISALLOW_COPY(PointerWrapper)
};


} // namespace v8wrap