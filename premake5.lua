

workspace "VulkanTest"
    configurations { "Debug", "Release" }

project "VulkanTest"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    targetdir "bin/%{cfg.buildcfg}"

    files { "inc/**.h", "inc/**.hpp", "src/**.cpp", "src/**.m" }
    includedirs { "inc", "${VULKAN_SDK}/include", "/Library/Frameworks/SDL2.framework/Headers" }
    libdirs { "${VULKAN_SDK}/lib" }

    links { "vulkan", "MoltenVK", "SDL2.framework"  }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter { "system:macosx", "configurations:Debug" }
        buildoptions { "-fsanitize=address"
                    , "-fsanitize-address-use-after-scope"
                    , "-fno-omit-frame-pointer"
                    , "-fno-optimize-sibling-calls" }
        linkoptions { "-fsanitize=address" }

