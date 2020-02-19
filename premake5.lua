function os.winSdkVersion()
    local reg_arch = iif( os.is64bit(), "\\Wow6432Node\\", "\\" )
    local sdk_version = os.getWindowsRegistry( "HKLM:SOFTWARE" .. reg_arch .."Microsoft\\Microsoft SDKs\\Windows\\v10.0\\ProductVersion" )
    if sdk_version ~= nil then return sdk_version else return "" end
end

workspace "VulkanWorkspace"
    configurations { "Debug", "Release" }
    platforms      { "x64" }

    filter { "system:windows", "action:vs*"}
        defines { "_SECURE_SCL=0", "_CRT_SECURE_NO_WARNINGS=1" } --, "_ITERATOR_DEBUG_LEVEL=0", "_HAS_ITERATOR_DEBUGGING=0" }
        systemversion(os.winSdkVersion() .. ".0")
        buildoptions { "/std:c++latest", "/permissive-" }
        flags { "MultiProcessorCompile" }
    filter{}

local cwd = os.getcwd()
shader_out_path = cwd .. "/generated"

project "ComputeTest"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    flags { "NoPCH", "StaticRuntime" }    
    targetdir "bin/%{cfg.buildcfg}"

    files { "inc/**.h", "inc/**.hpp", "src/**.cpp", "media/shaders/**.vert", "media/shaders/**.frag" }

    filter { "system:linux or system:macosx" }
        includedirs { "inc", "${VULKAN_SDK}/include", "lib/tinyobjloader", "lib/stb", "lib/glm" }
        libdirs { "${VULKAN_SDK}/lib" }

    filter { "system:windows" }
        includedirs { "inc", "lib/tinyobjloader", "lib/stb", "lib/glm", "lib/SDL2-2.0.10/include" , "%VULKAN_SDK%/Include" }
        libdirs { "%VULKAN_SDK%/lib", "lib/SDL2-2.0.10/lib/x64" }

    filter { "system:linux or system:macosx" }
        toolset "clang"

    filter { "system:linux or system:macosx", "files:media/shaders/**.vert or files:media/shaders/**.frag" }
        buildmessage "Compiling shader %{file.name}"
        buildcommands {
            "glslangValidator -e main -o %{shader_out_path}/%{file.name}.spirv -DVK=1 -V %{file.relpath}"
        }
        buildoutputs {
            "%{shader_out_path}/%{file.name}.spirv"
        }

    filter { "system:windows", "files:media/shaders/**.vert or files:media/shaders/**.frag" }
        buildmessage "Compiling shader %{file.name}"
        buildcommands {
            "glslangValidator.exe -e main -o %{shader_out_path}/%{file.name}.spirv -DVK=1 -V %{file.relpath}"
        }
        buildoutputs {
            "%{shader_out_path}/%{file.name}.spirv"
        }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter { "system:linux" }
        links { "vulkan", "SDL2" }

    filter { "system:windows" }
        links { "vulkan-1", "SDL2" }

    filter { "system:macosx" }
        links { "MoltenVK", "vulkan.1", "SDL2.framework"  }
        libdirs { "ext/macos/Frameworks" }
        includedirs { "ext/macos/Frameworks/SDL2.framework/headers" }
        linkoptions { "-rpath @executable_path/Frameworks", "-rpath @executable_path/../Frameworks", "-rpath @executable_path/../../ext/macos/Frameworks", "-F ext/macos/Frameworks/" }

    filter { "system:macosx or system:linux", "configurations:Debug" }
        buildoptions { "-fsanitize=address"
                    , "-fsanitize-address-use-after-scope"
                    , "-fno-omit-frame-pointer"
                    , "-fno-optimize-sibling-calls" }
        linkoptions { "-fsanitize=address" }

    filter { "system:macosx or system:linux" }
        buildoptions{ "-Wall", "-Wextra" }
