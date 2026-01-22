#pragma once
#include "MemberDefine.h"
#include "v8wrap/bind/JsManagedResource.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace v8wrap::bind::meta {


class ClassDefine {
public:
    std::string const          name_;
    StaticMemberDefine const   staticMemberDef_;
    InstanceMemberDefine const instanceMemberDef_;
    ClassDefine const*         base_{nullptr};
    // TODO: use RTTI typeId
    // reflection::TypeId const   typeId_;

    [[nodiscard]] inline bool hasConstructor() const { return instanceMemberDef_.constructor_ != nullptr; }

    // 由于采用 void* 提升了运行时的灵活性，但缺少了类型信息。
    // delete void* 是不安全的，因此需要此辅助回调生成合理的 finalizer。
    // 但是因为 finalizer 是和资源相关联的，故提供一个工厂方法创建托管资源并设置 getter & finalizer
    // 此回调仅在 JavaScript 使用 `new` 构造时调用，用于包装 InstanceConstructor 返回的实例 (T*)
    // 注意：当 defineClass<T> 时如果设置构造为 Disabled，那么 factory 为空
    using ManagedResourceFactory = std::unique_ptr<struct JsManagedResource> (*)(void* instance);
    ManagedResourceFactory const factory_{nullptr};

    [[nodiscard]] inline auto manage(void* instance) const {
        if (!factory_) [[unlikely]] {
            throw std::logic_error(
                "ClassDefine::manage called but factory_ is null — class is not constructible from JS"
            );
        }
        return factory_(instance);
    }

    explicit ClassDefine(
        std::string          name,
        StaticMemberDefine   staticDef,
        InstanceMemberDefine instanceDef,
        ClassDefine const*   base,
        // reflection::TypeId     typeId,
        ManagedResourceFactory factory
    )
    : name_(std::move(name)),
      staticMemberDef_(std::move(staticDef)),
      instanceMemberDef_(std::move(instanceDef)),
      base_(base),
      //   typeId_(std::move(typeId)),
      factory_(factory) {}
};


} // namespace v8wrap::bind::meta