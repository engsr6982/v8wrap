#include "v8wrap/JsValue.hpp"
#include "catch2/catch_test_macros.hpp"
#include "v8wrap/JsException.hpp"
#include "v8wrap/JsPlatform.hpp"
#include "v8wrap/JsRuntime.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/Types.hpp"
#include <iostream>


struct JsValueTestFixture {
    JsValueTestFixture() { rt = new v8wrap::JsRuntime(); }
    ~JsValueTestFixture() { rt->destroy(); }
    v8wrap::JsRuntime* rt;
};

TEST_CASE_METHOD(JsValueTestFixture, "JsNull") {
    v8wrap::JsRuntimeScope enter(rt);

    auto null = v8wrap::JsNull::newNull();
    CHECK(null.toString().getValue() == "null");
}

TEST_CASE_METHOD(JsValueTestFixture, "JsUndefined") {
    v8wrap::JsRuntimeScope enter(rt);

    auto undefined = v8wrap::JsUndefined::newUndefined();
    CHECK(undefined.toString().getValue() == "undefined");
}

TEST_CASE_METHOD(JsValueTestFixture, "JsBoolean") {
    v8wrap::JsRuntimeScope enter(rt);

    auto boolean = v8wrap::JsBoolean::newBoolean(true);
    CHECK(boolean.getValue() == true);
}

TEST_CASE_METHOD(JsValueTestFixture, "JsNumber") {
    v8wrap::JsRuntimeScope enter(rt);
    SECTION("Float") {
        auto num = v8wrap::JsNumber::newNumber(3.1f);
        CHECK(num.getFloat() == 3.1f);
    }
    SECTION("Int") {
        auto num = v8wrap::JsNumber::newNumber(114514);
        CHECK(num.getInt32() == 114514);
    }
    SECTION("Double") {
        auto num = v8wrap::JsNumber::newNumber(3.1415926);
        CHECK(num.getDouble() == 3.1415926);
    }
}

TEST_CASE_METHOD(JsValueTestFixture, "JsBigInt") {
    v8wrap::JsRuntimeScope enter(rt);

    auto bigInt = v8wrap::JsBigInt::newBigInt(114514);
    CHECK(bigInt.getInt64() == 114514);
}

TEST_CASE_METHOD(JsValueTestFixture, "JsString") {
    v8wrap::JsRuntimeScope enter(rt);
    SECTION("std::string") {
        auto str = v8wrap::JsString::newString("Hello, World!");
        CHECK(str.getValue() == "Hello, World!");
        CHECK(str.length() == 13);
    }
    SECTION("char*") {
        auto str = v8wrap::JsString::newString("Hello, World!");
        CHECK(str.getValue() == "Hello, World!");
        CHECK(str.length() == 13);
    }
}

TEST_CASE_METHOD(JsValueTestFixture, "JsSymbol") {
    v8wrap::JsRuntimeScope enter(rt);

    auto symbol = v8wrap::JsSymbol::newSymbol("Hello, World!");
    CHECK(symbol.getDescription().isString());
    CHECK(symbol.getDescription().asString().getValue() == "Hello, World!");
}

TEST_CASE_METHOD(JsValueTestFixture, "JsFunction") {
    v8wrap::JsRuntimeScope enter(rt);

    SECTION("Basic Function") {
        auto func = v8wrap::JsFunction::newFunction([](v8wrap::Arguments const& args) {
            CHECK(args.length() == 1);
            CHECK(args[0].isString());
            CHECK(args[0].asString().getValue() == "Hello, World!");
            return v8wrap::JsBoolean::newBoolean(true);
        });
        rt->getGlobalThis().set(v8wrap::JsString::newString("testFunc"), func);

        auto value = rt->eval(v8wrap::JsString::newString("testFunc('Hello, World!');"));
        REQUIRE(value.isBoolean());
        CHECK(value.asBoolean().getValue() == true);
    }

    SECTION("Function with callback") {
        auto func = v8wrap::JsFunction::newFunction([](v8wrap::Arguments const& args) {
            REQUIRE(args.length() == 1);
            REQUIRE(args[0].isFunction());
            auto result = args[0].asFunction().call(
                {},
                {v8wrap::JsString::newString("Hello, World!"), v8wrap::JsNumber::newNumber(114514)}
            );
            return result;
        });
        rt->getGlobalThis().set(v8wrap::JsString::newString("testFunc2"), func);

        try {
            auto value = rt->eval(v8wrap::JsString::newString("testFunc2((str, num) => { return num; });"));
            REQUIRE(value.isNumber());
            CHECK(value.asNumber().getInt32() == 114514);
        } catch (v8wrap::JsException const& e) {
            std::cout << e.what() << std::endl;
        }
    }

    SECTION("Function with exception") {
        auto func = v8wrap::JsFunction::newFunction([](v8wrap::Arguments const&) -> v8wrap::Local<v8wrap::JsValue> {
            throw v8wrap::JsException("Test Exception"); // native => js
        });
        rt->getGlobalThis().set(v8wrap::JsString::newString("testFunc3"), func);

        CHECK_THROWS_AS(rt->eval(v8wrap::JsString::newString("testFunc3();")), v8wrap::JsException);


        // js => native
        auto func2 = v8wrap::JsFunction::newFunction([](v8wrap::Arguments const& args) {
            REQUIRE(args.length() == 1);
            REQUIRE(args[0].isFunction());
            CHECK_THROWS_AS(args[0].asFunction().call({}, {}), v8wrap::JsException);
            return v8wrap::JsBoolean::newBoolean(true);
        });
        rt->getGlobalThis().set(v8wrap::JsString::newString("testFunc4"), func2);

        auto res = rt->eval(v8wrap::JsString::newString("testFunc4(() => { throw new Error('test'); });"));
        REQUIRE(res.isBoolean());
    }
}

