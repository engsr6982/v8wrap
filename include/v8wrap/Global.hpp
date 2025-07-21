#pragma once
#include <v8-version.h>

namespace v8wrap {


#define V8WRAP_DISALLOW_COPY_AND_MOVE(T)                                                                               \
    T(const T&)            = delete;                                                                                   \
    T& operator=(const T&) = delete;                                                                                   \
    T(T&&)                 = delete;                                                                                   \
    T& operator=(T&&)      = delete;


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