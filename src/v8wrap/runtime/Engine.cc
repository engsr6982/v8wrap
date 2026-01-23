#include "v8wrap/runtime/Engine.h"
#include "v8wrap/bind/JsManagedResource.h"
#include "v8wrap/bind/meta/ClassDefine.h"
#include "v8wrap/bind/meta/EnumDefine.h"
#include "v8wrap/bind/meta/MemberDefine.h"
#include "v8wrap/reference/Local.h"
#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/runtime/Exception.h"
#include "v8wrap/runtime/Platform.h"
#include "v8wrap/types/Value.h"

#include <cassert>
#include <fstream>
#include <optional>
#include <utility>
#include <vector>


V8_WRAP_WARNING_GUARD_BEGIN
#include "v8-external.h"
#include "v8-function-callback.h"
#include "v8-local-handle.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-template.h"
#include <v8-context.h>
#include <v8-exception.h>
#include <v8-isolate.h>
#include <v8-locker.h>
#include <v8-message.h>
#include <v8-persistent-handle.h>
#include <v8-script.h>
#include <v8-value.h>
V8_WRAP_WARNING_GUARD_END

namespace v8wrap {


Engine::Engine() {
    v8::Isolate::CreateParams params;
    params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

    isolate_ = v8::Isolate::New(params);

    v8::Locker         locker(isolate_);
    v8::Isolate::Scope isolate_scope(isolate_);
    v8::HandleScope    handle_scope(isolate_);
    context_.Reset(isolate_, v8::Context::New(isolate_));

    constructorSymbol_ = v8::Global<v8::Symbol>(isolate_, v8::Symbol::New(isolate_));
}

Engine::Engine(v8::Isolate* isolate, v8::Local<v8::Context> context)
: isolate_(isolate),
  context_(v8::Global<v8::Context>{isolate, context}),
  isExternalIsolate_(true) {
    constructorSymbol_ = v8::Global<v8::Symbol>(isolate_, v8::Symbol::New(isolate_));
}

Engine::~Engine() {
    if (isDestroying()) return;
    isDestroying_ = true;

    if (userData_) userData_.reset();

    {
        EngineScope scope(this);

        for (auto& [key, value] : managedResources_) {
            value.Reset();
            key->deleter(key->resource);
            delete key;
        }
        for (auto& [_, ctor] : classConstructors_) {
            ctor.Reset();
        }

        constructorSymbol_.Reset();
        classConstructors_.clear();
        registeredClasses_.clear();
        managedResources_.clear();
        context_.Reset();
    }

    if (!isExternalIsolate_) isolate_->Dispose();
}


v8::Isolate*           Engine::isolate() const { return isolate_; }
v8::Local<v8::Context> Engine::context() const { return context_.Get(isolate_); }

void Engine::setData(std::shared_ptr<void> data) { userData_ = std::move(data); }

bool Engine::isDestroying() const { return isDestroying_; }

Local<Value> Engine::eval(Local<String> const& code) { return eval(code, String::newString("<eval>")); }

Local<Value> Engine::eval(Local<String> const& code, Local<String> const& source) {
    v8::TryCatch try_catch(isolate_);

    auto v8Code   = ValueHelper::unwrap(code);
    auto v8Source = ValueHelper::unwrap(source);
    auto ctx      = context_.Get(isolate_);

    auto origin = v8::ScriptOrigin(v8Source);
    auto script = v8::Script::Compile(ctx, v8Code, &origin);
    Exception::rethrow(try_catch);

    auto result = script.ToLocalChecked()->Run(ctx);
    Exception::rethrow(try_catch);
    return ValueHelper::wrap<Value>(result.ToLocalChecked());
}

void Engine::loadFile(std::filesystem::path const& path) {
    if (isDestroying()) return;
    if (!std::filesystem::exists(path)) {
        throw Exception("File not found: " + path.string());
    }
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        throw Exception("Failed to open file: " + path.string());
    }
    std::string code((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    eval(String::newString(code), String::newString(path.string()));
}

Local<Object> Engine::getGlobalThis() const { return ValueHelper::wrap<Object>(context_.Get(isolate_)->Global()); }

Local<Value> Engine::getVauleFromGlobalThis(Local<String> const& key) const {
    auto globalThis = getGlobalThis();
    if (!globalThis.has(key)) return {};
    return globalThis.get(key);
}

void Engine::setVauleToGlobalThis(Local<String> const& key, Local<Value> const& value) const {
    auto globalThis = getGlobalThis();
    globalThis.set(key, value);
}

void Engine::addManagedResource(void* resource, v8::Local<v8::Value> value, std::function<void(void*)>&& deleter) {
    auto managed = std::make_unique<ManagedResource>(this, resource, std::move(deleter));

    v8::Global<v8::Value> weak{isolate_, value};
    weak.SetWeak(
        static_cast<void*>(managed.get()),
        [](v8::WeakCallbackInfo<void> const& data) {
            auto managed = static_cast<ManagedResource*>(data.GetParameter());
            auto runtime = managed->runtime;
            {
                v8::Locker locker(runtime->isolate_); // Since the v8 GC is not on the same thread, locking is required
                auto       iter = runtime->managedResources_.find(managed);
                assert(iter != runtime->managedResources_.end()); // ManagedResource should be in the map
                runtime->managedResources_.erase(iter);

                data.SetSecondPassCallback([](v8::WeakCallbackInfo<void> const& data) {
                    auto       managed = static_cast<ManagedResource*>(data.GetParameter());
                    v8::Locker locker(managed->runtime->isolate_);
                    delete managed;
                });
            }
        },
        v8::WeakCallbackType::kParameter
    );
    managedResources_.emplace(managed.release(), std::move(weak));
}

void Engine::registerClass(bind::meta::ClassDefine const& binding) {
    if (registeredClasses_.contains(binding.name_)) {
        throw Exception("Class binding already registered: " + binding.name_);
    }

    v8::TryCatch vtry(isolate_);

    v8::Local<v8::FunctionTemplate> ctor; // js: new T()
    if (binding.hasConstructor()) {
        ctor = createInstanceClassCtor(binding);
    } else {
        ctor = v8::FunctionTemplate::New(
            isolate_,
            nullptr,
            {},
            {},
            0,
            v8::ConstructorBehavior::kThrow // Static classes have no constructors
        );
        ctor->RemovePrototype();
    }

    auto scriptClassName = String::newString(binding.name_);
    ctor->SetClassName(ValueHelper::unwrap(scriptClassName));

    if (binding.base_ != nullptr) {
        if (!binding.base_->hasConstructor()) {
            throw Exception{
                binding.name_ + " cannot inherit from " + binding.base_->name_
                + " because it is a static class without a prototype."
            };
        }
        auto iter = classConstructors_.find(binding.base_);
        if (iter == classConstructors_.end()) {
            throw Exception{
                binding.name_ + " cannot inherit from " + binding.base_->name_
                + " because the parent class is not registered."
            };
        }
        auto parentCtor = iter->second.Get(isolate_);
        ctor->Inherit(parentCtor);
    }

    implStaticRegister(ctor, binding.staticMemberDef_);
    implInstanceRegister(ctor, binding.instanceMemberDef_);

    auto function = ctor->GetFunction(context_.Get(isolate_));
    Exception::rethrow(vtry);

    registeredClasses_.emplace(binding.name_, &binding);
    classConstructors_.emplace(&binding, v8::Global<v8::FunctionTemplate>{isolate_, ctor});

    // TODO: update toStringTag

    setVauleToGlobalThis(scriptClassName, ValueHelper::wrap<Function>(function.ToLocalChecked()));
}


void Engine::implStaticRegister(
    v8::Local<v8::FunctionTemplate>&      ctor,
    bind::meta::StaticMemberDefine const& staticBinding
) {
    for (auto& property : staticBinding.property_) {
        auto scriptPropertyName = String::newString(property.name_);

        auto v8Getter = [](v8::Local<v8::Name>, v8::PropertyCallbackInfo<v8::Value> const& info) {
            auto pbin = static_cast<bind::meta::StaticMemberDefine::Property*>(info.Data().As<v8::External>()->Value());
            try {
                auto ret = pbin->getter_();
                info.GetReturnValue().Set(ValueHelper::unwrap(ret));
            } catch (Exception const& e) {
                e.rethrowToRuntime();
            }
        };

        v8::AccessorNameSetterCallback v8Setter = nullptr;
        if (property.setter_) {
            v8Setter = [](v8::Local<v8::Name>, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info) {
                auto pbin =
                    static_cast<bind::meta::StaticMemberDefine::Property*>(info.Data().As<v8::External>()->Value());
                try {
                    pbin->setter_(ValueHelper::wrap<Value>(value));
                } catch (Exception const& e) {
                    e.rethrowToRuntime();
                }
            };
        } else {
            v8Setter = [](v8::Local<v8::Name>, v8::Local<v8::Value>, v8::PropertyCallbackInfo<void> const&) {
                Exception(
                    "Native property have only one getter, and you cannot modify native property without "
                    "getters",
                    Exception::Type::TypeError
                )
                    .rethrowToRuntime();
            };
        }

        ctor->SetNativeDataProperty(
            ValueHelper::unwrap(scriptPropertyName).As<v8::Name>(),
            std::move(v8Getter),
            std::move(v8Setter),
            v8::External::New(isolate_, const_cast<bind::meta::StaticMemberDefine::Property*>(&property)),
            v8::PropertyAttribute::DontDelete
        );
    }
    for (auto& function : staticBinding.functions_) {
        auto scriptFunctionName = String::newString(function.name_);

        auto fn = v8::FunctionTemplate::New(
            isolate_,
            [](v8::FunctionCallbackInfo<v8::Value> const& info) {
                auto fbin =
                    static_cast<bind::meta::StaticMemberDefine::Function*>(info.Data().As<v8::External>()->Value());

                try {
                    auto ret = (fbin->callback_)(Arguments{EngineScope::currentRuntime(), info});
                    info.GetReturnValue().Set(ValueHelper::unwrap(ret));
                } catch (Exception const& e) {
                    e.rethrowToRuntime();
                }
            },
            v8::External::New(isolate_, const_cast<bind::meta::StaticMemberDefine::Function*>(&function)),
            {},
            0,
            v8::ConstructorBehavior::kThrow
        );
        ctor->Set(ValueHelper::unwrap(scriptFunctionName).As<v8::Name>(), fn, v8::PropertyAttribute::DontDelete);
    }
}

constexpr int kCtorExternal_ClassBinding = 0;
constexpr int kCtorExternal_JsRuntime    = 1;

v8::Local<v8::FunctionTemplate> Engine::createInstanceClassCtor(bind::meta::ClassDefine const& binding) {
    v8::TryCatch vtry{isolate_};

    v8::Local<v8::Object> data = v8::Object::New(isolate_);
    Exception::rethrow(vtry);

    auto ctx = context_.Get(isolate_);

    (void)data->Set(
        ctx,
        kCtorExternal_ClassBinding,
        v8::External::New(isolate_, const_cast<bind::meta::ClassDefine*>(&binding))
    );
    Exception::rethrow(vtry);

    (void)data->Set(ctx, kCtorExternal_JsRuntime, v8::External::New(isolate_, this));
    Exception::rethrow(vtry);

    auto ctor = v8::FunctionTemplate::New(
        isolate_,
        [](v8::FunctionCallbackInfo<v8::Value> const& info) {
            auto ctx  = info.GetIsolate()->GetCurrentContext();
            auto data = info.Data().As<v8::Object>();

            auto binding = static_cast<bind::meta::ClassDefine*>(
                data->Get(ctx, kCtorExternal_ClassBinding).ToLocalChecked().As<v8::External>()->Value()
            );
            auto runtime = static_cast<Engine*>(
                data->Get(ctx, kCtorExternal_JsRuntime).ToLocalChecked().As<v8::External>()->Value()
            );

            auto& ctor = binding->instanceMemberDef_.constructor_;

            try {
                if (!info.IsConstructCall()) {
                    throw Exception{"Native class constructor cannot be called as a function"};
                }

                void* instance        = nullptr;
                bool  constructFromJs = true;
                if (info.Length() == 2 && info[0]->IsSymbol()
                    && info[0]->StrictEquals(runtime->constructorSymbol_.Get(runtime->isolate_))
                    && info[1]->IsExternal()) {
                    // constructor call from native code
                    instance        = info[1].As<v8::External>()->Value();
                    constructFromJs = false;
                } else {
                    // constructor call from JS code
                    instance = ctor(Arguments{runtime, info});
                }

                if (instance == nullptr) {
                    if (constructFromJs) {
                        throw Exception{"This native class cannot be constructed."};
                    } else {
                        throw Exception{"This native class cannot be constructed from native code."};
                    }
                }

                void* wrapped = constructFromJs ? binding->manage(instance).release() : instance;
                {
                    auto typed     = static_cast<bind::JsManagedResource*>(wrapped);
                    typed->define_ = const_cast<bind::meta::ClassDefine*>(binding);
                    typed->engine_ = runtime;

                    (*const_cast<bool*>(&typed->constructFromJs_)) = constructFromJs;
                }
                info.This()->SetAlignedPointerInInternalField(kInternalField_WrappedResource, wrapped);

                if (constructFromJs) {
                    runtime->isolate_->AdjustAmountOfExternalAllocatedMemory(
                        static_cast<int64_t>(binding->instanceMemberDef_.classSize_)
                    );
                }

                runtime->addManagedResource(wrapped, info.This(), [](void* wrapped) {
                    auto typed = static_cast<bind::JsManagedResource*>(wrapped);

                    if (typed->constructFromJs_) {
                        typed->engine_->isolate_->AdjustAmountOfExternalAllocatedMemory(
                            -static_cast<int64_t>(typed->define_->instanceMemberDef_.classSize_)
                        );
                    }
                    delete typed;
                });
            } catch (Exception const& e) {
                e.rethrowToRuntime();
            }
        },
        data
    );
    ctor->InstanceTemplate()->SetInternalFieldCount(kInternalFieldCount);
    return ctor;
}

void Engine::implInstanceRegister(
    v8::Local<v8::FunctionTemplate>&        ctor,
    bind::meta::InstanceMemberDefine const& instanceBinding
) {
    // TODO: mount "$equals"
    auto prototype = ctor->PrototypeTemplate();
    auto signature = v8::Signature::New(isolate_);

    for (auto& method : instanceBinding.methods_) {
        auto scriptMethodName = String::newString(method.name_);

        auto fn = v8::FunctionTemplate::New(
            isolate_,
            [](v8::FunctionCallbackInfo<v8::Value> const& info) {
                auto method =
                    static_cast<bind::meta::InstanceMemberDefine::Method*>(info.Data().As<v8::External>()->Value());
                auto wrapped = info.This()->GetAlignedPointerFromInternalField(kInternalField_WrappedResource);

                auto typed   = static_cast<bind::JsManagedResource*>(wrapped);
                auto runtime = const_cast<Engine*>(typed->engine_);
                auto thiz    = (*typed)(); // operator()()
                if (thiz == nullptr) {
                    info.GetReturnValue().SetNull(); // object has been destroyed
                    return;
                }
                try {
                    auto val = (method->callback_)(thiz, Arguments{runtime, info});
                    info.GetReturnValue().Set(ValueHelper::unwrap(val));
                } catch (Exception const& e) {
                    e.rethrowToRuntime();
                }
            },
            v8::External::New(isolate_, const_cast<bind::meta::InstanceMemberDefine::Method*>(&method)),
            signature
        );
        prototype->Set(ValueHelper::unwrap(scriptMethodName), fn, v8::PropertyAttribute::DontDelete);
    }

    for (auto& prop : instanceBinding.property_) {
        auto scriptPropertyName = String::newString(prop.name_);
        auto data = v8::External::New(isolate_, const_cast<bind::meta::InstanceMemberDefine::Property*>(&prop));
        v8::Local<v8::FunctionTemplate> v8Getter;
        v8::Local<v8::FunctionTemplate> v8Setter;

        v8Getter = v8::FunctionTemplate::New(
            isolate_,
            [](v8::FunctionCallbackInfo<v8::Value> const& info) {
                auto prop =
                    static_cast<bind::meta::InstanceMemberDefine::Property*>(info.Data().As<v8::External>()->Value());
                auto wrapped = info.This()->GetAlignedPointerFromInternalField(kInternalField_WrappedResource);

                auto typed   = static_cast<bind::JsManagedResource*>(wrapped);
                auto runtime = const_cast<Engine*>(typed->engine_);
                auto thiz    = (*typed)(); // operator()()
                if (thiz == nullptr) {
                    info.GetReturnValue().SetNull(); // object has been destroyed
                    return;
                }
                try {
                    auto val = (prop->getter_)(thiz, Arguments{runtime, info});
                    info.GetReturnValue().Set(ValueHelper::unwrap(val));
                } catch (Exception const& e) {
                    e.rethrowToRuntime();
                }
            },
            data,
            signature
        );

        if (prop.setter_) {
            v8Setter = v8::FunctionTemplate::New(
                isolate_,
                [](v8::FunctionCallbackInfo<v8::Value> const& info) {
                    auto prop = static_cast<bind::meta::InstanceMemberDefine::Property*>(
                        info.Data().As<v8::External>()->Value()
                    );
                    auto wrapped = info.This()->GetAlignedPointerFromInternalField(kInternalField_WrappedResource);

                    auto typed   = static_cast<bind::JsManagedResource*>(wrapped);
                    auto runtime = const_cast<Engine*>(typed->engine_);
                    auto thiz    = (*typed)(); // operator()()
                    if (thiz == nullptr) {
                        info.GetReturnValue().SetNull(); // object has been destroyed
                        return;
                    }
                    try {
                        (prop->setter_)(thiz, Arguments{runtime, info});
                    } catch (Exception const& e) {
                        e.rethrowToRuntime();
                    }
                },
                data,
                signature
            );
        }

        prototype->SetAccessorProperty(
            ValueHelper::unwrap(scriptPropertyName).As<v8::Name>(),
            v8Getter,
            v8Setter,
            v8::PropertyAttribute::DontDelete
        );
    }
}

Local<Object>
Engine::newInstance(bind::meta::ClassDefine const& bind, std::unique_ptr<bind::JsManagedResource>&& wrappedResource) {
    auto iter = classConstructors_.find(&bind);
    if (iter == classConstructors_.end()) {
        throw Exception{"The native class " + bind.name_ + " is not registered, so an instance cannot be constructed."};
    }

    v8::TryCatch vtry{isolate_};

    auto ctx  = context_.Get(isolate_);
    auto ctor = iter->second.Get(isolate_)->GetFunction(ctx);
    Exception::rethrow(vtry);

    // (symbol, instance)
    auto args = std::vector<v8::Local<v8::Value>>{
        constructorSymbol_.Get(isolate_).As<v8::Value>(),
        v8::External::New(isolate_, wrappedResource.release())
    };
    auto val = ctor.ToLocalChecked()->NewInstance(ctx, static_cast<int>(args.size()), args.data());
    Exception::rethrow(vtry);

    return ValueHelper::wrap<Object>(val.ToLocalChecked());
}

bool Engine::isInstanceOf(Local<Object> const& obj, bind::meta::ClassDefine const& binding) const {
    auto iter = classConstructors_.find(&binding);
    if (iter == classConstructors_.end()) {
        return false;
    }
    auto ctor = iter->second.Get(isolate_);
    return ctor->HasInstance(ValueHelper::unwrap(obj));
}

void* Engine::getNativeInstanceOf(Local<Object> const& obj) const {
    auto v8Obj   = ValueHelper::unwrap(obj);
    auto wrapped = v8Obj->GetAlignedPointerFromInternalField(kInternalField_WrappedResource);
    auto typed   = static_cast<bind::JsManagedResource*>(wrapped);
    if (!isInstanceOf(obj, *typed->define_)) {
        return nullptr;
    }
    return (*typed)();
}

void Engine::gc() const { isolate_->LowMemoryNotification(); }

} // namespace v8wrap