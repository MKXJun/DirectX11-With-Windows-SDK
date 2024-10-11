if is_mode("debug") then
    binDir = path.join(os.projectdir(),"bin/Debug/Archive")
else 
    binDir = path.join(os.projectdir(),"bin/Release/Archive")
end
includes("Mouse and Keyboard")