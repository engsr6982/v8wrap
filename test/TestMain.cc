#include "v8wrap/runtime/Platform.h"
#include <catch2/catch_session.hpp>


int main(int argc, char* argv[]) {
    v8wrap::Platform::initJsPlatform();

    int result = Catch::Session().run(argc, argv);

    v8wrap::Platform::getPlatform()->shutdownJsPlatform();

    return result;
}