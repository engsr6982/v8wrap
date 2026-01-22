#pragma once
#include "v8wrap/bind/meta/EnumDefine.hpp"

namespace v8wrap::bind {

template <typename E>
struct EnumDefineBuilder {
    static_assert(std::is_enum_v<E>, "EnumDefineBuilder only accept enum type!");

    std::string                          name_;
    std::vector<meta::EnumDefine::Entry> entries_;

    explicit EnumDefineBuilder(std::string name) : name_(std::move(name)) {}

    EnumDefineBuilder& value(std::string name, E e) {
        entries_.emplace_back(std::move(name), static_cast<int64_t>(e));
        return *this;
    }

    [[nodiscard]] meta::EnumDefine build() { return meta::EnumDefine{std::move(name_), std::move(entries_)}; }
};

template <typename E>
inline EnumDefineBuilder<E> defineEnum(std::string name) {
    return EnumDefineBuilder<E>(std::move(name));
}

} // namespace v8wrap::bind