set_project("DirectX11 With Windows SDK")
set_xmakever("2.9.0")

option("WIN7_SYSTEM_SUPPORT")
    set_default(false)
    set_description("Windows7 users need to select this option!")
option_end()


add_rules("mode.debug", "mode.release", "mode.releasedbg")
set_toolchains("msvc")
set_languages("c99", "cxx17")
set_encodings("utf-8")

if is_os("windows") then 
    -- add_defines("_WINDOWS")
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
add_requires("assimp",{system=false})
--Assimp
-- package("assimp")
--     add_deps("cmake")
--     set_sourcedir(path.join(os.scriptdir(), "assimp"))
--     on_install(function (package)
--         local configs = {"-DASSIMP_BUILD_ZLIB=ON",
--                          "-DASSIMP_BUILD_ASSIMP_TOOLS=OFF",
--                          "-DASSIMP_BUILD_TESTS=OFF",
--                          "-DASSIMP_INSTALL_PDB=OFF",
--                          "-DASSIMP_INJECT_DEBUG_POSTFIX=OFF",
--                          }
--         table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
--         import("package.tools.cmake").install(package, configs)
--     end)
--     add_includedirs("assimp/include", {public =true})
-- package_end()

includes("Project 01-09")
includes("Project 10-17")
includes("Project 19-")
-- includes("Project Archive")
