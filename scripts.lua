rule("hlsl_shader_complier")
    set_extensions(".hlsl",".hlsli")
    on_buildcmd_files(function (target,batchcmds,sourcebatch, opt)
		import("lib.detect.find_program")
		local msvc = target:toolchain("msvc")
		local runenvs = msvc:runenvs()
		local fxcopt = {}
		fxcopt.envs = runenvs
		fxcopt.force = true
		fxcopt.check   = opt.check or function () end
		fxc = find_program(opt.program or "fxc", fxcopt)

        for _,sourcefile_hlsl in ipairs(sourcebatch.sourcefiles) do 
            local ext = path.extension(sourcefile_hlsl)
            if ext==".hlsl" then 
                local hlsl_basename = path.basename(sourcefile_hlsl)
                local hlsl_relative_file_name = path.relative(sourcefile_hlsl,target:scriptdir())
                local hlsl_relative_path = path.directory(hlsl_relative_file_name)
                local hlsl_output_path = path.join(target:targetdir() ,hlsl_relative_path)
                if not os.isdir(hlsl_output_path) then
                    os.mkdir(hlsl_output_path)
                end
                local hlsl_output_file_name= path.join(hlsl_output_path,hlsl_basename..".cso")
                local hlsl_shader_entrypoint = string.sub(hlsl_basename,-2)
				batchcmds:show_progress(opt.progress, "${color.build.object}compiling.hlsl %s", sourcefile_hlsl)
                if is_mode("debug") then  
					batchcmds:vrunv(fxc,{sourcefile_hlsl,"/Zi","/Od","/E",hlsl_shader_entrypoint,"/Fo",hlsl_output_file_name,"/T",string.lower(hlsl_shader_entrypoint).."_5_0","/nologo"})
                else 
					batchcmds:vrunv(fxc,{sourcefile_hlsl,"/E",hlsl_shader_entrypoint,"/Fo",hlsl_output_file_name,"/T",string.lower(hlsl_shader_entrypoint).."_5_0","/nologo"})
                end 
            end
        end 
    end)
rule_end()

rule("hlsl_shader_copy")
    set_extensions(".hlsl",".hlsli")
    after_build(function (target)
        -- shader files
        if os.exists(path.join(target:scriptdir(),"/Shaders")) then
            os.cp(path.join(target:scriptdir(),"/Shaders"), path.join(target:targetdir(),"/Shaders"))
        end
    end)
rule_end()

rule("imguiini")
    after_build(function (target)
        imguiini_file=path.join(target:scriptdir(),"imgui.ini")
        if os.isfile(imguiini_file) then
            os.cp(imguiini_file,target:targetdir())
        end
    end)
rule_end()

rule("asset_file")
    after_build(function (target)
        if os.exists(path.join(target:scriptdir(),"../Texture")) then
            os.cp(path.join(target:scriptdir(),"../Texture"), path.join(target:targetdir(),"../Texture"))
        end
        if os.exists(path.join(target:scriptdir(),"../Model")) then
            os.cp(path.join(target:scriptdir(),"../Model"), path.join(target:targetdir(),"../Model"))
        end
    end)
rule_end()
