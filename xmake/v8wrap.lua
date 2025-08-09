target("v8wrap")
    set_kind("static")
    add_files("$(projectdir)/src/**.cc")
    add_includedirs("$(projectdir)/src", "$(projectdir)/include")
    add_headerfiles("$(projectdir)/include/(v8wrap/**.hpp)")
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
        end
    end

    if is_mode("debug") then
        add_defines("V8WRAP_DEBUG")
    end
target_end()