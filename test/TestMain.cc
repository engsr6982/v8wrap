#include "catch2/catch_all.hpp"
#include "catch2/catch_test_macros.hpp"
#include "v8wrap/JsPlatform.hpp"
#include "v8wrap/JsRuntime.hpp"

TEST_CASE("TestMain", "[.]") {
    // auto platform = new v8wrap::JsPlatform();

    auto runtime = new v8wrap::JsRuntime(nullptr);

    REQUIRE(runtime != nullptr); // This test is just to make sure that Catch2 is working
}