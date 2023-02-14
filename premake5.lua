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
    architecture "x64"
    toolset(_OPTIONS["toolset"])
    startproject "Test"
    
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
        "test/src/**.cpp",
        "test/include/**.h",
        "include/**.h"
    }

    includedirs {
        "test/include",
        "include"
    }

    -- OS
    filter "system:windows"
        systemversion "latest"
        
        conformancemode "on"
        
        flags { "MultiProcessorCompile" }

    filter "system:linux"
        systemversion "latest"
    
    -- Config
    filter "configurations:Debug"
        defines { "BS_DEBUG_BREAK" }
        
        runtime "Debug"
        symbols "on"
        
    filter "configurations:Release"
        flags { "LinkTimeOptimization" }

        runtime "Release"
        optimize "on"