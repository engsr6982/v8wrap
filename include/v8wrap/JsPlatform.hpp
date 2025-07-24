#pragma once
#include "v8wrap/Global.hpp"
#include "v8wrap/Types.hpp"
#include <memory>
#include <mutex>
#include <v8-platform.h>

namespace v8wrap {


class JsPlatform {
    std::unique_ptr<v8::Platform> mPlatform;
    std::vector<JsRuntime*>       mRuntimes;
    mutable std::mutex            mMutex;

    void initPlatform();

public:
    V8WRAP_DISALLOW_COPY(JsPlatform);

    JsPlatform();                                                // default, create default platform
    explicit JsPlatform(std::unique_ptr<v8::Platform> platform); // use existing platform
    virtual ~JsPlatform();

    [[nodiscard]] JsRuntime* newRuntime();

    void removeRuntime(JsRuntime* runtime, bool destroyRuntime = true);
};


} // namespace v8wrap