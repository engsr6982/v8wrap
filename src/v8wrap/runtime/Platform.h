#pragma once
#include "v8wrap/Global.h"
#include "v8wrap/Types.h"

#include <cstddef>
#include <memory>


V8_WRAP_WARNING_GUARD_BEGIN
#include <v8-platform.h>
V8_WRAP_WARNING_GUARD_END


namespace v8wrap {


class Platform final {
    struct Impl;
    std::unique_ptr<Impl> impl_{nullptr};

    Platform() noexcept = default;

    void ensureInitialized() const;

public:
    V8WRAP_DISALLOW_COPY(Platform);
    ~Platform();

    [[nodiscard]] static Platform& getInstance();

    void initialize();

    void shutdown();

    [[nodiscard]] Engine* newEngine();

    /**
     * @brief 添加一个引擎到平台中
     * @note 当引擎被添加到平台后，平台会负责管理引擎的生命周期，当平台被销毁时，引擎也会被销毁
     */
    [[nodiscard]] Engine* addEngine(std::unique_ptr<Engine>&& engine);

    bool destroyEngine(Engine* engine);

    size_t engineCount() const;

    /**
     * @brief 遍历平台中的所有引擎
     * @param callback 回调函数，返回false时停止遍历
     * @note callback 里禁止访问 Platform 任何 API，否则会导致死锁
     */
    void forEachEngine(std::function<bool(Engine const&)> const& callback) const;
};


} // namespace v8wrap