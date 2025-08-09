add_rules("mode.debug", "mode.release")
set_project("v8wrap")
set_version("0.1.0")

if has_config("test") then
    add_requires("catch2 v3.8.1")
end

if is_plat("windows") then
    if not has_config("vs_runtime") then
        set_runtimes("MT")
    end
    -- set_toolchains("clang-cl")
elseif is_plat("linux") then 
    set_toolchains("clang")
end

option("v8_include_dir")
    set_default("")
    set_showmenu(true)
    set_description("Path to V8 include directory (e.g., /usr/local/include/v8)")
option_end()

option("test")
    set_default(false)
    set_showmenu(true)
option_end()

option("v8_static_lib")
    set_default("")
    set_showmenu(true)
    set_description("Path to V8 static library (e.g., /usr/local/lib/libv8_base.a)")
option_end()


includes("xmake/v8wrap.lua")
includes("xmake/test.lua")
