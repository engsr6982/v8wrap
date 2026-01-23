#include "v8wrap/runtime/Platform.h"
#include "v8wrap/runtime/Engine.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>


V8_WRAP_WARNING_GUARD_BEGIN
#include <libplatform/libplatform-export.h>
#include <libplatform/libplatform.h>
#include <libplatform/v8-tracing.h>
#include <v8-initialization.h>
#include <v8-platform.h>
V8_WRAP_WARNING_GUARD_END

namespace v8wrap {


struct Platform::Impl {
    using EnginePtr       = std::unique_ptr<Engine>;
    using EnginePtrVector = std::vector<EnginePtr>;

    std::unique_ptr<v8::Platform> v8Platform_{nullptr};
    EnginePtrVector               engines_{};
    mutable std::mutex            mutex_{};

    // 全局只能有一个 v8 平台
    inline static std::atomic_bool isInitialized_{false};

    explicit Impl() {
        bool expected = false;
        if (!isInitialized_.compare_exchange_strong(expected, true)) {
            throw std::logic_error("v8 platform has been initialized");
        }
        isInitialized_ = true;
        v8Platform_    = v8::platform::NewDefaultPlatform();
        v8::V8::InitializePlatform(v8Platform_.get());
        v8::V8::Initialize();
    }
    ~Impl() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            engines_.clear();
        }
        v8::V8::Dispose();
        v8::V8::DisposePlatform();
        isInitialized_.store(false, std::memory_order_release);
    }

    std::pair<Engine*, bool> addEngine(EnginePtr engine) {
        std::lock_guard<std::mutex> lock(mutex_);

        Engine* raw = engine.get();
        engines_.push_back(std::move(engine));
        return {raw, true};
    }

    bool destroyEngine(Engine* engine) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto iter = std::find_if(engines_.begin(), engines_.end(), [engine](auto& e) { return e.get() == engine; });
        if (iter == engines_.end()) {
            return false;
        }
        engines_.erase(iter);
        return true;
    }
};

Platform::~Platform() { shutdown(); }

Platform& Platform::getInstance() {
    static Platform instance;
    return instance;
}

void Platform::initialize() {
    if (impl_) {
        return;
    }
    impl_ = std::make_unique<Impl>();
}

void Platform::shutdown() {
    if (!impl_) {
        return;
    }
    impl_.reset();
}

void Platform::ensureInitialized() const {
    if (!impl_) {
        throw std::logic_error("v8 platform has not been initialized");
    }
}

Engine* Platform::newEngine() {
    ensureInitialized();
    auto engine         = std::make_unique<Engine>();
    auto [ptr, success] = impl_->addEngine(std::move(engine));
    return ptr;
}
Engine* Platform::addEngine(std::unique_ptr<Engine>&& engine) {
    ensureInitialized();
    auto [ptr, success] = impl_->addEngine(std::move(engine));
    return ptr;
}
bool Platform::destroyEngine(Engine* engine) {
    ensureInitialized();
    return impl_->destroyEngine(engine);
}

size_t Platform::engineCount() const {
    ensureInitialized();
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->engines_.size();
}

void Platform::forEachEngine(std::function<bool(Engine const&)> const& callback) const {
    ensureInitialized();
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    for (auto& engine : impl_->engines_) {
        if (!callback(*engine)) break;
    }
}


} // namespace v8wrap