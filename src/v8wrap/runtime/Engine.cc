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


Engine::Engine() : mPlatform(Platform::getPlatform()) {
    v8::Isolate::CreateParams params;
    params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();

    mIsolate = v8::Isolate::New(params);

    v8::Locker         locker(mIsolate);
    v8::Isolate::Scope isolate_scope(mIsolate);
    v8::HandleScope    handle_scope(mIsolate);
    mContext.Reset(mIsolate, v8::Context::New(mIsolate));
    mPlatform->addRuntime(this);

    mConstructorSymbol = v8::Global<v8::Symbol>(mIsolate, v8::Symbol::New(mIsolate));
}

Engine::Engine(v8::Isolate* isolate, v8::Local<v8::Context> context)
: mIsolate(isolate),
  mContext(v8::Global<v8::Context>{isolate, context}),
  mIsExternalIsolate(true) {
    mConstructorSymbol = v8::Global<v8::Symbol>(mIsolate, v8::Symbol::New(mIsolate));
}

Engine::~Engine() = default;


v8::Isolate*           Engine::isolate() const { return mIsolate; }
v8::Local<v8::Context> Engine::context() const { return mContext.Get(mIsolate); }

void Engine::setData(std::shared_ptr<void> data) { mUserData = std::move(data); }

void Engine::destroy() {
    if (isDestroying()) return;
    mDestroying = true;

    if (mUserData) mUserData.reset();

    {
        EngineScope scope(this);

        for (auto& [key, value] : mManagedResources) {
            value.Reset();
            key->deleter(key->resource);
            delete key;
        }
        for (auto& [_, ctor] : mJsClassConstructor) {
            ctor.Reset();
        }

        mConstructorSymbol.Reset();
        mJsClassConstructor.clear();
        mRegisteredBindings.clear();
        mManagedResources.clear();
        mContext.Reset();
    }

    if (!mIsExternalIsolate) mIsolate->Dispose();
    if (mPlatform) mPlatform->removeRuntime(this, false);

    delete this;
}

bool Engine::isDestroying() const { return mDestroying; }

Local<Value> Engine::eval(Local<String> const& code) { return eval(code, String::newString("<eval>")); }

Local<Value> Engine::eval(Local<String> const& code, Local<String> const& source) {
    v8::TryCatch try_catch(mIsolate);

    auto v8Code   = ValueHelper::unwrap(code);
    auto v8Source = ValueHelper::unwrap(source);
    auto ctx      = mContext.Get(mIsolate);

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

Local<Object> Engine::getGlobalThis() const { return ValueHelper::wrap<Object>(mContext.Get(mIsolate)->Global()); }

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

    v8::Global<v8::Value> weak{mIsolate, value};
    weak.SetWeak(
        static_cast<void*>(managed.get()),
        [](v8::WeakCallbackInfo<void> const& data) {
            auto managed = static_cast<ManagedResource*>(data.GetParameter());
            auto runtime = managed->runtime;
            {
                v8::Locker locker(runtime->mIsolate); // Since the v8 GC is not on the same thread, locking is required
                auto       iter = runtime->mManagedResources.find(managed);
                assert(iter != runtime->mManagedResources.end()); // ManagedResource should be in the map
                runtime->mManagedResources.erase(iter);

                data.SetSecondPassCallback([](v8::WeakCallbackInfo<void> const& data) {
                    auto       managed = static_cast<ManagedResource*>(data.GetParameter());
                    v8::Locker locker(managed->runtime->mIsolate);
                    delete managed;
                });
            }
        },
        v8::WeakCallbackType::kParameter
    );
    mManagedResources.emplace(managed.release(), std::move(weak));
}

