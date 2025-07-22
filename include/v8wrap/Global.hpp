#pragma once
#include <v8-version.h>

namespace v8wrap {


#define V8WRAP_DISALLOW_COPY(T)                                                                                        \
    T(const T&)            = delete;                                                                                   \
    T& operator=(const T&) = delete;

#define V8WRAP_DISALLOW_MOVE(T)                                                                                        \
    T(T&&)            = delete;                                                                                        \
    T& operator=(T&&) = delete;

#define V8WRAP_DISALLOW_COPY_AND_MOVE(T)                                                                               \
    V8WRAP_DISALLOW_COPY(T);                                                                                           \
    V8WRAP_DISALLOW_MOVE(T);

#define V8WRAP_DISALLOW_NEW()                                                                                          \
    static void* operator new(std::size_t)                          = delete;                                          \
    static void* operator new(std::size_t, const std::nothrow_t&)   = delete;                                          \
    static void* operator new[](std::size_t)                        = delete;                                          \
    static void* operator new[](std::size_t, const std::nothrow_t&) = delete;


#define V8WRAP_SUPPORTED_MIN_V8_MAJOR_VERSION 12
#define V8WRAP_SUPPORTED_MIN_V8_MINOR_VERSION 4

// #define V8WRAP_SKIP_CHECK_V8_VERSION

#ifndef V8WRAP_SKIP_CHECK_V8_VERSION
static_assert(
    !((V8_MAJOR_VERSION < V8WRAP_SUPPORTED_MIN_V8_MAJOR_VERSION)
      || (V8_MINOR_VERSION < V8WRAP_SUPPORTED_MIN_V8_MINOR_VERSION)),
    "v8wrap requires at least V8 version 12.4"
);
#endif


} // namespace v8wrap