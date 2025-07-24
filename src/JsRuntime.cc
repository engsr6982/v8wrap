#include <fstream>
#include <stdexcept>
#include <utility>

#include <v8-context.h>
#include <v8-exception.h>
#include <v8-isolate.h>
#include <v8-locker.h>
#include <v8-message.h>
#include <v8-persistent-handle.h>
#include <v8-script.h>

#include "v8wrap/JsException.hpp"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsRuntime.hpp"
#include "v8wrap/JsValue.hpp"

namespace v8wrap {


JsRuntime::JsRuntime(v8::Isolate* isolate) : mIsolate(isolate) {
    v8::Locker         locker(mIsolate);
    v8::Isolate::Scope isolate_scope(mIsolate);
    v8::HandleScope    handle_scope(mIsolate);
    mContext.Reset(mIsolate, v8::Context::New(mIsolate));
}
JsRuntime::JsRuntime(v8::Isolate* isolate, v8::Local<v8::Context> context)
: mIsolate(isolate),
  mContext(v8::Global<v8::Context>{isolate, context}) {}

JsRuntime::~JsRuntime() = default;


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

Local<JsValue> JsRuntime::eval(Local<JsString> const& code) { return eval(code, JsString::newString("<eval>")); }

Local<JsValue> JsRuntime::eval(Local<JsString> const& code, Local<JsString> const& source) {
    v8::TryCatch try_catch(mIsolate);

    auto v8Code   = code.val;
    auto v8Source = source.val;
    auto ctx      = mContext.Get(mIsolate);

    auto origin = v8::ScriptOrigin(v8Source);
    auto script = v8::Script::Compile(ctx, v8Code, &origin);
    JsException::rethrow(try_catch);

    auto result = script.ToLocalChecked()->Run(ctx);
    JsException::rethrow(try_catch);
    return Local<JsValue>(result.ToLocalChecked());
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

Local<JsValue> JsRuntime::get(Local<JsString> key) {
    // TODO: implement
    return {};
}

void JsRuntime::set(Local<JsString> key, Local<JsValue> value, bool readOnly) {
    // TODO: implement
}


} // namespace v8wrap