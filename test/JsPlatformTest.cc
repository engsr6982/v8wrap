#include "catch2/catch_test_macros.hpp"

#include "v8wrap/JsPlatform.hpp"
#include "v8wrap/JsRuntime.hpp"

TEST_CASE("JsPlatformTest") {
    v8wrap::JsPlatform::initJsPlatform();

    auto platform = v8wrap::JsPlatform::getPlatform();
    REQUIRE(platform != nullptr);

    auto rt  = platform->newRuntime();
    auto rt2 = platform->newRuntime();
    auto rt3 = platform->newRuntime();
    REQUIRE(rt != nullptr);
    REQUIRE(rt2 != nullptr);
    REQUIRE(rt3 != nullptr);

    REQUIRE(platform->getRuntimes().size() == 3);


    rt->destroy();
    REQUIRE(platform->getRuntimes().size() == 2);

    platform->shutdownJsPlatform();
}
