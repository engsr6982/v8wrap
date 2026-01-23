#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers.hpp"
#include "catch2/matchers/catch_matchers_exception.hpp"

#include "v8wrap/runtime/Engine.h"
#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/runtime/Exception.h"
#include "v8wrap/runtime/Platform.h"
#include "v8wrap/types/Value.h"


using Catch::Matchers::MessageMatches;

TEST_CASE("Exception") {
    auto rt = v8wrap::Platform::getInstance().newEngine();

    {
        v8wrap::EngineScope scope(rt);

        auto bol = v8wrap::Boolean::newBoolean(true);
        CHECK(bol.toString().getValue() == "true");

        REQUIRE_THROWS_AS(bol.asValue().asString(), v8wrap::Exception); // 非法的类型转换

        try {
            throw v8wrap::Exception("test exception");
        } catch (const v8wrap::Exception& e) {
            CHECK(e.message() == "test exception");
        }
    }
}