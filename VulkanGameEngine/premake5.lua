project "VulkanGameEngine"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	
	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "src/pch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
		"vendor/includes/tinyobjloader/tiny_obj_loader.cpp"
	}

	libdirs
	{
		"vendor/libs"
	}

	includedirs
	{
		"src",
		"vendor/includes"
	}

	links 
	{
		"glfw3.lib", "vulkan-1.lib"
	}

	filter "system:windows"
		systemversion "latest"

	prebuildcommands {
		"assets/shaders/compile.bat"
	}

	filter "configurations:Debug"
		defines "GLCORE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "GLCORE_RELEASE"
		runtime "Release"
        optimize "on"
