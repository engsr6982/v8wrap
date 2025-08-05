#pragma once
#include "v8wrap/Types.hpp"
#include <concepts>
#include <memory>


namespace v8wrap {


template <typename H>
concept HasDeleter = requires(H* h, JsRuntime* rt) {
    { h->deleter(rt) } -> std::same_as<void>;
};


// When Owned is true, it means that v8wrap takes over ownership of the resource, and a deleter needs to be provided.
// v8wrap calls this deleter when the resource is destroyed.
// On the other hand, v8wrap does not take over the resource, the lifecycle of the resource is externally responsible,
// and v8wrap does not notify the resource of its release
// example:
// struct Res {};
// struct MyResHolder {
//     Res* ptr;
//     MyResHolder(Res* ptr) : ptr(ptr) {} // constructible_from is required
//     Res* operator()(JsRuntime*) const { return ptr; } // requires
//     void deleter(JsRuntime*) const { delete ptr; } // only required when Owned is true
// }
//
// The design concept of this Holder is that v8wrap is not responsible for creating and destroying.
// These methods should be provided by you (the user) v8wrap only for calls between Js <=> C++
template <typename H, typename Ty, bool Owned = true>
concept Holder = requires(H h, JsRuntime* rt) {
    { h(rt) } -> std::same_as<Ty*>;
} && (Owned ? HasDeleter<H> : !HasDeleter<H>) && std::constructible_from<H, Ty*>;


template <typename T>
struct UniquePtrHolder {
    std::unique_ptr<T> ptr;
    UniquePtrHolder(std::unique_ptr<T> ptr) : ptr(std::move(ptr)) {}

    T*   operator()(JsRuntime*) const { return ptr.get(); }
    void deleter(JsRuntime*) const { ptr.reset(); }
};

template <typename T>
struct SharedPtrHolder {
    std::shared_ptr<T> ptr;
    SharedPtrHolder(std::shared_ptr<T> ptr) : ptr(std::move(ptr)) {}

    T*   operator()(JsRuntime*) const { return ptr.get(); }
    void deleter(JsRuntime*) const { ptr.reset(); }
};

template <typename T>
struct WeakPtrHolder {
    std::weak_ptr<T> ptr;
    WeakPtrHolder(std::weak_ptr<T> ptr) : ptr(std::move(ptr)) {}

    T*   operator()(JsRuntime*) const { return ptr.lock().get(); }
    void deleter(JsRuntime*) const { ptr.reset(); }
};

template <typename T>
struct UnsafeRawPtrHolder {
    T* ptr;
    UnsafeRawPtrHolder(T* ptr) : ptr(ptr) {}
    T* operator()(JsRuntime*) const { return ptr; }
};

template <typename T>
struct RawPtrHolder {
    T* ptr;
    RawPtrHolder(T* ptr) : ptr(ptr) {}
    T*   operator()(JsRuntime*) const { return ptr; }
    void deleter(JsRuntime*) const { ptr = nullptr; }
};


} // namespace v8wrap