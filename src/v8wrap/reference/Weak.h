#pragma once
#include "v8wrap/Types.h"
#include "v8wrap/types/internal/V8TypeAlias.h"

#include "v8-persistent-handle.h"


namespace v8wrap {


template <typename T>
class Weak final {
    static_assert(std::is_base_of_v<Value, T>, "T must be derived from Value");

public:
    V8WRAP_DISALLOW_COPY(Weak); // v8::Global is not copyable

    Weak() noexcept; // empty

    explicit Weak(Local<T> const& val);
    explicit Weak(Global<T> const& val);

    Weak(Weak<T>&& other) noexcept;
    Weak& operator=(Weak<T>&& other) noexcept;

    ~Weak();

    [[nodiscard]] Local<T> get() const;

    [[nodiscard]] Local<Value> getValue() const;

    [[nodiscard]] bool isEmpty() const;

    void reset();

private:
    using v8Global = v8::Global<internal::V8Type_v<T>>;

    Engine*  engine_ = nullptr;
    v8Global handle_;

    inline void markWeak();

    friend class Engine;

    template <typename>
    friend class Local;
    template <typename>
    friend class Weak;
};


} // namespace v8wrap