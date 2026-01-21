#include "v8wrap/runtime/Platform.h"
#include "v8wrap/runtime/Engine.h"
#include <memory>
#include <mutex>
#include <vector>

V8_WRAP_WARNING_GUARD_BEGIN
#include <libplatform/libplatform-export.h>
#include <libplatform/libplatform.h>
#include <libplatform/v8-tracing.h>
#include <v8-initialization.h>
#include <v8-platform.h>
V8_WRAP_WARNING_GUARD_END

namespace v8wrap {


std::unique_ptr<Platform> Platform::sInstance{nullptr};

void Platform::initJsPlatform() {
    if (sInstance) {
        return;
    }
    sInstance = std::unique_ptr<Platform>(new Platform());
}
Platform* Platform::getPlatform() { return sInstance.get(); }

void Platform::shutdownJsPlatform() { sInstance.reset(); }

Platform::Platform() : mPlatform(v8::platform::NewDefaultPlatform()) {
    v8::V8::InitializePlatform(mPlatform.get());
    v8::V8::Initialize();
}

Platform::~Platform() {
    for (auto runtime : mRuntimes) {
        runtime->destroy();
    }
    std::lock_guard<std::mutex> lock(mMutex);
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
}

Engine* Platform::newRuntime() {
    return new Engine(); // The constructor internally adds a runtime to the Platform
}

void Platform::addRuntime(Engine* runtime) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (std::find(mRuntimes.begin(), mRuntimes.end(), runtime) == mRuntimes.end()) {
        mRuntimes.push_back(runtime);
    }
}

void Platform::removeRuntime(Engine* runtime, bool destroyRuntime) {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        std::erase_if(mRuntimes, [runtime](Engine* r) { return r == runtime; });
    }
    if (destroyRuntime) runtime->destroy();
}

std::vector<Engine*> Platform::getRuntimes() const { return mRuntimes; }

} // namespace v8wrap