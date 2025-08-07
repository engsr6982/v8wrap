#include "v8wrap/JsRuntime.hpp"
#include "v8-external.h"
#include "v8-function-callback.h"
#include "v8-local-handle.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-template.h"
#include "v8wrap/Bindings.hpp"
#include "v8wrap/JsException.hpp"
#include "v8wrap/JsPlatform.hpp"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsValue.hpp"
#include "v8wrap/Native.hpp"
#include <cassert>
#include <fstream>
#include <utility>
#include <vector>

V8_WRAP_WARNING_GUARD_BEGIN
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


JsRuntime::JsRuntime() : mPlatform(JsPlatform::getPlatform()) {
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

JsRuntime::JsRuntime(v8::Isolate* isolate, v8::Local<v8::Context> context)
: mIsolate(isolate),
  mContext(v8::Global<v8::Context>{isolate, context}),
  mIsExternalIsolate(true) {
    mConstructorSymbol = v8::Global<v8::Symbol>(mIsolate, v8::Symbol::New(mIsolate));
}

JsRuntime::~JsRuntime() = default;


v8::Isolate*           JsRuntime::isolate() const { return mIsolate; }
v8::Local<v8::Context> JsRuntime::context() const { return mContext.Get(mIsolate); }

void JsRuntime::setData(std::shared_ptr<void> data) { mUserData = std::move(data); }

void JsRuntime::destroy() {
    if (isDestroying()) return;
    mDestroying = true;

    if (mUserData) mUserData.reset();

    {
        JsRuntimeScope scope(this);

        for (auto& [key, value] : mManagedResources) {
            value.Reset();
            key->deleter(key->resource);
            delete key;
        }
        for (auto& [_, ctor] : mJsClassConstructor) {
            ctor.Reset();
        }

        // TODO: implement

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

bool JsRuntime::isDestroying() const { return mDestroying; }

Local<JsValue> JsRuntime::eval(Local<JsString> const& code) { return eval(code, JsString::newString("<eval>")); }

Local<JsValue> JsRuntime::eval(Local<JsString> const& code, Local<JsString> const& source) {
    v8::TryCatch try_catch(mIsolate);

    auto v8Code   = JsValueHelper::unwrap(code);
    auto v8Source = JsValueHelper::unwrap(source);
    auto ctx      = mContext.Get(mIsolate);

    auto origin = v8::ScriptOrigin(v8Source);
    auto script = v8::Script::Compile(ctx, v8Code, &origin);
    JsException::rethrow(try_catch);

    auto result = script.ToLocalChecked()->Run(ctx);
    JsException::rethrow(try_catch);
    return JsValueHelper::wrap<JsValue>(result.ToLocalChecked());
}

void JsRuntime::loadFile(std::filesystem::path const& path) {
    if (isDestroying()) return;
    if (!std::filesystem::exists(path)) {
        throw JsException("File not found: " + path.string());
    }
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        throw JsException("Failed to open file: " + path.string());
    }
    std::string code((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    eval(JsString::newString(code), JsString::newString(path.string()));
}

Local<JsObject> JsRuntime::getGlobalThis() const {
    return JsValueHelper::wrap<JsObject>(mContext.Get(mIsolate)->Global());
}

Local<JsValue> JsRuntime::getVauleFromGlobalThis(Local<JsString> const& key) const {
    auto globalThis = getGlobalThis();
    if (!globalThis.has(key)) return {};
    return globalThis.get(key);
}

void JsRuntime::setVauleToGlobalThis(Local<JsString> const& key, Local<JsValue> const& value) const {
    auto globalThis = getGlobalThis();
    globalThis.set(key, value);
}

void JsRuntime::addManagedResource(void* resource, v8::Local<v8::Value> value, std::function<void(void*)>&& deleter) {
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
                    managed->deleter(managed->resource);
                    delete managed;
                });
            }
        },
        v8::WeakCallbackType::kParameter
    );
    mManagedResources.emplace(managed.release(), std::move(weak));
}

