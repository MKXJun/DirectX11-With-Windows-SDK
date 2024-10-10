target("Common")
    set_group(GroupName)
    set_kind("static")
    if is_mode("debug") then
        set_targetdir(path.join(os.projectdir(),"bin/Debug/Common"))
    else 
        set_targetdir(path.join(os.projectdir(),"bin/Release/Common"))
    end
    add_deps("ImGui")
    add_packages("assimp")
    add_headerfiles("**.h")
    add_files("**.cpp")
    add_includedirs("./",{public = true})
target_end()
