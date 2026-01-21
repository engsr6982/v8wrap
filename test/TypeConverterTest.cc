#include "v8wrap/TypeConverter.h"
#include "catch2/catch_test_macros.hpp"
#include "v8wrap/JsRuntime.h"
#include "v8wrap/JsRuntimeScope.h"
#include <string>


TEST_CASE("TypeConverter") {
    auto rt = new v8wrap::JsRuntime();

    v8wrap::JsRuntimeScope scope(rt);

    auto value = v8wrap::ConvertToJs("hello world");
    REQUIRE(value.isString());

    REQUIRE(v8wrap::ConvertToCpp<std::string>(value) == "hello world");


    std::string srt = "aaaa";
    auto        v2  = v8wrap::ConvertToJs(srt);
    REQUIRE(v2.isString());
    REQUIRE(v8wrap::ConvertToCpp<std::string>(v2) == srt);


    int  i  = 123;
    auto v3 = v8wrap::ConvertToJs(i);
    REQUIRE(v3.isNumber());
    REQUIRE(v8wrap::ConvertToCpp<int>(v3) == i);


    double d  = 123.456;
    auto   v4 = v8wrap::ConvertToJs(d);
    REQUIRE(v4.isNumber());
    REQUIRE(v8wrap::ConvertToCpp<double>(v4) == d);
}