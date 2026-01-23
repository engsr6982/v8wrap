#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers.hpp"
#include "catch2/matchers/catch_matchers_exception.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "v8wrap/Types.h"
#include "v8wrap/bind/TypeConverter.h"
#include "v8wrap/bind/builder/ClassDefineBuilder.h"
#include "v8wrap/reference/Local.h"
#include "v8wrap/runtime/Engine.h"
#include "v8wrap/runtime/EngineScope.h"
#include "v8wrap/runtime/Exception.h"
#include "v8wrap/runtime/Platform.h"
#include "v8wrap/types/Value.h"


#include <iostream>
#include <string>
#include <utility>


struct BindingTestFixture {
    BindingTestFixture() { rt = v8wrap::Platform::getInstance().newEngine(); }
    ~BindingTestFixture() { v8wrap::Platform::getInstance().destroyEngine(rt); }

    v8wrap::Engine* rt;
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
    v8wrap::EngineScope enter(rt);

    SECTION("No return value function") {
        auto fn = v8wrap::Function::newFunction(&defaultFunc);
        rt->getGlobalThis().set(v8wrap::String::newString("defaultFunc"), fn);

        REQUIRE(rt->eval("defaultFunc(1, 2.0);").isUndefined()); // void -> undefined

        auto fn2 = v8wrap::Function::newFunction(&noArgsFunc);
        rt->getGlobalThis().set(v8wrap::String::newString("noArgsFunc"), fn2);

        REQUIRE(rt->eval("noArgsFunc();").isUndefined()); // void -> undefined
    }

    SECTION("Return value function") {
        auto fn = v8wrap::Function::newFunction(&stdcout);
        rt->getGlobalThis().set(v8wrap::String::newString("stdcout"), fn);

        REQUIRE(rt->eval("stdcout('hello');").isBoolean()); // bool -> true/false
        REQUIRE(rt->eval("stdcout('鸡你太美');").isBoolean());
    }

    SECTION("Lambda function") {
        auto add = v8wrap::Function::newFunction([](int a, int b) -> v8wrap::Local<v8wrap::Value> {
            return v8wrap::Number::newNumber(a + b);
        });
        rt->getGlobalThis().set(v8wrap::String::newString("add"), add);

        auto value = rt->eval("add(1, 2);");
        REQUIRE(value.isNumber());
        REQUIRE(value.asNumber().getInt32() == 3);
    }

    SECTION("Args not matched") {
        auto fn = v8wrap::Function::newFunction(&stdcout);
        rt->getGlobalThis().set(v8wrap::String::newString("stdcout1"), fn);

        REQUIRE_THROWS_MATCHES(
            rt->eval("stdcout1();"),
            v8wrap::Exception,
            Catch::Matchers::ExceptionMessageMatcher("Uncaught TypeError: argument count mismatch")
        );
    }

    SECTION("Args type not matched") {
        auto fn = v8wrap::Function::newFunction(&stdcout);
        rt->getGlobalThis().set(v8wrap::String::newString("stdcout2"), fn);

        REQUIRE_THROWS_MATCHES(
            rt->eval("stdcout2(1);"),
            v8wrap::Exception,
            Catch::Matchers::ExceptionMessageMatcher("Uncaught Error: cannot convert to String")
        );
    }

