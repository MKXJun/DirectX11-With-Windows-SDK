if is_mode("debug") then
    binDir = path.join(os.projectdir(),"bin/Debug/Project 01-09")
else 
    binDir = path.join(os.projectdir(),"bin/Release/Project 01-09")
end
includes("01 DirectX11 Initialization")
includes("02 Rendering a Triangle")
includes("03 Rendering a Cube")
includes("06 Use ImGui")
includes("07 Lighting")
includes("08 Direct2D and Direct3D Interoperability")
includes("09 Texture Mapping")