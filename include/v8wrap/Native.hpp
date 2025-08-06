#pragma once
#include "v8wrap/Concepts.hpp"
#include "v8wrap/Types.hpp"
#include <concepts>
#include <memory>


namespace v8wrap {


struct IHolder {
    JsRuntime const*    mRuntime{nullptr};
    ClassBinding const* mClassBinding{nullptr};
};


/**
 * In v8wrap, every instance class bound to JavaScript requires a Holder.
 * v8wrap does not care about how you store or manage this resource.
 * When a bound class is constructed, v8wrap will store the instance (void*) in the Holder.
 * Similarly, when the resource is destructed, v8wrap will delete this Holder. If you need to release the resource, you
 *  must do so in the destructor of the Holder.
 * For safety, all Holders must satisfy the constraints of Holder and inherit from IHolder.
 * You are not responsible for initializing the resources of IHolder; v8wrap will initialize it at the appropriate time.
 * Each Holder should follow the RAII principle, which can maximize the prevention of memory leaks.
 * The design concept of Holder is to separate construction and storage, providing maximum flexibility.
 * Therefore, the safety of the resource is the responsibility of the Holder, and v8wrap only manages the life cycle of
 *  the Holder.
 */
template <typename H, typename Ty>
concept Holder = std::is_base_of_v<IHolder, H> && std::is_final_v<H> && std::constructible_from<H, Ty*>
              && HasUserDeclaredDestructor<H> && requires(H h, JsRuntime* rt) {
                     { h(rt) } -> std::same_as<Ty*>;
                 };


template <typename T>
struct RawPtrHolder final : public IHolder {
    T* ptr;
    RawPtrHolder(T* ptr) : ptr(ptr) {}
    ~RawPtrHolder() {
        delete ptr;
        ptr = nullptr;
    }
    T* operator()(JsRuntime*) const { return ptr; }
};

template <typename T>
struct UnsafeRawPtrHolder final : public IHolder {
    T* ptr;
    UnsafeRawPtrHolder(T* ptr) : ptr(ptr) {}
    T* operator()(JsRuntime*) const { return ptr; }
};

template <typename T>
struct UniquePtrHolder final : public IHolder {
    std::unique_ptr<T> ptr;
    UniquePtrHolder(std::unique_ptr<T> ptr) : ptr(std::move(ptr)) {}
    ~UniquePtrHolder() { ptr.reset(); }
    T* operator()(JsRuntime*) const { return ptr.get(); }
};

template <typename T>
struct SharedPtrHolder final : public IHolder {
    std::shared_ptr<T> ptr;
    SharedPtrHolder(std::shared_ptr<T> ptr) : ptr(std::move(ptr)) {}
    ~SharedPtrHolder() { ptr.reset(); }
    T* operator()(JsRuntime*) const { return ptr.get(); }
};

template <typename T>
struct WeakPtrHolder final : public IHolder {
    std::weak_ptr<T> ptr;
    WeakPtrHolder(std::weak_ptr<T> ptr) : ptr(std::move(ptr)) {}
    ~WeakPtrHolder() { ptr.reset(); }
    T* operator()(JsRuntime*) const { return ptr.lock().get(); }
};


} // namespace v8wrap