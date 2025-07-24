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


JsPlatform::JsPlatform() : mPlatform(v8::platform::NewDefaultPlatform()) { initPlatform(); }
JsPlatform::JsPlatform(std::unique_ptr<v8::Platform> platform) : mPlatform(std::move(platform)) { initPlatform(); }

void JsPlatform ::initPlatform() {
    v8::V8::InitializePlatform(mPlatform.get());
    v8::V8::Initialize();
}

JsPlatform::~JsPlatform() {
    std::lock_guard<std::mutex> lock(mMutex);
    v8::V8::Dispose();
    v8::V8::DisposePlatform();
}

JsRuntime* JsPlatform::newRuntime() {
    std::lock_guard<std::mutex> lock(mMutex);
    v8::Isolate::CreateParams   params;
    params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

    v8::Isolate* isolate = v8::Isolate::New(params);

    JsRuntime* runtime;
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope    handle_scope(isolate);
        runtime = new JsRuntime(isolate);
    }
    mRuntimes.emplace_back(runtime);
    return runtime;
}

void JsPlatform::removeRuntime(JsRuntime* runtime, bool destroyRuntime) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::erase_if(mRuntimes, [runtime](JsRuntime* r) { return r == runtime; });

    if (destroyRuntime) {
        runtime->destroy();
    }
}

} // namespace v8wrap