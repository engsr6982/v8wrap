#include "v8wrap/JsPlatform.hpp"
#include <catch2/catch_session.hpp>


int main(int argc, char* argv[]) {
    v8wrap::JsPlatform::initJsPlatform();

    int result = Catch::Session().run(argc, argv);

    v8wrap::JsPlatform::getPlatform()->shutdownJsPlatform();

    return result;
}