    SECTION("Overload") {
        auto fn = v8wrap::Function::newFunction(
            static_cast<int (*)(int)>(&overloadedFn),
            static_cast<std::string (*)(std::string const&)>(&overloadedFn),
            static_cast<std::string (*)(int, std::string const&)>(&overloadedFn)
        );
        rt->getGlobalThis().set(v8wrap::String::newString("overloadedFn"), fn);

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
            v8wrap::Exception,
            Catch::Matchers::ExceptionMessageMatcher("Uncaught TypeError: no overload found")
        );
    }

    SECTION("Overload with lambda") {
        auto fn = v8wrap::Function::newFunction(
            [](int a) { return a; },
            [](std::string const& str) { return str; },
            [](int a, float b) { return a + b; }
        );
        rt->getGlobalThis().set(v8wrap::String::newString("overloadedFn"), fn);

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
            v8wrap::Exception,
            Catch::Matchers::ExceptionMessageMatcher("Uncaught TypeError: no overload found")
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
    static v8wrap::Local<v8wrap::Value> subtract(v8wrap::Arguments const& args) {
        REQUIRE(args.length() == 2);
        REQUIRE(args[0].isNumber());
        REQUIRE(args[1].isNumber());

        double a = args[0].asNumber().getDouble();
        double b = args[1].asNumber().getDouble();
        return v8wrap::Number::newNumber(a - b);
    }

    static int constexpr readOnly = 114; // read-only member
    static std::string name;             // read-write member
    static bool        custom;
};
std::string Test::name   = "Test";
bool        Test::custom = true;


v8wrap::bind::meta::ClassDefine StaticBind =
    v8wrap::bind::defineClass<void>("Test")
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
            []() -> v8wrap::Local<v8wrap::Value> { return v8wrap::Boolean::newBoolean(Test::custom); },
            [](v8wrap::Local<v8wrap::Value> const& val) { Test::custom = val.asBoolean().getValue(); }
        )
        .build();

