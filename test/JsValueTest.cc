#include "catch2/catch_test_macros.hpp"
#include "v8wrap/Types.h"
#include "v8wrap/runtime/Engine.h"
#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/runtime/Exception.h"
#include "v8wrap/runtime/Platform.h"
#include "v8wrap/types/Value.h"
#include <cstddef>
#include <iostream>


struct JsValueTestFixture {
    JsValueTestFixture() { rt = new v8wrap::Engine(); }
    ~JsValueTestFixture() { rt->destroy(); }
    v8wrap::Engine* rt;
};

TEST_CASE_METHOD(JsValueTestFixture, "Null") {
    v8wrap::EngineScope enter(rt);

    auto null = v8wrap::Null::newNull();
    CHECK(null.toString().getValue() == "null");
}

TEST_CASE_METHOD(JsValueTestFixture, "Undefined") {
    v8wrap::EngineScope enter(rt);

    auto undefined = v8wrap::Undefined::newUndefined();
    CHECK(undefined.toString().getValue() == "undefined");
}

TEST_CASE_METHOD(JsValueTestFixture, "Boolean") {
    v8wrap::EngineScope enter(rt);

    auto boolean = v8wrap::Boolean::newBoolean(true);
    CHECK(boolean.getValue() == true);
}

TEST_CASE_METHOD(JsValueTestFixture, "Number") {
    v8wrap::EngineScope enter(rt);
    SECTION("Float") {
        auto num = v8wrap::Number::newNumber(3.1f);
        CHECK(num.getFloat() == 3.1f);
    }
    SECTION("Int") {
        auto num = v8wrap::Number::newNumber(114514);
        CHECK(num.getInt32() == 114514);
    }
    SECTION("Double") {
        auto num = v8wrap::Number::newNumber(3.1415926);
        CHECK(num.getDouble() == 3.1415926);
    }
}

TEST_CASE_METHOD(JsValueTestFixture, "BigInt") {
    v8wrap::EngineScope enter(rt);

    auto bigInt = v8wrap::BigInt::newBigInt(int64_t{114514});
    CHECK(bigInt.getInt64() == 114514);
}

TEST_CASE_METHOD(JsValueTestFixture, "String") {
    v8wrap::EngineScope enter(rt);
    SECTION("std::string") {
        auto str = v8wrap::String::newString("Hello, World!");
        CHECK(str.getValue() == "Hello, World!");
        CHECK(str.length() == 13);
    }
    SECTION("char*") {
        auto str = v8wrap::String::newString("Hello, World!");
        CHECK(str.getValue() == "Hello, World!");
        CHECK(str.length() == 13);
    }
}

TEST_CASE_METHOD(JsValueTestFixture, "Symbol") {
    v8wrap::EngineScope enter(rt);

    auto symbol = v8wrap::Symbol::newSymbol("Hello, World!");
    CHECK(symbol.getDescription().isString());
    CHECK(symbol.getDescription().asString().getValue() == "Hello, World!");
}

TEST_CASE_METHOD(JsValueTestFixture, "Function") {
    v8wrap::EngineScope enter(rt);

    SECTION("Basic Function") {
        auto func = v8wrap::Function::newFunction([](v8wrap::Arguments const& args) {
            CHECK(args.length() == 1);
            CHECK(args[0].isString());
            CHECK(args[0].asString().getValue() == "Hello, World!");
            return v8wrap::Boolean::newBoolean(true);
        });
        rt->getGlobalThis().set(v8wrap::String::newString("testFunc"), func);

        auto value = rt->eval(v8wrap::String::newString("testFunc('Hello, World!');"));
        REQUIRE(value.isBoolean());
        CHECK(value.asBoolean().getValue() == true);
    }

    SECTION("Function with callback") {
        auto func = v8wrap::Function::newFunction([](v8wrap::Arguments const& args) {
            REQUIRE(args.length() == 1);
            REQUIRE(args[0].isFunction());
            auto result = args[0].asFunction().call(
                {},
                v8wrap::String::newString("Hello, World!"),
                v8wrap::Number::newNumber(114514)
            );
            return result;
        });
        rt->getGlobalThis().set(v8wrap::String::newString("testFunc2"), func);

        try {
            auto value = rt->eval(v8wrap::String::newString("testFunc2((str, num) => { return num; });"));
            REQUIRE(value.isNumber());
            CHECK(value.asNumber().getInt32() == 114514);
        } catch (v8wrap::Exception const& e) {
            std::cout << e.what() << std::endl;
        }
    }

    SECTION("Function with exception") {
        auto func = v8wrap::Function::newFunction([](v8wrap::Arguments const&) -> v8wrap::Local<v8wrap::Value> {
            throw v8wrap::Exception("Test Exception"); // native => js
        });
        rt->getGlobalThis().set(v8wrap::String::newString("testFunc3"), func);

        CHECK_THROWS_AS(rt->eval(v8wrap::String::newString("testFunc3();")), v8wrap::Exception);


        // js => native
        auto func2 = v8wrap::Function::newFunction([](v8wrap::Arguments const& args) {
            REQUIRE(args.length() == 1);
            REQUIRE(args[0].isFunction());
            CHECK_THROWS_AS(args[0].asFunction().call({}, {}), v8wrap::Exception);
            return v8wrap::Boolean::newBoolean(true);
        });
        rt->getGlobalThis().set(v8wrap::String::newString("testFunc4"), func2);

        auto res = rt->eval(v8wrap::String::newString("testFunc4(() => { throw new Error('test'); });"));
        REQUIRE(res.isBoolean());
    }
}

