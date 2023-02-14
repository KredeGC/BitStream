local p = premake

if not p.modules.test then
    p.modules.test = {}

    newaction {
        trigger = "test",
        description = "Runs tests using the given config",
        execute = function()
            print("Testing")
            
            local res,msg,sig;
            
            local architecture = _OPTIONS["architecture"];
            if (architecture == "x64") then
                architecture = "x86_64"
            end
            
            if (os.host() == "windows") then
                res,msg,sig = os.execute("bin\\".._OPTIONS["config"].."-windows-"..architecture.."\\Test.exe")
            else
                res,msg,sig = os.execute("bin/".._OPTIONS["config"].."-linux-"..architecture.."/Test")
            end
            
            if (not res and msg == "exit") then
                error("Test "..msg.." with code: "..sig, 0)
            end
        end
    }
end

return p.modules.test