TEST_CASE_METHOD(JsValueTestFixture, "JsObject") {
    v8wrap::JsRuntimeScope enter(rt);

    SECTION("Basic Object") {
        auto obj = v8wrap::JsObject::newObject();
        CHECK(obj.toString().getValue() == "[object Object]");
    }

    SECTION("Object Property Operations") {
        auto obj   = v8wrap::JsObject::newObject();
        auto key   = v8wrap::JsString::newString("testKey");
        auto value = v8wrap::JsString::newString("testValue");

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
        v8wrap::JsRuntimeScope enter(rt);

        auto obj         = v8wrap::JsObject::newObject();
        auto constructor = v8wrap::JsObject::newObject();
        CHECK_THROWS_AS(obj.instanceof(constructor) == false, v8wrap::JsException);
    }

    SECTION("Object Prototype") {
        auto obj = v8wrap::JsObject::newObject();
        auto res = obj.defineOwnProperty(
            v8wrap::JsString::newString("test"),
            v8wrap::JsNumber::newNumber(114),
            static_cast<v8::PropertyAttribute>(v8::PropertyAttribute::ReadOnly | v8::PropertyAttribute::DontDelete)
        );
        CHECK(res == true);

        obj.set(v8wrap::JsString::newString("test"), v8wrap::JsNumber::newNumber(514));

        CHECK_FALSE(obj.get(v8wrap::JsString::newString("test")).asNumber().getInt32() == 514);
        CHECK(obj.get(v8wrap::JsString::newString("test")).asNumber().getInt32() == 114);
    }
}

TEST_CASE_METHOD(JsValueTestFixture, "JsArray") {
    v8wrap::JsRuntimeScope enter(rt);

    SECTION("Basic Array") {
        auto arr = v8wrap::JsArray::newArray();
        CHECK(arr.length() == 0);
        CHECK(arr.toString().getValue() == "");
    }

    SECTION("Array Operations") {
        auto arr = v8wrap::JsArray::newArray();

        // Test push and length
        auto str1 = v8wrap::JsString::newString("item1");
        auto str2 = v8wrap::JsString::newString("item2");
        arr.push(str1);
        arr.push(str2);
        CHECK(arr.length() == 2);

        // Test get
        CHECK(arr.get(0).asString().getValue() == "item1");
        CHECK(arr.get(1).asString().getValue() == "item2");

        // Test set
        auto newValue = v8wrap::JsString::newString("newItem");
        arr.set(1, newValue);
        CHECK(arr.get(1).asString().getValue() == "newItem");

        // Test clear
        arr.clear();
        CHECK(arr.length() == 0);
    }

    SECTION("Fixed Length Array") {
        auto arr = v8wrap::JsArray::newArray(3);
        CHECK(arr.length() == 3);

        // Initialize array elements
        for (size_t i = 0; i < 3; i++) {
            auto num = v8wrap::JsNumber::newNumber(static_cast<int>(i));
            arr.set(i, num);
        }

        // Verify elements
        for (size_t i = 0; i < 3; i++) {
            CHECK(arr.get(i).asNumber().getInt32() == static_cast<int>(i));
        }
    }
}

TEST_CASE_METHOD(JsValueTestFixture, "JsArray Boundary Tests") {
    v8wrap::JsRuntimeScope enter(rt);
    auto                   arr = v8wrap::JsArray::newArray(3);

    SECTION("Out-of-bounds access returns undefined") {
        CHECK(arr.get(10).isUndefined()); // undefined
    }

    SECTION("Negative index returns undefined") {
        CHECK(arr.get(-1).isUndefined()); // undefined
    }

    SECTION("Valid access works") {
        arr.set(0, v8wrap::JsNumber::newNumber(42));
        CHECK(arr.get(0).asNumber().getInt32() == 42);
    }
}