TEST_CASE_METHOD(JsValueTestFixture, "Object") {
    v8wrap::EngineScope enter(rt);

    SECTION("Basic Object") {
        auto obj = v8wrap::Object::newObject();
        CHECK(obj.toString().getValue() == "[object Object]");
    }

    SECTION("Object Property Operations") {
        auto obj   = v8wrap::Object::newObject();
        auto key   = v8wrap::String::newString("testKey");
        auto value = v8wrap::String::newString("testValue");

        // Test set and get
        obj.set(key, value);
        CHECK(obj.has(key) == true);
        auto retrieved = obj.get(key);
        CHECK(retrieved.isString());
        CHECK(retrieved.asString().getValue() == "testValue");

        // Test property names
        auto names = obj.getOwnPropertyNamesAsString();
        REQUIRE(names.size() == 1);
        CHECK(names[0] == "testKey");

        // Test remove
        obj.remove(key);
        CHECK(obj.has(key) == false);
    }

    SECTION("Instance Check") {
        auto obj         = v8wrap::Object::newObject();
        auto constructor = v8wrap::Object::newObject();
        CHECK_THROWS_AS(obj.instanceof(constructor) == false, v8wrap::Exception);
    }

    SECTION("Object Prototype") {
        auto obj = v8wrap::Object::newObject();
        auto res = obj.defineOwnProperty(
            v8wrap::String::newString("test"),
            v8wrap::Number::newNumber(114),
            static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete)
        );
        CHECK(res == true);

        obj.set(v8wrap::String::newString("test"), v8wrap::Number::newNumber(514));

        CHECK_FALSE(obj.get(v8wrap::String::newString("test")).asNumber().getInt32() == 514);
        CHECK(obj.get(v8wrap::String::newString("test")).asNumber().getInt32() == 114);
    }
}

TEST_CASE_METHOD(JsValueTestFixture, "Array") {
    v8wrap::EngineScope enter(rt);

    SECTION("Basic Array") {
        auto arr = v8wrap::Array::newArray();
        CHECK(arr.length() == 0);
        CHECK(arr.toString().getValue() == "");
    }

    SECTION("Array Operations") {
        auto arr = v8wrap::Array::newArray();

        // Test push and length
        auto str1 = v8wrap::String::newString("item1");
        auto str2 = v8wrap::String::newString("item2");
        arr.push(str1);
        arr.push(str2);
        CHECK(arr.length() == 2);

        // Test get
        CHECK(arr.get(0).asString().getValue() == "item1");
        CHECK(arr.get(1).asString().getValue() == "item2");

        // Test set
        auto newValue = v8wrap::String::newString("newItem");
        arr.set(1, newValue);
        CHECK(arr.get(1).asString().getValue() == "newItem");

        // Test clear
        arr.clear();
        CHECK(arr.length() == 0);
    }

    SECTION("Fixed Length Array") {
        auto arr = v8wrap::Array::newArray(3);
        CHECK(arr.length() == 3);

        // Initialize array elements
        for (size_t i = 0; i < 3; i++) {
            auto num = v8wrap::Number::newNumber(static_cast<int>(i));
            arr.set(i, num);
        }

        // Verify elements
        for (size_t i = 0; i < 3; i++) {
            CHECK(arr.get(i).asNumber().getInt32() == static_cast<int>(i));
        }
    }
}

TEST_CASE_METHOD(JsValueTestFixture, "Array Boundary Tests") {
    v8wrap::EngineScope enter(rt);
    auto                arr = v8wrap::Array::newArray(3);

    SECTION("Out-of-bounds access returns undefined") {
        CHECK(arr.get(10).isUndefined()); // undefined
    }

    SECTION("Negative index returns undefined") {
        CHECK(arr.get(static_cast<size_t>(-1)).isUndefined()); // undefined
    }

    SECTION("Valid access works") {
        arr.set(0, v8wrap::Number::newNumber(42));
        CHECK(arr.get(0).asNumber().getInt32() == 42);
    }
}