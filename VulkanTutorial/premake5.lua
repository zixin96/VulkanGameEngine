project "VulkanTutorial"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"
	
	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp",
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


	filter "configurations:Debug"
		defines "GLCORE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "GLCORE_RELEASE"
		runtime "Release"
        optimize "on"
