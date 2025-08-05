#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers.hpp"
#include "catch2/matchers/catch_matchers_exception.hpp"
#include "v8wrap/Bindings.hpp"
#include "v8wrap/JsException.hpp"
#include "v8wrap/JsPlatform.hpp"
#include "v8wrap/JsReference.hpp"
#include "v8wrap/JsRuntime.hpp"
#include "v8wrap/JsRuntimeScope.hpp"
#include "v8wrap/JsValue.hpp"
#include "v8wrap/Native.hpp"
#include "v8wrap/Types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <iostream>
#include <utility>


struct BindingTestFixture {
    BindingTestFixture() { rt = new v8wrap::JsRuntime(); }
    ~BindingTestFixture() { rt->destroy(); }
    v8wrap::JsRuntime* rt;
};


void defaultFunc(int a, double b) {
    REQUIRE(a == 1);
    REQUIRE(b == 2.0);
}

void noArgsFunc() { REQUIRE(true); }

bool stdcout(std::string const& str) {
    std::cout << str << std::endl;
    return true;
}

int         overloadedFn(int a) { return a; }
std::string overloadedFn(std::string const& str) { return str; }
std::string overloadedFn(int a, std::string const& str) { return std::to_string(a) + str; }

TEST_CASE_METHOD(BindingTestFixture, "Static Binding") {
    v8wrap::JsRuntimeScope enter(rt);

    SECTION("No return value function") {
        auto fn = v8wrap::JsFunction::newFunction(&defaultFunc);
        rt->getGlobalThis().set(v8wrap::JsString::newString("defaultFunc"), fn);

        REQUIRE(rt->eval("defaultFunc(1, 2.0);").isUndefined()); // void -> undefined

        auto fn2 = v8wrap::JsFunction::newFunction(&noArgsFunc);
        rt->getGlobalThis().set(v8wrap::JsString::newString("noArgsFunc"), fn2);

        REQUIRE(rt->eval("noArgsFunc();").isUndefined()); // void -> undefined
    }

    SECTION("Return value function") {
        auto fn = v8wrap::JsFunction::newFunction(&stdcout);
        rt->getGlobalThis().set(v8wrap::JsString::newString("stdcout"), fn);

        REQUIRE(rt->eval("stdcout('hello');").isBoolean()); // bool -> true/false
        REQUIRE(rt->eval("stdcout('鸡你太美');").isBoolean());
    }

    SECTION("Lambda function") {
        auto add = v8wrap::JsFunction::newFunction([](int a, int b) -> v8wrap::Local<v8wrap::JsValue> {
            return v8wrap::JsNumber::newNumber(a + b);
        });
        rt->getGlobalThis().set(v8wrap::JsString::newString("add"), add);

        auto value = rt->eval("add(1, 2);");
        REQUIRE(value.isNumber());
        REQUIRE(value.asNumber().getInt32() == 3);
    }

    SECTION("Args not matched") {
        auto fn = v8wrap::JsFunction::newFunction(&stdcout);
        rt->getGlobalThis().set(v8wrap::JsString::newString("stdcout1"), fn);

        REQUIRE_THROWS_MATCHES(
            rt->eval("stdcout1();"),
            v8wrap::JsException,
            Catch::Matchers::ExceptionMessageMatcher("Uncaught Error: argument count mismatch")
        );
    }

    SECTION("Args type not matched") {
        auto fn = v8wrap::JsFunction::newFunction(&stdcout);
        rt->getGlobalThis().set(v8wrap::JsString::newString("stdcout2"), fn);

        REQUIRE_THROWS_MATCHES(
            rt->eval("stdcout2(1);"),
            v8wrap::JsException,
            Catch::Matchers::ExceptionMessageMatcher("Uncaught Error: cannot convert to JsString")
        );
    }

    SECTION("Overload") {
        auto fn = v8wrap::JsFunction::newFunction(
            static_cast<int (*)(int)>(&overloadedFn),
            static_cast<std::string (*)(std::string const&)>(&overloadedFn),
            static_cast<std::string (*)(int, std::string const&)>(&overloadedFn)
        );
        rt->getGlobalThis().set(v8wrap::JsString::newString("overloadedFn"), fn);

        auto pick1 = rt->eval("overloadedFn(1.0);");
        REQUIRE(pick1.isNumber());
        REQUIRE(pick1.asNumber().getInt32() == 1);

        auto pick2 = rt->eval("overloadedFn('hello');");
        REQUIRE(pick2.isString());
        REQUIRE(pick2.asString().getValue() == "hello");

        auto pick3 = rt->eval("overloadedFn(1, 'hello');");
        REQUIRE(pick3.isString());
        REQUIRE(pick3.asString().getValue() == "1hello");

        // No matching overload
        REQUIRE_THROWS_MATCHES(
            rt->eval("overloadedFn(1, 2);"),
            v8wrap::JsException,
            Catch::Matchers::ExceptionMessageMatcher("Uncaught Error: no overload found")
        );
    }

    SECTION("Overload with lambda") {
        auto fn = v8wrap::JsFunction::newFunction(
            [](int a) { return a; },
            [](std::string const& str) { return str; },
            [](int a, float b) { return a + b; }
        );
        rt->getGlobalThis().set(v8wrap::JsString::newString("overloadedFn"), fn);

        auto pick1 = rt->eval("overloadedFn(1.0);");
        REQUIRE(pick1.isNumber());
        REQUIRE(pick1.asNumber().getInt32() == 1);

        auto pick2 = rt->eval("overloadedFn('hello');");
        REQUIRE(pick2.isString());
        REQUIRE(pick2.asString().getValue() == "hello");

        auto pick3 = rt->eval("overloadedFn(1, 2.0);");
        REQUIRE(pick3.isNumber());
        REQUIRE(pick3.asNumber().getDouble() == 3.0);

        // No matching overload
        REQUIRE_THROWS_MATCHES(
            rt->eval("overloadedFn(1, 2, 3);"),
            v8wrap::JsException,
            Catch::Matchers::ExceptionMessageMatcher("Uncaught Error: no overload found")
        );
    }
}


