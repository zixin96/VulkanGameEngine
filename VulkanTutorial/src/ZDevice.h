#pragma once
#include "ZWindow.h"

#include <vector>
#include <optional>

namespace ZZX
{
	// this struct represents all the queue families we need
	struct QueueFamilyIndices
	{
		// these uint32_t represent indices into the array of queue families
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

	class ZDevice
	{
	public:
		// Disable validation layer on release mode
#ifdef NDEBUG
		const bool m_enableValidationLayers = false;
#else
		const bool m_enableValidationLayers = true;
#endif

		ZDevice(ZWindow& window);
		~ZDevice();

		// Not copyable or movable
		ZDevice(const ZDevice&) = delete;
		ZDevice& operator=(const ZDevice&) = delete;
		ZDevice(ZDevice&&) = delete;
		ZDevice& operator=(ZDevice&&) = delete;

		VkCommandPool getCommandPool() { return m_commandPool; }
		VkDevice device() { return m_device; }
		VkSurfaceKHR surface() { return m_surface; }
		VkQueue graphicsQueue() { return m_graphicsQueue; }
		VkQueue presentQueue() { return m_presentQueue; }

		SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(m_physicalDevice); }
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(m_physicalDevice); }
		VkFormat findSupportedFormat(
			const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		// Buffer Helper Functions
		void createBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory);
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void copyBufferToImage(
			VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);

		void createImageWithInfo(
			const VkImageCreateInfo& imageInfo,
			VkMemoryPropertyFlags properties,
			VkImage& image,
			VkDeviceMemory& imageMemory);

		VkPhysicalDeviceProperties m_properties;

	private:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createCommandPool();

		// helper functions
		int rateDeviceSuitability(VkPhysicalDevice device);
		bool checkValidationLayerSupport();
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		bool checkInstanceExtensionsSupport();
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		ZWindow& m_window;
		VkCommandPool m_commandPool;

		VkDevice m_device;
		VkSurfaceKHR m_surface;
		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue;

		const std::vector<const char*> m_validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
		const std::vector<const char*> m_deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		const std::vector<const char*> m_instanceExtensions = {
			// this extension provides a VkSurfaceKHR handle, an object representing an output for presentation
			"VK_KHR_surface",
			// this extension provides platform-specific HWND and HMODULE handles on Windows for surface creation
			"VK_KHR_win32_surface",
		#ifdef NDEBUG
		#else
			// this extension provides debug message callback
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		#endif
		};
	};
};