void JsRuntime::registerBindingClass(ClassBinding const& binding) {
    if (mRegisteredBindings.contains(binding.mClassName)) {
        throw JsException("Class binding already registered: " + binding.mClassName);
    }

    v8::TryCatch vtry(mIsolate);

    v8::Local<v8::FunctionTemplate> ctor; // js: new T()
    if (binding.hasInstanceConstructor()) {
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

    auto scriptClassName = JsString::newString(binding.mClassName);
    ctor->SetClassName(JsValueHelper::unwrap(scriptClassName));

    if (binding.mExtends != nullptr) {
        auto iter = mJsClassConstructor.find(binding.mExtends);
        if (iter == mJsClassConstructor.end()) {
            throw JsException{
                binding.mClassName + " cannot inherit from " + binding.mExtends->mClassName
                + " because the parent class is not registered."
            };
        }
        auto parentCtor = iter->second.Get(mIsolate);
        ctor->Inherit(parentCtor);
    }

    implStaticRegister(ctor, binding.mStaticBinding);
    implInstanceRegister(ctor, binding.mInstanceBinding);

    auto function = ctor->GetFunction(mContext.Get(mIsolate));
    JsException::rethrow(vtry);

    mRegisteredBindings.emplace(binding.mClassName, &binding);
    mJsClassConstructor.emplace(&binding, v8::Global<v8::FunctionTemplate>{mIsolate, ctor});

    setVauleToGlobalThis(scriptClassName, JsValueHelper::wrap<JsFunction>(function.ToLocalChecked()));
}

void JsRuntime::implStaticRegister(v8::Local<v8::FunctionTemplate>& ctor, StaticBinding const& staticBinding) {
    for (auto& property : staticBinding.mProperty) {
        auto scriptPropertyName = JsString::newString(property.mName);

        auto v8Getter = [](v8::Local<v8::Name>, v8::PropertyCallbackInfo<v8::Value> const& info) {
            auto pbin = static_cast<StaticBinding::Property*>(info.Data().As<v8::External>()->Value());
            try {
                auto ret = pbin->mGetter();
                info.GetReturnValue().Set(JsValueHelper::unwrap(ret));
            } catch (JsException const& e) {
                e.rethrowToRuntime();
            }
        };

        v8::AccessorNameSetterCallback v8Setter = nullptr;
        if (property.mSetter) {
            v8Setter = [](v8::Local<v8::Name>, v8::Local<v8::Value> value, v8::PropertyCallbackInfo<void> const& info) {
                auto pbin = static_cast<StaticBinding::Property*>(info.Data().As<v8::External>()->Value());
                try {
                    pbin->mSetter(JsValueHelper::wrap<JsValue>(value));
                } catch (JsException const& e) {
                    e.rethrowToRuntime();
                }
            };
        } else {
            v8Setter = [](v8::Local<v8::Name>, v8::Local<v8::Value>, v8::PropertyCallbackInfo<void> const&) {
                JsException(
                    "Native property have only one getter, and you cannot modify native property without "
                    "getters",
                    JsException::Type::TypeError
                )
                    .rethrowToRuntime();
            };
        }

        ctor->SetNativeDataProperty(
            JsValueHelper::unwrap(scriptPropertyName).As<v8::Name>(),
            std::move(v8Getter),
            std::move(v8Setter),
            v8::External::New(mIsolate, const_cast<StaticBinding::Property*>(&property)),
            v8::PropertyAttribute::DontDelete
        );
    }
    for (auto& function : staticBinding.mFunctions) {
        auto scriptFunctionName = JsString::newString(function.mName);

        auto fn = v8::FunctionTemplate::New(
            mIsolate,
            [](v8::FunctionCallbackInfo<v8::Value> const& info) {
                auto fbin = static_cast<StaticBinding::Function*>(info.Data().As<v8::External>()->Value());

                try {
                    auto ret = (fbin->mCallback)(Arguments{JsRuntimeScope::currentRuntime(), info});
                    info.GetReturnValue().Set(JsValueHelper::unwrap(ret));
                } catch (JsException const& e) {
                    e.rethrowToRuntime();
                }
            },
            v8::External::New(mIsolate, const_cast<StaticBinding::Function*>(&function)),
            {},
            0,
            v8::ConstructorBehavior::kThrow
        );
        ctor->Set(JsValueHelper::unwrap(scriptFunctionName).As<v8::Name>(), fn, v8::PropertyAttribute::DontDelete);
    }
}

v8::Local<v8::FunctionTemplate> JsRuntime::createInstanceClassCtor(ClassBinding const& binding) {
    v8::TryCatch vtry{mIsolate};

    v8::Local<v8::Object> data = v8::Object::New(mIsolate);
    JsException::rethrow(vtry);

    auto ctx = mContext.Get(mIsolate);

    auto _ = data->Set(ctx, 0, v8::External::New(mIsolate, const_cast<ClassBinding*>(&binding)));
    JsException::rethrow(vtry);

    _ = data->Set(ctx, 1, v8::External::New(mIsolate, this));
    JsException::rethrow(vtry);

    auto ctor = v8::FunctionTemplate::New(
        mIsolate,
        [](v8::FunctionCallbackInfo<v8::Value> const& info) {
            auto ctx  = info.GetIsolate()->GetCurrentContext();
            auto data = info.Data().As<v8::Object>();

            auto binding = static_cast<ClassBinding*>(data->Get(ctx, 0).ToLocalChecked().As<v8::External>()->Value());
            auto runtime = static_cast<JsRuntime*>(data->Get(ctx, 1).ToLocalChecked().As<v8::External>()->Value());

            auto& ctor = binding->mInstanceBinding.mConstructor;

            try {
                if (!info.IsConstructCall()) {
                    throw JsException{"Native class constructor cannot be called as a function"};
                }

                void* instance = nullptr;
                if (info.Length() == 2 && info[0]->IsSymbol()
                    && info[0]->StrictEquals(runtime->mConstructorSymbol.Get(runtime->mIsolate))
                    && info[1]->IsExternal()) {
                    instance = info[1].As<v8::External>()->Value(); // constructor call from native code
                } else {
                    instance = ctor(Arguments{runtime, info}); // constructor call from JS code
                }

                if (instance == nullptr) {
                    throw JsException{"This native class cannot be constructed."};
                }

                void* holder = binding->wrapInstance(instance);
                {
                    auto typedHolder           = reinterpret_cast<IHolder*>(holder);
                    typedHolder->mRuntime      = runtime;
                    typedHolder->mClassBinding = binding;
                }

                info.This()->SetAlignedPointerInInternalField(0, holder);
                runtime->mIsolate->AdjustAmountOfExternalAllocatedMemory(
                    static_cast<int64_t>(binding->mInstanceBinding.mClassSize)
                );

                runtime->addManagedResource(holder, info.This(), [](void* holder) {
                    auto* typedHolder = reinterpret_cast<IHolder*>(holder);

                    auto runtime = typedHolder->mRuntime;
                    auto binding = typedHolder->mClassBinding;

                    runtime->mIsolate->AdjustAmountOfExternalAllocatedMemory(
                        -static_cast<int64_t>(binding->mInstanceBinding.mClassSize)
                    );
                    binding->deleteHolderAndInstance(holder);
                });
            } catch (JsException const& e) {
                e.rethrowToRuntime();
            }
        },
        data
    );
    ctor->InstanceTemplate()->SetInternalFieldCount(1);
    return ctor;
}

void JsRuntime::implInstanceRegister(v8::Local<v8::FunctionTemplate>& ctor, InstanceBinding const& instanceBinding) {
    auto iTemplate = ctor->InstanceTemplate();
    auto signature = v8::Signature::New(mIsolate);

    for (auto& method : instanceBinding.mMethods) {
        auto scriptMethodName = JsString::newString(method.mName);

        auto fn = v8::FunctionTemplate::New(
            mIsolate,
            [](v8::FunctionCallbackInfo<v8::Value> const& info) {
                auto method = static_cast<InstanceBinding::Method*>(info.Data().As<v8::External>()->Value());
                auto holder = info.This()->GetAlignedPointerFromInternalField(0);

                auto typedHolder = reinterpret_cast<IHolder*>(holder);
                auto runtime     = const_cast<JsRuntime*>(typedHolder->mRuntime);
                auto binding     = typedHolder->mClassBinding;

                auto thiz = binding->unwrapInstance(holder, runtime);

                try {
                    auto val = (method->mCallback)(thiz, Arguments{runtime, info});
                    info.GetReturnValue().Set(JsValueHelper::unwrap(val));
                } catch (JsException const& e) {
                    e.rethrowToRuntime();
                }
            },
            v8::External::New(mIsolate, const_cast<InstanceBinding::Method*>(&method)),
            signature
        );
        iTemplate->Set(JsValueHelper::unwrap(scriptMethodName), fn, v8::PropertyAttribute::DontDelete);
    }
}

Local<JsObject> JsRuntime::newBindingClass(ClassBinding const& binding, void* instance) {
    auto iter = mJsClassConstructor.find(&binding);
    if (iter == mJsClassConstructor.end()) {
        throw JsException{
            "The native class " + binding.mClassName + " is not registered, so an instance cannot be constructed."
        };
    }

    v8::TryCatch vtry{mIsolate};

    auto ctx  = mContext.Get(mIsolate);
    auto ctor = iter->second.Get(mIsolate)->GetFunction(ctx);
    JsException::rethrow(vtry);

    auto args = std::vector<v8::Local<v8::Value>>{
        mConstructorSymbol.Get(mIsolate).As<v8::Value>(),
        v8::External::New(mIsolate, instance)
    };
    auto val = ctor.ToLocalChecked()->NewInstance(ctx, static_cast<int>(args.size()), args.data());
    JsException::rethrow(vtry);

    return JsValueHelper::wrap<JsObject>(val.ToLocalChecked());
}

Local<JsObject> JsRuntime::newBindingClass(std::string const& className, void* instance) {
    auto iter = mRegisteredBindings.find(className);
    if (iter == mRegisteredBindings.end()) {
        // return {}; // undefined
        throw JsException{
            "The native class " + className + " is not registered, so an instance cannot be constructed."
        };
    }
    return newBindingClass(*iter->second, instance);
}

bool JsRuntime::isInstanceOf(Local<JsObject> const& obj, ClassBinding const& binding) const {
    auto iter = mJsClassConstructor.find(&binding);
    if (iter == mJsClassConstructor.end()) {
        return false;
    }
    auto ctor = iter->second.Get(mIsolate);
    return ctor->HasInstance(JsValueHelper::unwrap(obj));
}

void* JsRuntime::getBindingClassInstance(Local<JsObject> const& obj) const {
    auto v8Obj       = JsValueHelper::unwrap(obj);
    auto holder      = v8Obj->GetAlignedPointerFromInternalField(0);
    auto typedHolder = reinterpret_cast<IHolder*>(holder);
    if (!isInstanceOf(obj, *typedHolder->mClassBinding)) {
        return nullptr;
    }
    return typedHolder->mClassBinding->unwrapInstance(holder, const_cast<JsRuntime*>(typedHolder->mRuntime));
}

void JsRuntime::gc() const { mIsolate->LowMemoryNotification(); }

} // namespace v8wrap