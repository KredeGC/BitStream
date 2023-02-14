local p = premake

if not p.modules.build then
    p.modules.build = {}

    newaction {
        trigger = "build",
        description = "Builds the project using whatever toolset is chosen",
        onProject = function(prj)
            printf("Building project '%s'", prj.name)
            
            local res,msg,sig;
            
            if (os.host() == "windows") then
                res,msg,sig = os.execute("\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe\" "..prj.location.."\\"..prj.name..".vcxproj -t:Build -verbosity:minimal -p:Configuration=".._OPTIONS["config"].." -p:Platform=".._OPTIONS["architecture"])
            elseif (os.host() == "linux") then
                res,msg,sig = os.execute("make -j3 "..prj.name.." config=".._OPTIONS["config"].." platform=".._OPTIONS["architecture"])
            end
            
            if (not res and msg == "exit") then
                error("Build "..msg.." with code: "..sig, 0)
            end
        end
    }
end

return p.modules.build