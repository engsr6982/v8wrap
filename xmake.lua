add_rules("mode.debug", "mode.release")

if is_plat("windows") then
    if not has_config("vs_runtime") then
        set_runtimes("MD")
    end
elseif is_plat("linux") then 
    set_toolchains("clang")
end

option("test")
    set_default(false)
    set_showmenu(true)
option_end()


target("v8bind")
    set_kind("static")
    add_includedirs("src")
    add_files("src/**.cc")
    set_languages("cxx20")
    set_symbols("debug")

    add_defines(
        "NOMINMAX",
        "UNICODE",
        "_AMD64_"
    )

    if is_plat("windows") then 
        add_cxflags(
            "/utf-8",
            "/W4",
            "/sdl"
        )
    elseif is_plat("linux") then
        add_cxflags(
            "-fPIC",
            "-stdlib=libc++",
            "-fdeclspec",
            {force = true}
        )
        add_ldflags(
            "-stdlib=libc++",
            {force = true}
        )
        add_syslinks("dl", "pthread", "c++", "c++abi")
    end

    if is_mode("debug") then
        add_defines("V8BIND_DEBUG")
    end

    if has_config("test") then
        add_files("test/**.cc")
    end