void Engine::registerClass(bind::meta::ClassDefine const& binding) {
    if (mRegisteredBindings.contains(binding.name_)) {
        throw Exception("Class binding already registered: " + binding.name_);
    }

    v8::TryCatch vtry(mIsolate);

    v8::Local<v8::FunctionTemplate> ctor; // js: new T()
    if (binding.hasConstructor()) {
        ctor = createInstanceClassCtor(binding);
    } else {
        ctor = v8::FunctionTemplate::New(
            mIsolate,
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
        auto iter = mJsClassConstructor.find(binding.base_);
        if (iter == mJsClassConstructor.end()) {
            throw Exception{
                binding.name_ + " cannot inherit from " + binding.base_->name_
                + " because the parent class is not registered."
            };
        }
        auto parentCtor = iter->second.Get(mIsolate);
        ctor->Inherit(parentCtor);
    }

    implStaticRegister(ctor, binding.staticMemberDef_);
    implInstanceRegister(ctor, binding.instanceMemberDef_);

    auto function = ctor->GetFunction(mContext.Get(mIsolate));
    Exception::rethrow(vtry);

    mRegisteredBindings.emplace(binding.name_, &binding);
    mJsClassConstructor.emplace(&binding, v8::Global<v8::FunctionTemplate>{mIsolate, ctor});

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
            v8::External::New(mIsolate, const_cast<bind::meta::StaticMemberDefine::Property*>(&property)),
            v8::PropertyAttribute::DontDelete
        );
    }
    for (auto& function : staticBinding.functions_) {
        auto scriptFunctionName = String::newString(function.name_);

        auto fn = v8::FunctionTemplate::New(
            mIsolate,
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
            v8::External::New(mIsolate, const_cast<bind::meta::StaticMemberDefine::Function*>(&function)),
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
    v8::TryCatch vtry{mIsolate};

    v8::Local<v8::Object> data = v8::Object::New(mIsolate);
    Exception::rethrow(vtry);

    auto ctx = mContext.Get(mIsolate);

    (void)data->Set(
        ctx,
        kCtorExternal_ClassBinding,
        v8::External::New(mIsolate, const_cast<bind::meta::ClassDefine*>(&binding))
    );
    Exception::rethrow(vtry);

    (void)data->Set(ctx, kCtorExternal_JsRuntime, v8::External::New(mIsolate, this));
    Exception::rethrow(vtry);

    auto ctor = v8::FunctionTemplate::New(
        mIsolate,
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
                    && info[0]->StrictEquals(runtime->mConstructorSymbol.Get(runtime->mIsolate))
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
                    runtime->mIsolate->AdjustAmountOfExternalAllocatedMemory(
                        static_cast<int64_t>(binding->instanceMemberDef_.classSize_)
                    );
                }

                runtime->addManagedResource(wrapped, info.This(), [](void* wrapped) {
                    auto typed = static_cast<bind::JsManagedResource*>(wrapped);

                    if (typed->constructFromJs_) {
                        typed->engine_->mIsolate->AdjustAmountOfExternalAllocatedMemory(
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
    auto signature = v8::Signature::New(mIsolate);

    for (auto& method : instanceBinding.methods_) {
        auto scriptMethodName = String::newString(method.name_);

        auto fn = v8::FunctionTemplate::New(
            mIsolate,
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
            v8::External::New(mIsolate, const_cast<bind::meta::InstanceMemberDefine::Method*>(&method)),
            signature
        );
        prototype->Set(ValueHelper::unwrap(scriptMethodName), fn, v8::PropertyAttribute::DontDelete);
    }

    for (auto& prop : instanceBinding.property_) {
        auto scriptPropertyName = String::newString(prop.name_);
        auto data = v8::External::New(mIsolate, const_cast<bind::meta::InstanceMemberDefine::Property*>(&prop));
        v8::Local<v8::FunctionTemplate> v8Getter;
        v8::Local<v8::FunctionTemplate> v8Setter;

        v8Getter = v8::FunctionTemplate::New(
            mIsolate,
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
                mIsolate,
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
    auto iter = mJsClassConstructor.find(&bind);
    if (iter == mJsClassConstructor.end()) {
        throw Exception{"The native class " + bind.name_ + " is not registered, so an instance cannot be constructed."};
    }

    v8::TryCatch vtry{mIsolate};

    auto ctx  = mContext.Get(mIsolate);
    auto ctor = iter->second.Get(mIsolate)->GetFunction(ctx);
    Exception::rethrow(vtry);

    // (symbol, instance)
    auto args = std::vector<v8::Local<v8::Value>>{
        mConstructorSymbol.Get(mIsolate).As<v8::Value>(),
        v8::External::New(mIsolate, wrappedResource.release())
    };
    auto val = ctor.ToLocalChecked()->NewInstance(ctx, static_cast<int>(args.size()), args.data());
    Exception::rethrow(vtry);

    return ValueHelper::wrap<Object>(val.ToLocalChecked());
}

bool Engine::isInstanceOf(Local<Object> const& obj, bind::meta::ClassDefine const& binding) const {
    auto iter = mJsClassConstructor.find(&binding);
    if (iter == mJsClassConstructor.end()) {
        return false;
    }
    auto ctor = iter->second.Get(mIsolate);
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

void Engine::gc() const { mIsolate->LowMemoryNotification(); }

} // namespace v8wrap