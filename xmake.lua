add_rules("mode.debug", "mode.release")
set_project("v8wrap")
set_version("0.1.0")

if has_config("test") then
    add_requires("catch2 v3.8.1")
end

if is_plat("windows") then
    if not has_config("vs_runtime") then
        set_runtimes("MD")
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

    if has_config("test") then
        add_defines("V8WRAP_TEST")
        add_files("test/**.cc")
        add_packages("catch2")
    end

    after_build(function(target)
        local binDir = os.projectdir() .. "/bin"
        if not os.isdir(binDir) then
            os.mkdir(binDir)
        end
        local targetBin = binDir .. "/" .. target:name()
        if not os.exists(targetBin) then
            os.mkdir(targetBin)
        end

        local lib = target:targetfile()
        os.cp(lib, targetBin)
        local pdb = lib:gsub(".lib", ".pdb")
        os.trycp(pdb, targetBin)
    end)