targetName = "06_Use_ImGui"
target(targetName)
    set_group("Project 01-09")
    set_kind("binary")
    set_targetdir(path.join(binDir,targetName))
    add_dx_sdk_options()
    add_deps("ImGui")
    add_headerfiles("**.h")
    add_files("**.cpp")
    --shader
    add_rules("hlsl_shader_complier")
    add_headerfiles("HLSL/**.hlsl|HLSL/**.hlsli")
    add_files("HLSL/**.hlsl|HLSL/**.hlsli")
target_end()