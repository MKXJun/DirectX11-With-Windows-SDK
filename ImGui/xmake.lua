target("ImGui")
    set_group("ImGui")
    set_kind("static")
    if is_mode("debug") then
        set_targetdir(path.join(os.projectdir(),"bin/Debug/ImGui"))
    else 
        set_targetdir(path.join(os.projectdir(),"bin/Release/ImGui"))
    end
    add_headerfiles("**.h")
    add_files("**.cpp")
    add_includedirs("./",{public = true})
target_end()