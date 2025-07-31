#include "v8wrap/Bindings.hpp"


namespace v8wrap {


StaticBinding::Property::Property(std::string name, JsGetterCallback getter, JsSetterCallback setter)
: mName(std::move(name)),
  mGetter(std::move(getter)),
  mSetter(std::move(setter)) {}


StaticBinding::Function::Function(std::string name, JsFunctionCallback callback)
: mName(std::move(name)),
  mCallback(std::move(callback)) {}


StaticBinding::StaticBinding(std::vector<Property> property, std::vector<Function> functions)
: mProperty(std::move(property)),
  mFunctions(std::move(functions)) {}


InstanceBinding::Property::Property(std::string name, JsInstanceGetterCallback getter, JsInstanceSetterCallback setter)
: mName(std::move(name)),
  mGetter(std::move(getter)),
  mSetter(std::move(setter)) {}


InstanceBinding::Function::Function(std::string name, JsInstanceFunctionCallback callback)
: mName(std::move(name)),
  mCallback(std::move(callback)) {}


InstanceBinding::InstanceBinding(
    JsInstanceConstructor constructor,
    std::vector<Property> property,
    std::vector<Function> functions
)
: mConstructor(std::move(constructor)),
  mProperty(std::move(property)),
  mFunctions(std::move(functions)) {}


ClassBinding::ClassBinding(
    std::string         name,
    StaticBinding       static_,
    InstanceBinding     instance,
    ClassBinding const* parent
)
: mClassName(std::move(name)),
  mStaticBinding(std::move(static_)),
  mInstanceBinding(std::move(instance)),
  mExtends(parent) {}

bool ClassBinding::hasInstanceConstructor() const { return mInstanceBinding.mConstructor != nullptr; }


} // namespace v8wrap