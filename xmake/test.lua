
target("v8wrap_test")
    set_default(false)
    add_deps("v8wrap")
    set_kind("binary")
    set_symbols("debug")
    set_languages("cxx20")
    add_includedirs("$(projectdir)/include") -- add v8wrap include dir
    add_files("$(projectdir)/test/**.cc")
    set_symbols("debug")

    if is_plat("linux") then
        add_packages("catch2", {links = {"Catch2", "Catch2Main"}})
    else
        add_packages("catch2")
    end

    if is_plat("windows") then 
        add_cxflags("/utf-8", "/W4", "/sdl")
        add_syslinks("winmm", "advapi32", "dbghelp") -- add required system libraries
    elseif is_plat("linux") then
        add_cxflags("-fPIC", "-stdlib=libc++", {force = true})
        add_ldflags("-stdlib=libc++", {force = true})
        add_syslinks("dl", "pthread", "c++", "m")
    end

    -- v8 headers and libs
    add_includedirs(get_config("v8_include_dir"))

    if is_plat("linux") then
        -- TODO: fix linking issue on Linux
        add_ldflags("-Wl,--whole-archive", get_config("v8_static_lib"), "-Wl,--no-whole-archive", {force = true})
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
        add_packages("libelf")
        add_syslinks("dl", "pthread", "c++", "c++abi")
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
