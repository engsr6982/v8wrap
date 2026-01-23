#pragma once
#include "Engine.h"
#include "v8wrap/Types.h"
#include "v8wrap/bind/JsManagedResource.h"

#include "v8-object.h"


namespace v8wrap {


template <typename T>
std::shared_ptr<T> Engine::getData() const {
    return std::static_pointer_cast<T>(userData_);
}

template <typename T>
    requires concepts::StringLike<T>
Local<Value> Engine::eval(T const& str) {
    return eval(String::newString(str));
}

template <typename T>
Local<Object> Engine::newInstanceOfRaw(bind::meta::ClassDefine const& bind, T* instance) {
    auto wrap = bind::JsManagedResource::make(
        instance,
        [](void* res) -> void* { return res; }, // no-op
        [](void* res) -> void { delete static_cast<T*>(res); }
    );
    return newInstance(bind, std::move(wrap));
}

template <typename T>
Local<Object> Engine::newInstanceOfView(bind::meta::ClassDefine const& bind, T* instance) {
    auto wrap = bind::JsManagedResource::make(
        instance,
        [](void* res) -> void* { return res; }, // no-op
        [](void*) -> void {}
    );
    return newInstance(bind, std::move(wrap));
}

template <typename T>
Local<Object>
Engine::newInstanceOfView(bind::meta::ClassDefine const& bind, T* instance, Local<Object> const& ownerJs) {
    struct Control {
        v8::Global<v8::Object> ownerJsInst;
        void*                  nativeInst{nullptr};
        explicit Control(v8::Global<v8::Object>&& ownerJs, void* instance)
        : ownerJsInst(std::move(ownerJs)),
          nativeInst(instance) {}
        ~Control() { ownerJsInst.Reset(); }
    };
    auto control = new Control{
        v8::Global<v8::Object>{isolate_, ValueHelper::unwrap(ownerJs)},
        instance
    };
    auto wrap = bind::JsManagedResource::make(
        control,
        [](void* res) -> void* { return static_cast<Control*>(res)->nativeInst; },
        [](void* res) -> void { delete static_cast<Control*>(res); }
    );
    return newInstance(bind, std::move(wrap));
}

template <typename T>
Local<Object> Engine::newInstanceOfUnique(bind::meta::ClassDefine const& bind, std::unique_ptr<T>&& instance) {
    return newInstanceOfRaw<T>(bind, instance.release());
}

template <typename T>
Local<Object> Engine::newInstanceOfShared(bind::meta::ClassDefine const& bind, std::shared_ptr<T>&& instance) {
    struct Control {
        std::shared_ptr<T> instance;
        explicit Control(std::shared_ptr<T>&& instance) : instance(std::move(instance)) {}
    };
    auto control = new Control{std::move(instance)};
    auto wrap    = bind::JsManagedResource::make(
        control,
        [](void* res) -> void* { return static_cast<Control*>(res)->instance.get(); },
        [](void* res) -> void { delete static_cast<Control*>(res); }
    );
    return newInstance(bind, std::move(wrap));
}

template <typename T>
Local<Object> Engine::newInstanceOfWeak(bind::meta::ClassDefine const& bind, std::weak_ptr<T>&& instance) {
    struct Control {
        std::weak_ptr<T> instance;
        explicit Control(std::weak_ptr<T>&& instance) : instance(std::move(instance)) {}
    };
    auto control = new Control{std::move(instance)};
    auto wrap    = bind::JsManagedResource::make(
        control,
        [](void* res) -> void* { return static_cast<Control*>(res)->instance.lock().get(); },
        [](void* res) -> void { delete static_cast<Control*>(res); }
    );
    return newInstance(bind, wrap);
}

template <typename T>
T* Engine::getNativeInstanceOf(Local<Object> const& obj) const {
    return static_cast<T*>(this->getNativeInstanceOf(obj));
}


} // namespace v8wrap