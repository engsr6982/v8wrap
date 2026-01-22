#pragma once
#include "v8wrap/Types.h"
#include "v8wrap/types/internal/V8TypeAlias.h"

#include "v8-persistent-handle.h"

namespace v8wrap {


template <typename T>
class Global final {
    static_assert(std::is_base_of_v<Value, T>, "T must be derived from Value");

public:
    V8WRAP_DISALLOW_COPY(Global); // v8::Global is not copyable

    Global() noexcept; // empty

    explicit Global(Local<T> const& val);
    explicit Global(Weak<T> const& val);

    Global(Global<T>&& other) noexcept;
    Global& operator=(Global<T>&& other) noexcept;

    ~Global();

    [[nodiscard]] Local<T> get() const;

    [[nodiscard]] Local<Value> getValue() const;

    [[nodiscard]] bool isEmpty() const;

    void reset();

private:
    using v8Global = v8::Global<internal::V8Type_v<T>>;

    Engine*  engine_;
    v8Global handle_;

    friend class Engine;

    template <typename>
    friend class Local;
    template <typename>
    friend class Weak;
};


} // namespace v8wrap