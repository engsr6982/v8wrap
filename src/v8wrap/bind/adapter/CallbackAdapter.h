#pragma once
#include "v8wrap/reference/Global.h"
#include "v8wrap/reference/Local.h"
#include "v8wrap/runtime/Engine.h"
#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/runtime/Exception.h"
#include "v8wrap/types/Value.h"


#include <array>

namespace v8wrap::bind::adapter {

// Function -> std::function
template <typename R, typename... Args>
inline decltype(auto) bindScriptCallback(Local<Value> const& value) {
    if (!value.isFunction()) [[unlikely]] {
        throw Exception("expected function", Exception::Type::TypeError);
    }
    auto& engine = EngineScope::currentRuntimeChecked();

    Global<Function> global{value.asFunction()}; // keep alive
    return [keep = std::move(global), engine = &engine](Args&&... args) -> R {
        EngineScope lock{engine};

        auto function = keep.get();

        std::array<Value, sizeof...(Args)> argv{ConvertToJs(std::forward<Args>(args))...};
        if constexpr (std::is_void_v<R>) {
            function.call({}, argv);
        } else {
            return ConvertToCpp<R>(function.call({}, argv));
        }
    };
}


} // namespace v8wrap::bind::adapter