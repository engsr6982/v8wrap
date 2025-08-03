#pragma once
#include "v8wrap/Global.hpp"
#include "v8wrap/Types.hpp"
#include <memory>
#include <optional>
#include <type_traits>


namespace v8wrap {


template <typename Ty, typename Holder>
concept CustomHolderConcept =
    std::is_move_constructible_v<Holder> && std::is_move_assignable_v<Holder> && requires(Holder h, JsRuntime* rt) {
        { h(rt) } -> std::same_as<Ty*>;
    };


template <typename T, typename CustomHolder = void>
class ResourceHolder {
    static_assert(
        std::is_void_v<CustomHolder> || CustomHolderConcept<T, CustomHolder>,
        "CustomHolder must be void or satisfy CustomHolderConcept."
    );

public:
    enum class Type {
        Value,      // C++ value
        Reference,  // C++ reference (dangerous!)
        RawPointer, // Raw pointer (external ownership)
        UniquePtr,  // std::unique_ptr
        SharedPtr,  // std::shared_ptr
        WeakPtr,    // std::weak_ptr
        Custom      // Custom type (return T*)
    };

    [[nodiscard]] inline T* get() const {
        switch (mType) {
        case Type::Value:
            return &mValue.value();
        case Type::Reference:
        case Type::RawPointer:
            return mRawPointer;
        case Type::UniquePtr:
            return mUniquePtr.get();
        case Type::SharedPtr:
            return mSharedPtr.get();
        case Type::WeakPtr:
            return mWeakPtr.lock().get();
        case Type::Custom:
            return mCustom();
        }
    }

    [[nodiscard]] inline bool isValid() const {
        // Because value holding is always safe, but references are not guaranteed to be secure,
        // and other types are valid as long as they are not null pointers
        switch (mType) {
        case Type::Value:
            return true;
        case Type::Reference:
            return false; // External guarantee of validity
        case Type::RawPointer:
        case Type::UniquePtr:
        case Type::SharedPtr:
        case Type::WeakPtr:
        case Type::Custom:
            return get() != nullptr;
        }
    }

    [[nodiscard]] inline Type type() const { return mType; }

    explicit ResourceHolder(T&& value) : mType(Type::Value), mValue(std::move(value)) {}
    explicit ResourceHolder(T& value) : mType(Type::Reference), mRawPointer(&value) {
        struct [[deprecated("Using references as bindings is dangerous because binding wrappers don't know if "
                            "references are always valid, which can lead to dangling references!")]] Warning {};
        Warning unused{};
    }
    explicit ResourceHolder(T* ptr) : mType(Type::RawPointer), mRawPointer(ptr) {}
    explicit ResourceHolder(std::unique_ptr<T> ptr) : mType(Type::UniquePtr), mUniquePtr(std::move(ptr)) {}
    explicit ResourceHolder(std::shared_ptr<T> ptr) : mType(Type::SharedPtr), mSharedPtr(std::move(ptr)) {}
    explicit ResourceHolder(std::weak_ptr<T> ptr) : mType(Type::WeakPtr), mWeakPtr(std::move(ptr)) {}

    template <typename U = CustomHolder>
    explicit ResourceHolder(U&& holder)
        requires(!std::is_void_v<U> && CustomHolderConcept<T, U>)
    : mType(Type::Custom),
      mCustom(std::forward<U>(holder)) {}

    V8WRAP_DISALLOW_COPY(ResourceHolder);
    ResourceHolder(ResourceHolder&& other) noexcept : mType(other.mType) { handleMove(other); }
    ResourceHolder& operator=(ResourceHolder&& other) noexcept {
        if (this != &other) {
            mType = other.mType;
            handleMove(other);
        }
        return *this;
    }

private:
    void handleMove(ResourceHolder& other) {
        switch (mType) {
        case Type::Value:
            mValue = std::move(other.mValue);
            break;
        case Type::Reference:
        case Type::RawPointer:
            mRawPointer = other.mRawPointer;
            break;
        case Type::UniquePtr:
            mUniquePtr = std::move(other.mUniquePtr);
            break;
        case Type::SharedPtr:
            mSharedPtr = std::move(other.mSharedPtr);
            break;
        case Type::WeakPtr:
            mWeakPtr = std::move(other.mWeakPtr);
            break;
        case Type::Custom:
            mCustom = std::move(other.mCustom);
            break;
        }
    }

    Type const mType;

    std::optional<T> mValue{std::nullopt};
    T* mRawPointer{nullptr}; // Because the reference must be initialized, the address is stored in the original pointer
    std::unique_ptr<T> mUniquePtr{nullptr};
    std::shared_ptr<T> mSharedPtr{nullptr};
    std::weak_ptr<T>   mWeakPtr{nullptr};

    std::conditional_t<!std::is_void_v<CustomHolder>, CustomHolder, std::monostate> mCustom{};
};


} // namespace v8wrap