class Test {
public:
    // ordinary function
    static int add(int a, int b) { return a + b; }

    // overloaded function
    static std::string append(std::string const& a, std::string const& b) { return a + b; }
    static std::string append(std::string const& a, int b) { return a + std::to_string(b); }

    // script function
    static v8wrap::Local<v8wrap::JsValue> subtract(v8wrap::Arguments const& args) {
        REQUIRE(args.length() == 2);
        REQUIRE(args[0].isNumber());
        REQUIRE(args[1].isNumber());

        double a = args[0].asNumber().getDouble();
        double b = args[1].asNumber().getDouble();
        return v8wrap::JsNumber::newNumber(a - b);
    }

    static int constexpr readOnly = 114; // read-only member
    static std::string name;             // read-write member
    static bool        custom;
};
std::string Test::name   = "Test";
bool        Test::custom = true;


v8wrap::ClassBinding StaticBind =
    v8wrap::bindingClass<void>("Test")
        .function("add", &Test::add)
        .function(
            "append",
            static_cast<std::string (*)(std::string const&, std::string const&)>(&Test::append),
            static_cast<std::string (*)(std::string const&, int)>(&Test::append)
        )
        .function("subtract", &Test::subtract)
        .property("readOnly", &Test::readOnly) // Automatically generate getters and setters (non-const)
        .property("name", &Test::name)
        .property(
            "custom",
            []() -> v8wrap::Local<v8wrap::JsValue> { return v8wrap::JsBoolean::newBoolean(Test::custom); },
            [](v8wrap::Local<v8wrap::JsValue> const& val) { Test::custom = val.asBoolean().getValue(); }
        )
        .build();

TEST_CASE_METHOD(BindingTestFixture, "Static binding") {
    v8wrap::JsRuntimeScope enter{rt};

    rt->registerBindingClass(StaticBind);

    auto exist = rt->eval("Test !== undefined");
    REQUIRE(exist.isBoolean());
    REQUIRE(exist.asBoolean().getValue() == true);

    auto isFunc = rt->eval("typeof Test === 'function'");
    REQUIRE(isFunc.isBoolean());
    REQUIRE(isFunc.asBoolean().getValue() == true);

    // Static classes do not allow new
    REQUIRE_THROWS_MATCHES(
        rt->eval("new Test();"),
        v8wrap::JsException,
        Catch::Matchers::ExceptionMessageMatcher("Uncaught TypeError: Test is not a constructor")
    );

    SECTION("Static properties") {
        // properties can be modified
        REQUIRE(rt->eval("Test.name === 'Test'").asBoolean().getValue() == true);

        rt->eval("Test.name = '啊吧啊吧'");
        REQUIRE(Test::name == "啊吧啊吧"); // Test::name is modified
        REQUIRE(rt->eval("Test.name === '啊吧啊吧'").asBoolean().getValue() == true);

        // readOnly properties cannot be modified
        REQUIRE(rt->eval("Test.readOnly === 114").asBoolean().getValue() == true);
        REQUIRE_THROWS_MATCHES(
            rt->eval("Test.readOnly = 514;"),
            v8wrap::JsException,
            Catch::Matchers::ExceptionMessageMatcher("Uncaught TypeError: Native property have only one getter, and "
                                                     "you cannot modify native property without getters")
        );
        REQUIRE(Test::readOnly == 114); // Test::readOnly is not modified

        // custom properties
        REQUIRE(rt->eval("Test.custom === true").asBoolean().getValue() == true);
        rt->eval("Test.custom = false;");
        REQUIRE(Test::custom == false); // Test::custom is modified
        REQUIRE(rt->eval("Test.custom === false").asBoolean().getValue() == true);
    }

    SECTION("Static functions") {
        auto add = rt->eval("Test.add(1, 2);");
        REQUIRE(add.isNumber());
        REQUIRE(add.asNumber().getInt32() == 3);

        auto pick1 = rt->eval("Test.append('Hello, ', 'world!');");
        REQUIRE(pick1.isString());
        REQUIRE(pick1.asString().getValue() == "Hello, world!");

        auto pick2 = rt->eval("Test.append('Hello, ', 123);");
        REQUIRE(pick2.isString());
        REQUIRE(pick2.asString().getValue() == "Hello, 123");

        auto subtract = rt->eval("Test.subtract(5, 3);");
        REQUIRE(subtract.isNumber());
        REQUIRE(subtract.asNumber().getDouble() == 2);
    }
}


// TODO: Test instance binding
// TODO: Test inheritance
class Actor {
    int mID;

    static int genID() {
        static int id = 0;
        return id++;
    }

public:
    virtual ~Actor() { std::cout << "Actor::~Actor()" << std::endl; }

    virtual std::string getTypeName() const { return "Actor"; }

    virtual int getID() const { return mID; }

    Actor() : mID(genID()) {}
};

class Player : public Actor {
    std::string mName;

public:
    Player(std::string name) : mName(std::move(name)) {}

    std::string getTypeName() const override { return "Player"; }

    std::string getName() const { return mName; }
};

TEST_CASE_METHOD(BindingTestFixture, "Instance binding") {}