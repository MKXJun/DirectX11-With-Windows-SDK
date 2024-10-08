targetName = "29_OIT"
target(targetName)
    set_group("Project 19-")
    set_kind("binary")
    set_targetdir(path.join(binDir,targetName))
    add_rules("imguiini")
    add_dx_sdk_options()
    add_deps("Common")
    add_headerfiles("**.h")
    add_files("**.cpp")
    -- shader
    add_rules("hlsl_shader_complier")
    add_headerfiles("Shaders/**.hlsl|Shaders/**.hlsli")
    add_files("Shaders/**.hlsl|Shaders/**.hlsli")
    -- assert
    add_rules("asset_file")
target_end() 