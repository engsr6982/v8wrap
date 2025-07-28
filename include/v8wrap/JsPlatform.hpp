#pragma once
#include "v8wrap/Global.hpp"
#include "v8wrap/Types.hpp"
#include <memory>
#include <mutex>
#include <vector>

V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-platform.h>
V8_WRAP_WARNING_GUARD_END


namespace v8wrap {


class JsPlatform {
    std::unique_ptr<v8::Platform> mPlatform{nullptr};
    std::vector<JsRuntime*>       mRuntimes{};
    mutable std::mutex            mMutex{};

    static std::unique_ptr<JsPlatform> sInstance;
    JsPlatform();

public:
    V8WRAP_DISALLOW_COPY(JsPlatform);
    ~JsPlatform();

    static void initJsPlatform();

    static JsPlatform* getPlatform();

    void shutdownJsPlatform();

    [[nodiscard]] JsRuntime* newRuntime();

    void addRuntime(JsRuntime* runtime);

    void removeRuntime(JsRuntime* runtime, bool destroyRuntime = true);

    std::vector<JsRuntime*> getRuntimes() const;
};


} // namespace v8wrap