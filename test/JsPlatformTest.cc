#include "catch2/catch_test_macros.hpp"

#include "v8wrap/JsPlatform.hpp"
#include "v8wrap/JsRuntime.hpp"

TEST_CASE("JsPlatformTest") {
    v8wrap::JsPlatform::initJsPlatform();
    auto platform = v8wrap::JsPlatform::getPlatform();

    REQUIRE(platform != nullptr);

    SECTION("Create runtime") {
        auto rt = platform->newRuntime();
        REQUIRE(rt != nullptr);

        auto rt2 = platform->newRuntime();
        REQUIRE(rt2 != nullptr);

        auto rt3 = platform->newRuntime();
        REQUIRE(rt3 != nullptr);
    }

    REQUIRE(platform->getRuntimes().size() == 3);

    SECTION("Remove a runtime") {
        auto rt = platform->getRuntimes()[0];
        REQUIRE(rt != nullptr);

        rt->destroy();
        REQUIRE(platform->getRuntimes().size() == 2);
    }

    SECTION("Destroy platform") { platform->shutdownJsPlatform(); }
}
