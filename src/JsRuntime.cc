#include <algorithm>
#include <cassert>
#include <fstream>
#include <stdexcept>
#include <utility>

#include <v8-context.h>
#include <v8-exception.h>
#include <v8-isolate.h>
#include <v8-locker.h>
#include <v8-message.h>
#include <v8-persistent-handle.h>
#include <v8-script.h>

#include "v8-value.h"
#include "v8wrap/JsException.hpp"
#include "v8wrap/JsPlatform.hpp"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsRuntime.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsValue.hpp"

namespace v8wrap {


JsRuntime::JsRuntime() : mPlatform(JsPlatform::getPlatform()) {
    v8::Isolate::CreateParams params;
    params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

    mIsolate = v8::Isolate::New(params);

    v8::Locker         locker(mIsolate);
    v8::Isolate::Scope isolate_scope(mIsolate);
    v8::HandleScope    handle_scope(mIsolate);
    mContext.Reset(mIsolate, v8::Context::New(mIsolate));
    mPlatform->addRuntime(this);
}

JsRuntime::JsRuntime(v8::Isolate* isolate, v8::Local<v8::Context> context)
: mIsolate(isolate),
  mContext(v8::Global<v8::Context>{isolate, context}),
  mIsExternalIsolate(true) {}

JsRuntime::~JsRuntime() = default;


v8::Isolate*           JsRuntime::isolate() const { return mIsolate; }
v8::Local<v8::Context> JsRuntime::context() const { return mContext.Get(mIsolate); }

void JsRuntime::setData(std::shared_ptr<void> data) { mUserData = std::move(data); }

void JsRuntime::destroy() {
    if (isDestroying()) return;
    mDestroying = true;

    if (mUserData) mUserData.reset();

    {
        JsRuntimeScope scope(this);

        for (auto& [key, value] : mManagedResources) {
            value.Reset();
            key->deleter(key->resource);
            delete key;
        }

        // TODO: implement

        mManagedResources.clear();
        mContext.Reset();
    }

    if (!mIsExternalIsolate) mIsolate->Dispose();
    if (mPlatform) mPlatform->removeRuntime(this, false);

    delete this;
}

bool JsRuntime::isDestroying() const { return mDestroying; }

Local<JsValue> JsRuntime::eval(Local<JsString> const& code) { return eval(code, JsString::newString("<eval>")); }

Local<JsValue> JsRuntime::eval(Local<JsString> const& code, Local<JsString> const& source) {
    v8::TryCatch try_catch(mIsolate);

    auto v8Code   = code.val;
    auto v8Source = source.val;
    auto ctx      = mContext.Get(mIsolate);

    auto origin = v8::ScriptOrigin(v8Source);
    auto script = v8::Script::Compile(ctx, v8Code, &origin);
    JsException::rethrow(try_catch);

    auto result = script.ToLocalChecked()->Run(ctx);
    JsException::rethrow(try_catch);
    return Local<JsValue>(result.ToLocalChecked());
}

void JsRuntime::loadFile(std::filesystem::path const& path) {
    if (isDestroying()) return;
    if (!std::filesystem::exists(path)) {
        throw JsException("File not found: " + path.string());
    }
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        throw JsException("Failed to open file: " + path.string());
    }
    std::string code((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    eval(JsString::newString(code), JsString::newString(path.string()));
}

Local<JsObject> JsRuntime::getGlobalThis() const { return Local<JsObject>(mContext.Get(mIsolate)->Global()); }

Local<JsValue> JsRuntime::getVauleFromGlobalThis(Local<JsString> const& key) const {
    auto globalThis = getGlobalThis();
    if (!globalThis.has(key)) return {};
    return globalThis.get(key);
}

void JsRuntime::setVauleToGlobalThis(Local<JsString> const& key, Local<JsValue> const& value) const {
    auto globalThis = getGlobalThis();
    globalThis.set(key, value);
}

void JsRuntime::addManagedResource(void* resource, v8::Local<v8::Value> value, std::function<void(void*)>&& deleter) {
    auto managed = std::make_unique<ManagedResource>(this, resource, std::move(deleter));

    v8::Global<v8::Value> weak{mIsolate, value};
    weak.SetWeak(
        static_cast<void*>(managed.get()),
        [](v8::WeakCallbackInfo<void> const& data) {
            auto managed = static_cast<ManagedResource*>(data.GetParameter());
            auto runtime = managed->runtime;
            {
                v8::Locker locker(runtime->mIsolate); // Since the v8 GC is not on the same thread, locking is required
                auto       iter = runtime->mManagedResources.find(managed);
                assert(iter != runtime->mManagedResources.end()); // ManagedResource should be in the map
                runtime->mManagedResources.erase(iter);

                data.SetSecondPassCallback([](v8::WeakCallbackInfo<void> const& data) {
                    auto       managed = static_cast<ManagedResource*>(data.GetParameter());
                    v8::Locker locker(managed->runtime->mIsolate);
                    managed->deleter(managed->resource);
                    delete managed;
                });
            }
        },
        v8::WeakCallbackType::kParameter
    );
    mManagedResources.emplace(managed.release(), std::move(weak));
}


} // namespace v8wrap