TEST_CASE_METHOD(BindingTestFixture, "Static binding") {
    v8wrap::EngineScope enter{rt};

    rt->registerClass(StaticBind);

    auto exist = rt->eval("Test !== undefined");
    REQUIRE(exist.isBoolean());
    REQUIRE(exist.asBoolean().getValue() == true);

    auto isFunc = rt->eval("typeof Test === 'function'");
    REQUIRE(isFunc.isBoolean());
    REQUIRE(isFunc.asBoolean().getValue() == true);

    // Static classes do not allow new
    REQUIRE_THROWS_MATCHES(
        rt->eval("new Test();"),
        v8wrap::Exception,
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
            v8wrap::Exception,
            Catch::Matchers::ExceptionMessageMatcher(
                "Uncaught TypeError: Native property have only one getter, and "
                "you cannot modify native property without getters"
            )
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


class Actor {
    static int genID() {
        static int id = 0;
        return id++;
    }

public:
    int id_;

    Actor() : id_(genID()) { std::cout << "Actor::Actor() & id = " << id_ << std::endl; }
    virtual ~Actor() { std::cout << "Actor::~Actor() & id = " << id_ << std::endl; }

    virtual std::string getTypeName() const { return "Actor"; }

    virtual int getID() const { return id_; }

    static std::string foo() { return "Actor::foo()"; }
    static std::string bar; // static member
};
std::string Actor::bar = "Actor::bar";

class UUID {
public:
    std::string str_id_{};

    UUID() = default;
    UUID(std::string uuid) : str_id_(std::move(uuid)) {}

    std::string const& getUUID() const { return str_id_; }

    void setUUID(std::string uuid) { this->str_id_ = std::move(uuid); }
};

class Player : public Actor {
public:
    std::string name_{};
    UUID        uuid_{};

    Player(std::string name) : name_(std::move(name)) {}

    std::string getTypeName() const override { return "Player"; }

    std::string const& getName() const { return name_; }

    void setName(std::string name) { this->name_ = std::move(name); }

    UUID const& getUUID() const { return uuid_; }

    void setUUID(UUID uuid) { this->uuid_ = std::move(uuid); }
};


v8wrap::bind::meta::ClassDefine UUIDBind = v8wrap::bind::defineClass<UUID>("UUID")
                                               .constructor<std::string>()
                                               .instanceProperty("str_id_", &UUID::str_id_)
                                               .instanceMethod("getUUID", &UUID::getUUID)
                                               .instanceMethod("setUUID", &UUID::setUUID)
                                               .build();


v8wrap::bind::meta::ClassDefine ActorBind = v8wrap::bind::defineClass<Actor>("Actor")
                                                .disableConstructor()
                                                .instanceProperty("id_", &Actor::id_)
                                                .instanceMethod("getTypeName", &Actor::getTypeName)
                                                .instanceMethod("getID", &Actor::getID)
                                                .function("foo", &Actor::foo)
                                                .property("bar", &Actor::bar)
                                                .build();

v8wrap::bind::meta::ClassDefine PlayerBind =
    v8wrap::bind::defineClass<Player>("Player")
        .customConstructor([](v8wrap::Arguments const& args) -> void* {
            if (args.length() != 1) {
                return nullptr;
            }
            if (!args[0].isString()) {
                return nullptr;
            }
            return new Player(args[0].asString().getValue());
        })
        .instanceProperty("name_", &Player::name_)
        .instanceProperty(
            "uuid_",
            /* ! Note: Dangerous behavior. Attention should be paid to the lifecycle of nested objects. */
            [](void* inst, v8wrap::Arguments const& args) -> v8wrap::Local<v8wrap::Value> {
                auto typed = static_cast<Player*>(inst);
                /*
                 ! For custom types, it is generally not recommended to specialize them into TypeConverter.
                 ! If specializing into TypeConverter, memory safety must be ensured.
                 ! Direct use of newInstanceOf is not allowed here, as it will treat members as heap memory and cause
                    crashes during garbage collection.
                 ! Therefore, it is necessary to create instances using the view mode here.
                 ! The introduction of view instances is to ensure safety and avoid semantic fragmentation caused by
                    returning copies.
                 */
                //! return rt.newInstanceOf(UUIDBind, &typed->uuid_); // !!!dangerous!!!
                return args.runtime()->newInstanceOfView(UUIDBind, &typed->uuid_, args.thiz());
            },
            [](void* inst, v8wrap::Arguments const& args) -> void {
                auto typed = static_cast<Player*>(inst);
                auto value = args[0];
                if (!value.isObject()) {
                    return;
                }
                if (args.runtime()->isInstanceOf(value.asObject(), UUIDBind)) {
                    throw v8wrap::Exception(
                        "Failed to set property, invalid parameter type.",
                        v8wrap::Exception::Type::TypeError
                    );
                }
                auto uuid    = args.runtime()->getNativeInstanceOf<UUID>(value.asObject());
                typed->uuid_ = *uuid;
            }
        )
        .instanceMethod("getTypeName", &Player::getTypeName)
        .instanceMethod("getName", &Player::getName)
        .instanceMethod("setName", &Player::setName)
        .instanceMethod(
            "getUUID",
            [](void* inst, v8wrap::Arguments const& args) -> v8wrap::Local<v8wrap::Value> {
                auto  typed = static_cast<Player*>(inst);
                auto& uuid  = typed->getUUID();
                return args.runtime()->newInstanceOfView(UUIDBind, const_cast<UUID*>(&uuid), args.thiz());
            }
        )
        .instanceMethod(
            "setUUID",
            [](void* inst, v8wrap::Arguments const& args) -> v8wrap::Local<v8wrap::Value> {
                auto typed = static_cast<Player*>(inst);
                if (args.length() != 1) {
                    throw v8wrap::Exception("Wrong arguments count", v8wrap::Exception::Type::SyntaxError);
                }
                auto runtime = args.runtime();
                if (!args[0].isObject() || !runtime->isInstanceOf(args[0].asObject(), UUIDBind)) {
                    throw v8wrap::Exception("Wrong argument type", v8wrap::Exception::Type::TypeError);
                }
                auto uuid = args.runtime()->getNativeInstanceOf<UUID>(args[0].asObject());
                typed->setUUID(*uuid);
                return {}; // void -> undefined
            }
        )
        .extends(ActorBind)
        .build();


TEST_CASE_METHOD(BindingTestFixture, "Instance binding") {
    v8wrap::EngineScope enter{rt};

    REQUIRE_NOTHROW(rt->registerClass(ActorBind));
    REQUIRE_NOTHROW(rt->registerClass(PlayerBind));
    REQUIRE_NOTHROW(rt->registerClass(UUIDBind));

    SECTION("Verify the construction behavior") {
        // Binding construction
        auto uuid = rt->eval("new UUID('abcdef');");
        REQUIRE(uuid.isObject());
        REQUIRE(rt->isInstanceOf(uuid.asObject(), UUIDBind));
        REQUIRE(rt->getNativeInstanceOf<UUID>(uuid.asObject())->str_id_ == "abcdef");

        // Custom construction
        auto player = rt->eval("new Player('John');");
        REQUIRE(player.isObject());
        REQUIRE(rt->isInstanceOf(player.asObject(), PlayerBind));
        REQUIRE(rt->getNativeInstanceOf<Player>(player.asObject())->name_ == "John");

        // Prohibition of construction
        REQUIRE_THROWS_MATCHES(
            rt->eval("new Actor('John');"),
            v8wrap::Exception,
            Catch::Matchers::Message("Uncaught Error: This native class cannot be constructed.")
        );

        // C++ side construction
        auto fn = v8wrap::Function::newFunction([](v8wrap::Arguments const& args) -> v8wrap::Local<v8wrap::Value> {
            REQUIRE(args.length() == 0);
            return v8wrap::EngineScope::currentRuntimeChecked().newInstanceOfRaw(ActorBind, new Actor());
        });
        rt->setVauleToGlobalThis(v8wrap::String::newString("getActor"), fn);
        auto actor = rt->eval("getActor();");
        REQUIRE(actor.isObject());
        REQUIRE(rt->isInstanceOf(actor.asObject(), ActorBind));
    }

    SECTION("Verify prototype properties behavior") {
        // Static properties
        auto actorFoo = rt->eval("Actor.bar;");
        REQUIRE(actorFoo.isString());
        REQUIRE(actorFoo.asString().getValue() == "Actor::bar");

        auto playerFoo = rt->eval("Player.bar;");
        REQUIRE(playerFoo.isString());
        REQUIRE(playerFoo.asString().getValue() == "Actor::bar");

        // Instance properties
        auto playerName = rt->eval("new Player('John').name_;");
        REQUIRE(playerName.isString());
        REQUIRE(playerName.asString().getValue() == "John");

        //! Special case: the member is a native class
        auto playerUUID = rt->eval("new Player('John').uuid_;");
        REQUIRE(playerUUID.isObject());
        REQUIRE(rt->isInstanceOf(playerUUID.asObject(), UUIDBind));
    }

    SECTION("Verify method behavior") {
        rt->eval("globalThis.player = new Player('John');");

        auto typeName = rt->eval("player.getTypeName();");
        REQUIRE(typeName.isString());
        REQUIRE(typeName.asString().getValue() == "Player");

        auto name = rt->eval("player.getName();");
        REQUIRE(name.isString());
        REQUIRE(name.asString().getValue() == "John");

        // Call the base class method
        auto id = rt->eval("player.getID();");
        REQUIRE(id.isNumber());
        REQUIRE(id.asNumber().getInt32() == 4);

        REQUIRE_NOTHROW(rt->eval("player.setUUID(new UUID('crash'));"));
        auto player = rt->eval("player;");
        REQUIRE(player.isObject());
        REQUIRE(rt->getNativeInstanceOf<Player>(player.asObject())->uuid_.str_id_ == "crash");
    }

    SECTION("Verify inheritability") {
        rt->eval(R"(
            class MyUUID extends UUID {
                constructor(id) {
                    super(id);
                }
            }
        )");

        auto myUUID = rt->eval("new MyUUID('A3.1415926535');");
        REQUIRE(myUUID.isObject());
        REQUIRE(rt->isInstanceOf(myUUID.asObject(), UUIDBind));
        REQUIRE(rt->getNativeInstanceOf<UUID>(myUUID.asObject())->str_id_ == "A3.1415926535");
    }
}