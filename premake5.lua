workspace "VulkanGameEngine"
	architecture "x64"
	startproject "VulkanGameEngine"

	configurations
	{
		"Debug",
		"Release"
	}
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "VulkanGameEngine"