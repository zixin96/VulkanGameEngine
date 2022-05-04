#pragma once

// #include <vulkan/vulkan.h>
// GLFW will include vulkan header with it
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <vector>

// this struct represents all the queue families we need
struct QueueFamilyIndices
{
	// C++17 std::optional: graphicsFamily has "no value" until you assign a value to it
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	// return true if both graphics and present queue families are supported
	bool isComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

// this struct represents features that our swap chain support
struct SwapChainSupportDetails
{
	// Basic surface capabilities (min/max number of images in swap chain, min/max width and height of images)
	VkSurfaceCapabilitiesKHR capabilities;
	// Surface formats (pixel format, color space)
	std::vector<VkSurfaceFormatKHR> formats;
	// Available presentation modes
	std::vector<VkPresentModeKHR> presentModes;
};

/**
 * This class stores the Vulkan objects as private class members
 * and has functions to initiate each of them
 *
 * Error handling: If any kind of fatal error occurs during execution
 * then we'll throw a std::runtime_error exception with a descriptive message,
 * which will propagate back to the main functions
 *
 * Resource management: We choose to be explicit about allocation and deallocation of Vulkan objects
 * More complex vulkan application should use RAII model.
 * Vulkan objects creation: {vkCreateXXX, vkAllocateXXX}
 * Vulkan objects deletion: {vkDestroyXXX, vkFreeXXX}
 * PS: These functions share a common parameter pAllocator, which specifies callbacks for a custom memory allocator
 * In this tutorial, pAllocator is always nullptr
 */
class HelloTriangleApplication
{
public:
	void run();
private:
	void initWindow();

	void initVulkan();

	void mainLoop();

	void cleanup();
private:
	// The very first thing to initialize must be an instance, which is the connection between your application and the Vulkan library
	void createInstance();

	// return true if all extensions specified in instanceExtensions are supported by our vulkan instance
	bool checkInstanceExtensionsSupport();

	// return true if all layers specified in validationLayers are supported 
	bool checkValidationLayerSupport();

private:
	void setupDebugMessenger();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
private:
	// Window surface creation
	void createSurface();
private:
	// pick the first suitable device available to us
	void pickPhysicalDevice();

	// the device is suitable if
	// 1. it is a discrete GPU
	// 2. supports geometry shaders
	// 3. has a graphics queue family and a presentation queue
	// 4. supports swap chain 
	bool isDeviceSuitable(VkPhysicalDevice device);

	// look for all the queue families we need
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	// query what features do our swap chain support
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	// the following 3 functions choose the right settings for the swap chain
	// color depth
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	// conditions for "swapping" images to the screen
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	// resolution of images in swap chain
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	// return true if all extensions specified in deviceExtensions are supported by the physical device
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
private:
	void createLogicalDevice();
private:
	void createSwapChain();
	void createImageViews();
private:
	void createRenderPass();
private:
	void createGraphicsPipeline();
	// take a buffer with the bytecode as parameter and create a VkShaderModule from it
	// VkShaderModule is just a thin wrapper around the shader bytecode
	VkShaderModule createShaderModule(const std::vector<char>& code);
private:
	void createFramebuffers();
private:
	void createCommandPool();
	void createCommandBuffer();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
private:
	void createSyncObjects();
private:
	void drawFrame();
private:
	GLFWwindow* window;
	// instance is the connection between your application and the Vulkan library
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	// logical device is used to interface with the physical device
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	VkRenderPass renderPass;
	// for uniforms
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	// signal that an image has been acquired from the swapchain and is ready for rendering
	VkSemaphore imageAvailableSemaphore;
	// signal that rendering has finished and presentation can happen
	VkSemaphore renderFinishedSemaphore;
	// make sure only one frame is rendering at a time 
	VkFence inFlightFence;
};