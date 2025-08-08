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


InstanceBinding::Method::Method(std::string name, JsInstanceMethodCallback callback)
: mName(std::move(name)),
  mCallback(std::move(callback)) {}


InstanceBinding::InstanceBinding(
    JsInstanceConstructor constructor,
    std::vector<Property> property,
    std::vector<Method>   functions,
    size_t                classSize
)
: mConstructor(std::move(constructor)),
  mProperty(std::move(property)),
  mMethods(std::move(functions)),
  mClassSize(classSize) {}


ClassBinding::ClassBinding(
    std::string         name,
    StaticBinding       static_,
    InstanceBinding     instance,
    ClassBinding const* parent,
    OwnedHolderCtor     ownedHolderCtor,
    OwnedHolderGetter   ownedHolderGetter,
    OwnedHolderDeleter  ownedHolderDeleter,
    ViewHolderCtor      viewHolderCtor,
    ViewHolderGetter    viewHolderGetter,
    ViewHolderDeleter   viewHolderDeleter
)
: mClassName(std::move(name)),
  mStaticBinding(std::move(static_)),
  mInstanceBinding(std::move(instance)),
  mExtends(parent),
  mOwnedHolderCtor(std::move(ownedHolderCtor)),
  mOwnedHolderGetter(std::move(ownedHolderGetter)),
  mOwnedHolderDeleter(std::move(ownedHolderDeleter)),
  mViewHolderCtor(std::move(viewHolderCtor)),
  mViewHolderGetter(std::move(viewHolderGetter)),
  mViewHolderDeleter(std::move(viewHolderDeleter)) {}

bool ClassBinding::hasInstanceConstructor() const { return mInstanceBinding.mConstructor != nullptr; }

} // namespace v8wrap