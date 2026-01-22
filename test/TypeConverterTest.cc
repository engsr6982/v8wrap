#include "catch2/catch_test_macros.hpp"

#include <string>

#include "v8wrap/bind/TypeConverter.h"
#include "v8wrap/runtime/Engine.h"
#include "v8wrap/runtime/EngineScope.h"


TEST_CASE("TypeConverter") {
    auto rt = new v8wrap::Engine();

    v8wrap::EngineScope scope(rt);

    auto value = v8wrap::bind::ConvertToJs("hello world");
    REQUIRE(value.isString());
    REQUIRE(v8wrap::bind::ConvertToCpp<std::string>(value) == "hello world");


    std::string srt = "aaaa";
    auto        v2  = v8wrap::bind::ConvertToJs(srt);
    REQUIRE(v2.isString());
    REQUIRE(v8wrap::bind::ConvertToCpp<std::string>(v2) == srt);


    int  i  = 123;
    auto v3 = v8wrap::bind::ConvertToJs(i);
    REQUIRE(v3.isNumber());
    REQUIRE(v8wrap::bind::ConvertToCpp<int>(v3) == i);


    double d  = 123.456;
    auto   v4 = v8wrap::bind::ConvertToJs(d);
    REQUIRE(v4.isNumber());
    REQUIRE(v8wrap::bind::ConvertToCpp<double>(v4) == d);
}