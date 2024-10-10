if is_mode("debug") then
    binDir = path.join(os.projectdir(),"bin/Debug/Project 10-17")
else 
    binDir = path.join(os.projectdir(),"bin/Release/Project 10-17")
end
includes("10 Camera")
includes("11 Blending")
includes("12 Depth and Stenciling")
includes("13 Living Without FX11")
includes("14 Depth Test")
includes("15 Geometry Shader Beginning")
includes("16 Stream Output")
includes("17 Tree Billboard")