targetName = "01_DirectX11_Initialization"
target(targetName)
    set_group("Project 01-09")
    set_kind("binary")
    set_targetdir(path.join(binDir,targetName))
    add_dx_sdk_options()
    add_headerfiles("**.h")
    add_files("**.cpp")
target_end()