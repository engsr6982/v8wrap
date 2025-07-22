#include <utility>

#include "v8-context.h"
#include "v8-isolate.h"
#include "v8-locker.h"
#include "v8-persistent-handle.h"
#include "v8wrap/JsException.hpp"
#include "v8wrap/JsRuntime.hpp"


namespace v8wrap {


JsRuntime::JsRuntime(v8::Isolate* isolate) : mIsolate(isolate) { initalizeContext(); }
JsRuntime::JsRuntime(v8::Isolate* isolate, v8::Local<v8::Context> context)
: mIsolate(isolate),
  mContext(v8::Global<v8::Context>{isolate, context}) {
    initalizeContext();
}

JsRuntime::~JsRuntime() {
    // TODO: implement
}


v8::Isolate*           JsRuntime::isolate() const { return mIsolate; }
v8::Local<v8::Context> JsRuntime::context() const { return mContext.Get(mIsolate); }

void JsRuntime::setData(std::shared_ptr<void> data) { mUserData = std::move(data); }

void JsRuntime::destroy() {
    if (isDestroying()) return;
    mDestroying = true;

    // TODO: implement
}

bool JsRuntime::isDestroying() const { return mDestroying; }


// private
void JsRuntime::initalizeContext() {
    v8::Locker         locker(mIsolate);
    v8::Isolate::Scope isolate_scope(mIsolate);
    v8::HandleScope    handle_scope(mIsolate);

    if (mContext.IsEmpty()) {
        mContext.Reset(mIsolate, v8::Context::New(mIsolate));
    }
}

v8::Local<v8::Value> JsRuntime::unwrap(Local<JsValue> const& value) {
    throw JsException("Not implemented"); // TODO
}

Local<JsValue> JsRuntime::wrap(v8::Local<v8::Value> const& value) {
    throw JsException("Not implemented"); // TODO
}


} // namespace v8wrap