#pragma once
#include "ZWindow.h"

namespace ZZX
{
	// this struct represents all the queue families we need
	struct QueueFamilyIndices
	{
		// these uint32_t represent indices into the array of queue families
		// C++17 std::optional is used to distinguish between the case of a value being assigned or not

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
#ifdef GLCORE_RELEASE
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

		VkCommandPool getCommandPool() { return m_VkCommandPool; }
		VkDevice device() { return m_VkDevice; }
		VkSurfaceKHR surface() { return m_VkSurfaceKHR; }
		VkQueue graphicsQueue() { return m_VkGraphicsQueue; }
		VkQueue presentQueue() { return m_VkPresentQueue; }
		ZWindow& getZWindow() const { return m_ZWindow; }

		SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(m_VkPhysicalDevice); }
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilyIndices(m_VkPhysicalDevice); }
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
		std::tuple<int, std::string> rateDeviceSuitability(VkPhysicalDevice device);
		bool checkValidationLayerSupport();
		QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
		bool checkInstanceExtensionsSupport();
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		VkInstance m_VkInstance;
		VkDebugUtilsMessengerEXT m_VkDebugUtilsMessengerEXT;

		// the graphics card that supports the features we need
		VkPhysicalDevice m_VkPhysicalDevice = VK_NULL_HANDLE;

		ZWindow& m_ZWindow;
		VkCommandPool m_VkCommandPool;

		VkDevice m_VkDevice;
		VkSurfaceKHR m_VkSurfaceKHR;
		VkQueue m_VkGraphicsQueue;
		VkQueue m_VkPresentQueue;

		const std::vector<const char*> m_validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
		const std::vector<const char*> m_deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		std::vector<const char*> m_instanceExtensions;
	};
};
