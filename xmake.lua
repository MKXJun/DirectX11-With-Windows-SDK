set_project("DirectX11 With Windows SDK")
set_xmakever("2.9.0")

option("WIN7_SYSTEM_SUPPORT")
    set_default(false)
    set_description("Windows7 users need to select this option!")
option_end()


add_rules("mode.debug", "mode.release")
set_defaultmode("debug")
set_toolchains("msvc")
set_languages("c99", "cxx17")
set_encodings("utf-8")

if is_os("windows") then 
    add_defines("UNICODE")
    add_defines("_UNICODE")
end

function add_dx_sdk_options()
    if has_config("WIN7_SYSTEM_SUPPORT") then
        add_defines("_WIN32_WINNT=0x601")
    end
    add_syslinks("d3d11","dxgi","dxguid","D3DCompiler","d2d1","dwrite","winmm","user32","gdi32","ole32")
end

includes("scripts.lua")
--ImGui
includes("ImGui")
-- Assimp
add_requires("assimp",{system = false })

includes("Project 01-09")
includes("Project 10-17")
includes("Project 19-")
includes("Project Archive")
