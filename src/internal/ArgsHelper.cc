#include "v8wrap/internal/ArgsHelper.hpp"
#include "v8wrap/JsReference.hpp"

namespace v8wrap::internal {


v8::Local<v8::Value>* extractArgs(std::vector<Local<JsValue>> const& args) {
    static_assert(
        sizeof(Local<JsValue>) == sizeof(v8::Local<v8::Value>),
        "Local<JsValue> must be binary-compatible with v8::Local<v8::Value>"
    );
    if (args.empty()) {
        return nullptr;
    }
    return reinterpret_cast<v8::Local<v8::Value>*>(const_cast<Local<JsValue>*>(args.data()));
}


} // namespace v8wrap::internal