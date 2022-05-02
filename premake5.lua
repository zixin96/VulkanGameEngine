workspace "VulkanTutorial"
	architecture "x64"
	startproject "VulkanTutorial"

	configurations
	{
		"Debug",
		"Release"
	}
	
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "VulkanTutorial"