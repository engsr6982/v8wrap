#pragma once
#include "v8wrap/Global.h"
#include "v8wrap/Types.h"
#include <memory>
#include <mutex>
#include <vector>

V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-platform.h>
V8_WRAP_WARNING_GUARD_END


namespace v8wrap {


class Platform {
    std::unique_ptr<v8::Platform> mPlatform{nullptr};
    std::vector<Engine*>          mRuntimes{};
    mutable std::mutex            mMutex{};

    static std::unique_ptr<Platform> sInstance;
    Platform();

public:
    V8WRAP_DISALLOW_COPY(Platform);
    ~Platform();

    static void initJsPlatform();

    static Platform* getPlatform();

    void shutdownJsPlatform();

    [[nodiscard]] Engine* newRuntime();

    void addRuntime(Engine* runtime);

    void removeRuntime(Engine* runtime, bool destroyRuntime = true);

    std::vector<Engine*> getRuntimes() const;
};


} // namespace v8wrap