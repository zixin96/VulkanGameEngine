#pragma once

// GLFW will #include <vulkan/vulkan.h> with it
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

class HelloTriangleApplication
{
public:
	void run();
private:
	void initWindow();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	void initVulkan();
	void mainLoop();
	// TODO: how do I know which objects should be cleaned up? 
	void cleanup();
private:
	void createInstance();
	bool checkInstanceExtensionsSupport();
	bool checkValidationLayerSupport();
private:
	// TODO: how to use vk_layer_settings.txt file to configure the layers? Why do we need this? 
	void setupDebugMessenger();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
private:
	void createSurface();
private:
	void pickPhysicalDevice();
	int rateDeviceSuitability(VkPhysicalDevice device);
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
	void cleanupSwapChain();
	void recreateSwapChain();
	void createSwapChain();
	void createImageViews();
private:
	void createRenderPass();
private:
	void createDescriptorSetLayout();
private:
	void createGraphicsPipeline();
	// take a buffer with the bytecode as parameter and create a VkShaderModule from it
	// VkShaderModule is just a thin wrapper around the shader bytecode
	VkShaderModule createShaderModule(const std::vector<char>& code);
private:
	void createFramebuffers();
private:
	void createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory);
	void createVertexBuffer();
	void createIndexBuffer();
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	// find the right type of memory to use based on the requirements of the buffer and our own application
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void createUniformBuffers();
	void updateUniformBuffer(uint32_t currentImage);
	void createDescriptorPool();
	void createDescriptorSets();
private:
	void createCommandPool();
	void createCommandBuffers();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
private:
	void createSyncObjects();
private:
	void drawFrame();
private:
	GLFWwindow* window;
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
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	// signal that an image has been acquired from the swapchain and is ready for rendering
	std::vector<VkSemaphore> imageAvailableSemaphores;
	// signal that rendering has finished and presentation can happen
	std::vector<VkSemaphore> renderFinishedSemaphores;
	// make sure only one frame is rendering at a time 
	std::vector<VkFence> inFlightFences;
	bool framebufferResized = false;

	uint32_t currentFrame = 0;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
};
