#include <utility>

#include "v8-context.h"
#include "v8-isolate.h"
#include "v8-locker.h"
#include "v8-persistent-handle.h"
#include "v8-script.h"
#include "v8wrap/JsException.hpp"
#include "v8wrap/JsReference.hpp"
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


    delete this;
}

bool JsRuntime::isDestroying() const { return mDestroying; }

void JsRuntime::eval(Local<JsString> code) {
    // TODO: implement
}

void JsRuntime::eval(Local<JsString> code, Local<JsString> source) {
    // TODO: implement
}

void JsRuntime::loadFile(std::filesystem::path path) {
    // TODO: implement
}

Local<JsValue> JsRuntime::get(Local<JsString> key) {
    // TODO: implement
    return {};
}

void JsRuntime::set(Local<JsString> key, Local<JsValue> value, bool readOnly) {
    // TODO: implement
}


// private
void JsRuntime::initalizeContext() {
    v8::Locker         locker(mIsolate);
    v8::Isolate::Scope isolate_scope(mIsolate);
    v8::HandleScope    handle_scope(mIsolate);

    if (mContext.IsEmpty()) {
        mContext.Reset(mIsolate, v8::Context::New(mIsolate));
    }

    // TODO: implement
}


} // namespace v8wrap