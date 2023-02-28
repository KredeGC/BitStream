newoption {
    trigger = "toolset",
    value = "Toolset (eg. gcc, clang, msc)",
    description = "The toolset to use to compile with",
    default = os.host() == "windows" and "msc" or "gcc",
    allowed = {
        { "gcc", "gcc and g++ using ld and ar" },
        { "clang", "clang and clang++ using lld and llvm-ar" },
        { "msc", "msbuild and cl using link.exe" }
    }
}

newoption {
    trigger = "dialect",
    value = "Dialect (eg. C++17, C++20)",
    description = "The dialect to use when generating project files",
    default = "C++17",
}

newoption {
    trigger = "architecture",
    value = "Architecture",
    description = "The architecture to target",
    default = "x64",
    allowed = {
        { "x86", "32 bit x86 architecture" },
        { "x64", "64 bit x86 architecture" }
    }
}

newoption {
    trigger = "config",
    value = "Configuration",
    description = "The configuration to compile",
    default = "debug",
    allowed = {
        { "debug", "Debug build with symbols turned on" },
        { "release", "Release build without symbols, but with optimizations" }
    }
}

require "scripts/build"
require "scripts/test"

workspace "BitStream"
    toolset(_OPTIONS["toolset"])
    startproject "Test"
    
    platforms {
        "x86",
        "x64"
    }
    
    configurations {
        "debug",
        "release"
    }
    
    filter "toolset:clang"
        linkoptions { "-fuse-ld=lld" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Test"
    kind "ConsoleApp"
    language "C++"
    cppdialect(_OPTIONS["dialect"])
    staticruntime "off"
    
    targetdir ("bin/%{outputdir}")
    objdir ("bin-obj/%{outputdir}")

    files {
        "src/**.cpp",
        "src/**.h",
        "include/**.h"
    }

    includedirs {
        "src",
        "include"
    }

    -- OS
    filter "system:windows"
        systemversion "latest"
        
        conformancemode "on"
        
        flags { "MultiProcessorCompile" }

    filter "system:linux"
        systemversion "latest"
    
    -- Architecture
    filter "platforms:x86"
        architecture "x86"
    
    filter "platforms:x64"
        architecture "x86_64"
    
    -- Config
    filter "configurations:debug"
        defines { "BS_DEBUG_BREAK" }
        
        runtime "Debug"
        symbols "on"
        
    filter "configurations:release"
        flags { "LinkTimeOptimization" }

        runtime "Release"
        optimize "on"