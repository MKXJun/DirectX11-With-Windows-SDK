target("ImGui")
    set_group("ImGui")
    set_kind("static")
    add_headerfiles("**.h")
    add_files("**.cpp")
    add_includedirs("./",{public = true})
    -- if is_plat("windows") then
    --     add_defines("IMGUI_API=__declspec(dllexport)")
    -- end
target_end()