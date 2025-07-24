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


target("v8wrap")
    set_kind("static")
    add_files("src/**.cc")
    add_includedirs("src", "include")
    add_headerfiles("include/(v8wrap/**.hpp)")
    set_languages("cxx20")
    set_symbols("debug")

    if is_plat("windows") then 
        add_cxflags("/utf-8", "/W4", "/sdl")
    elseif is_plat("linux") then
        add_cxflags("-fPIC", "-stdlib=libc++", {force = true})
        add_syslinks("dl", "pthread")
    end

    if has_config("v8_include_dir") then
        local v8_inc = get_config("v8_include_dir")
        if v8_inc ~= "" and os.isdir(v8_inc) then
            add_includedirs(v8_inc)
            print("V8 include path added: " .. v8_inc)
        else
            print("Warning: v8_include_dir is invalid or not set! V8 headers will not be available.")
        end
    end

    if is_mode("debug") then
        add_defines("V8WRAP_DEBUG")
    end
target_end()


target("v8wrap_test")
    add_deps("v8wrap")
    set_kind("binary")
    set_symbols("debug")
    set_languages("cxx20")
    add_includedirs("include") -- add v8wrap include dir
    add_files("test/**.cc")

    -- 确保Catch2库正确链接
    if is_plat("linux") then
        -- 在Linux上，需要明确指定链接顺序
        add_packages("catch2", {links = {"Catch2", "Catch2Main"}})
    else
        add_packages("catch2")
    end

    if is_plat("windows") then 
        add_cxflags("/utf-8", "/W4", "/sdl")
        add_syslinks("winmm", "advapi32") -- add required system libraries
    elseif is_plat("linux") then
        add_cxflags("-fPIC", "-stdlib=libc++", {force = true})
        add_ldflags("-stdlib=libc++", {force = true})
        add_syslinks("dl", "pthread", "c++", "m")
    end

    -- v8 headers and libs
    add_includedirs(get_config("v8_include_dir"))

    -- 直接添加静态库文件而不是使用add_links
    if is_plat("linux") then
        -- 在Linux上，链接顺序很重要，先链接应用程序，再链接库
        add_ldflags("-Wl,--whole-archive", get_config("v8_static_lib"), "-Wl,--no-whole-archive", {force = true})
        add_syslinks("dl", "pthread", "c++", "m")
    else
        add_links(get_config("v8_static_lib"))
    end

    after_build(function (target)
        local binDir = os.projectdir() .. "/bin"
        if not os.isdir(binDir) then
            os.mkdir(binDir)
        end
        local test = target:targetfile()
        os.cp(test, binDir)
    end)
