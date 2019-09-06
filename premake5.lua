

workspace "VulkanTest"
    configurations { "Debug", "Release" }

project "VulkanTest"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    targetdir "bin/%{cfg.buildcfg}"

    files { "inc/**.h", "inc/**.hpp", "src/**.cpp" }
    includedirs { "inc", "${VULKAN_SDK}/include"  }
    libdirs { "${VULKAN_SDK}/lib" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter { "system:linux" }
        links { "vulkan", "SDL2" }

    filter { "system:macosx" }
        links { "MoltenVK", "vulkan.1", "SDL2.framework"  }
        libdirs { "ext/macos/Frameworks" }
        includedirs { "ext/macos/Frameworks/SDL2.framework/headers" }
        linkoptions { "-rpath @executable_path/Frameworks", "-rpath @executable_path/../Frameworks", "-rpath @executable_path/../../ext/macos/Frameworks", "-F ext/macos/Frameworks/" }

    filter { "system:macosx", "configurations:Debug" }
        buildoptions { "-fsanitize=address"
                    , "-fsanitize-address-use-after-scope"
                    , "-fno-omit-frame-pointer"
                    , "-fno-optimize-sibling-calls" }
        linkoptions { "-fsanitize=address" }

