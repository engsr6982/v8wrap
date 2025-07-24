#include "v8wrap/JsPlatform.hpp"
#include "v8-isolate.h"
#include "v8wrap/JsRuntime.hpp"
#include <libplatform/libplatform-export.h>
#include <libplatform/libplatform.h>
#include <libplatform/v8-tracing.h>
#include <memory>
#include <mutex>
#include <v8-initialization.h>
#include <v8-platform.h>
#include <vector>


namespace v8wrap {


std::unique_ptr<JsPlatform> JsPlatform::sInstance{nullptr};

void JsPlatform::initJsPlatform() {
    if (sInstance) {
        return;
    }
    sInstance = std::unique_ptr<JsPlatform>(new JsPlatform());
}
JsPlatform* JsPlatform::getPlatform() { return sInstance.get(); }

void JsPlatform::shutdownJsPlatform() { sInstance.reset(); }

JsPlatform::JsPlatform() : mPlatform(v8::platform::NewDefaultPlatform()) {
    v8::V8::InitializePlatform(mPlatform.get());
    v8::V8::Initialize();
}

JsPlatform::~JsPlatform() {
    for (auto runtime : mRuntimes) {
        runtime->destroy();
    }
    std::lock_guard<std::mutex> lock(mMutex);
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
}

JsRuntime* JsPlatform::newRuntime() {
    return new JsRuntime(); // The constructor internally adds a runtime to the JsPlatform
}

void JsPlatform::addRuntime(JsRuntime* runtime) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (std::find(mRuntimes.begin(), mRuntimes.end(), runtime) == mRuntimes.end()) {
        mRuntimes.push_back(runtime);
    }
}

void JsPlatform::removeRuntime(JsRuntime* runtime, bool destroyRuntime) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::erase_if(mRuntimes, [runtime](JsRuntime* r) { return r == runtime; });
    if (destroyRuntime) {
        runtime->destroy();
    }
}

std::vector<JsRuntime*> JsPlatform::getRuntimes() const { return mRuntimes; }

} // namespace v8wrap