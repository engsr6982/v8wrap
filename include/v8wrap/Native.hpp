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
 * 在 v8wrap 中，每个绑定到 JavaScript 的实例类都需要一个 Holder。
 * v8wrap 并不关心你如何存储或管理该资源。
 * 当绑定类被构造时，v8wrap 会将实例（void*）存储在 Holder 中。
 * 同样，当资源被销毁时，v8wrap 会删除这个 Holder。如果你需要释放资源，必须在 Holder 的析构函数中进行。
 * 为了安全起见，所有 Holder 都必须满足 Holder 的约束条件并继承自 IHolder。
 * 你无需负责初始化 IHolder 的资源；v8wrap 会在适当的时候对其进行初始化。
 * 每个 Holder 都应遵循 RAII 原则，这能最大限度地防止内存泄漏。
 * Holder 的设计理念是将构造与存储分离，以提供最大的灵活性。
 * 因此，资源的安全性由 Holder 负责，而 v8wrap 仅管理 Holder 的生命周期。
 *
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
concept OwnedHolder = std::is_base_of_v<IHolder, H> && std::is_final_v<H> && std::constructible_from<H, Ty*>
                   && HasUserDeclaredDestructor<H> && requires(H h, JsRuntime* rt) {
                          { h(rt) } -> std::same_as<Ty*>;
                      };

/**
 * 视图 Holder 是一种特殊的 Holder，它不拥有资源，而是仅持有资源。
 * 常见的使用场景为：类方法返回成员时，就需要使用视图 Holder，如果使用 OwnedHolder，则会导致 delete 非堆内存崩溃。
 * 所以，视图 Holder 的析构函数为空，并且不负责释放资源。
 * View Holder is a special Holder that does not own the resource but only holds the resource.
 * Common use cases include: when a class method returns a member, a view Holder is needed. If an OwnedHolder is used,
 *  it will cause a delete crash of non-heap memory.
 * Therefore, the destructor of the view Holder is empty and does not release the resource.
 */
template <typename H, typename Ty>
concept ViewHolder = std::is_base_of_v<IHolder, H> && std::is_final_v<H> && std::constructible_from<H, Ty*>
                  && requires(H h, JsRuntime* rt) {
                         { h(rt) } -> std::same_as<Ty*>;
                     };


template <typename T>
struct OwnedRawPtrHolder final : public IHolder {
    T* ptr;
    OwnedRawPtrHolder(T* ptr) : ptr(ptr) {}
    ~OwnedRawPtrHolder() { delete ptr; }
    T* operator()(JsRuntime*) const { return ptr; }
};

template <typename T>
struct ViewRawPtrHolder final : public IHolder {
    T* ptr;
    ViewRawPtrHolder(T* ptr) : ptr(ptr) {}
    ~ViewRawPtrHolder() {}
    T* operator()(JsRuntime*) const { return ptr; }
};

template <typename T>
struct UnsafeRawPtrHolder final : public IHolder {
    T* ptr;
    UnsafeRawPtrHolder(T* ptr) : ptr(ptr) {}
    ~UnsafeRawPtrHolder() { /* unsafe! */ }
    T* operator()(JsRuntime*) const { return ptr; }
};

template <typename T>
struct OwnedUniquePtrHolder final : public IHolder {
    std::unique_ptr<T> ptr;
    OwnedUniquePtrHolder(std::unique_ptr<T> ptr) : ptr(std::move(ptr)) {}
    ~OwnedUniquePtrHolder() { ptr.reset(); }
    T* operator()(JsRuntime*) const { return ptr.get(); }
};

template <typename T>
struct OwnedSharedPtrHolder final : public IHolder {
    std::shared_ptr<T> ptr;
    OwnedSharedPtrHolder(std::shared_ptr<T> ptr) : ptr(std::move(ptr)) {}
    ~OwnedSharedPtrHolder() { ptr.reset(); }
    T* operator()(JsRuntime*) const { return ptr.get(); }
};

template <typename T>
struct OwnedWeakPtrHolder final : public IHolder {
    std::weak_ptr<T> ptr;
    OwnedWeakPtrHolder(std::weak_ptr<T> ptr) : ptr(std::move(ptr)) {}
    ~OwnedWeakPtrHolder() { ptr.reset(); }
    T* operator()(JsRuntime*) const { return ptr.lock().get(); }
};


} // namespace v8wrap