#include "v8wrap/JsException.hpp"
#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers.hpp"
#include "catch2/matchers/catch_matchers_exception.hpp"
#include "v8wrap/JsPlatform.hpp"
#include "v8wrap/JsRuntime.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsValue.hpp"


using Catch::Matchers::MessageMatches;

TEST_CASE("JsException") {
    auto rt = new v8wrap::JsRuntime();

    {
        v8wrap::JsRuntimeScope scope(rt);

        auto bol = v8wrap::JsBoolean::newBoolean(true);
        CHECK(bol.toString().getValue() == "true");

        REQUIRE_THROWS_AS(bol.asValue().asString(), v8wrap::JsException); // 非法的类型转换

        try {
            throw v8wrap::JsException("test exception");
        } catch (const v8wrap::JsException& e) {
            CHECK(e.message() == "test exception");
        }
    }

    rt->destroy();
}