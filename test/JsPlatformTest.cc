#include "catch2/catch_test_macros.hpp"

#include "v8wrap/runtime/Engine.h"
#include "v8wrap/runtime/Platform.h"



TEST_CASE("JsPlatformTest") {
    auto platform = v8wrap::Platform::getPlatform();
    REQUIRE(platform != nullptr);

    auto rt  = platform->newRuntime();
    auto rt2 = platform->newRuntime();
    REQUIRE(rt != nullptr);
    REQUIRE(rt2 != nullptr);

    REQUIRE(platform->getRuntimes().size() == 2);

    rt->destroy();
    REQUIRE(platform->getRuntimes().size() == 1);

    platform->removeRuntime(rt2);
    REQUIRE(platform->getRuntimes().size() == 0);
}
