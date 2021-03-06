#include "pch.h"
#include "ZDevice.h"

namespace ZZX
{
	// user-defined debug callback function
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                                    void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		// always return VK_FALSE
		return VK_FALSE;
	}

	// create a proxy function that creates the debug messenger for this vulkan instance
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	                                      const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	                                      const VkAllocationCallbacks* pAllocator,
	                                      VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		// look up the address of vkCreateDebugUtilsMessengerEXT function
		// since this function is an extension function, it is not automatically loaded
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}


	// create a proxy function that destroy the debug messenger for this vulkan instance
	void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	                                   VkDebugUtilsMessengerEXT debugMessenger,
	                                   const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	ZDevice::ZDevice(ZWindow& window) : m_ZWindow{window}
	{
		// Instance creation
		createInstance();
		// Debugging support
		setupDebugMessenger();
		// Window surface creation
		createSurface();
		// Physical device and queue families selection
		pickPhysicalDevice();
		// Logical device creation
		createLogicalDevice();
		// Command pool creation
		createCommandPool();
	}

	ZDevice::~ZDevice()
	{
		// this call will destroy both the command pool and any command buffers allocated from this pool
		vkDestroyCommandPool(m_VkDevice, m_VkCommandPool, nullptr);

		// this call will destroy both logical device and device queues
		vkDestroyDevice(m_VkDevice, nullptr);

		if (m_enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(m_VkInstance, m_VkDebugUtilsMessengerEXT, nullptr);
		}

		vkDestroySurfaceKHR(m_VkInstance, m_VkSurfaceKHR, nullptr);

		// this call will destroy both instance and physical device 
		vkDestroyInstance(m_VkInstance, nullptr);
	}

	void ZDevice::createInstance()
	{
		// Before creating an instance, we need to check if the requested validation layers and instance extensions are supported
		if (m_enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}
		if (!checkInstanceExtensionsSupport())
		{
			throw std::runtime_error("extensions requested, but not available!");
		}

		// Before creating an instance, we can optionally provide some info about our application to the driver for potential optimization
		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Hello Triangle",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "No Engine",
			.engineVersion = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion = VK_API_VERSION_1_0,
		};

		// To create an instance, we must provide a createInfo struct that tells the Vulkan driver which *global* extensions and validation layers we want to use
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// The next two layers specify the desired global extensions
		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_instanceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_instanceExtensions.data();

		// it is necessary to place debugCreateInfo outside the if statement
		// to ensure it is not destroyed before vkCreateInstance call
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		// specify the desired global validation layers
		if (m_enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
			createInfo.ppEnabledLayerNames = m_validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			// pass a VkDebugUtilsMessengerCreateInfoEXT struct to pNext field
			// so that we can debug any issues in the vkCreateInstance and vkDestroyInstance calls
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &m_VkInstance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}
	}

	void ZDevice::pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());

		// this map stores score-physicalDevice pair by increasing score
		// multiple GPU could have the same score, thus we use multimap
		std::multimap<int, std::tuple<VkPhysicalDevice, std::string>> candidates;

		// find the first suitable physical device
		for (const auto& device : devices)
		{
			auto [score, deviceName] = rateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, std::make_tuple<>(device, deviceName)));
		}

		std::cout << "Available physical devices:\n";
		for (const auto& device : candidates)
		{
			std::cout << "\tDevice: " << std::get<1>(device.second) << "\n\tScore: " << device.first << '\n' << '\n';
		}

		// Check if the best candidate is suitable at all
		if (candidates.rbegin()->first > 0)
		{
			m_VkPhysicalDevice = std::get<0>(candidates.rbegin()->second);
		}
		else
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	std::tuple<int, std::string> ZDevice::rateDeviceSuitability(VkPhysicalDevice device)
	{
		// check for basic device properties like the name, type and supported Vulkan version
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		// check for optional features like texture compression, 64 bit floats and multi viewport rendering (useful for VR)
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		int score = 0;

		// Discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 1000;
		}

		// Maximum possible size of textures affects graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		// Application can't function without geometry shaders
		if (!deviceFeatures.geometryShader)
		{
			return { 0, deviceProperties.deviceName };
		}

		// check if the physical device has queue families that can process the commands we want to use
		QueueFamilyIndices indices = findQueueFamilyIndices(device);
		// Application can't function without requested queue families
		if (!indices.isComplete())
		{
			return { 0, deviceProperties.deviceName };
		}

		// strongly prefer a physical device that supports graphics and presentation in the same queue (for improved performance)
		if (indices.graphicsFamily.value() == indices.presentFamily.value())
		{
			score += 1000;
		}

		// check if swap chain support is sufficient
		bool swapChainAdequate = false;
		if (checkDeviceExtensionSupport(device))
		{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			// swap chain support is sufficient if there is at least one supported image format and one supported presentation mode given the window surface we have
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		// Application can't function without adequate swap chain support
		if (!swapChainAdequate)
		{
			return { 0, deviceProperties.deviceName };
		}

		return { score, deviceProperties.deviceName };
	}

	void ZDevice::createLogicalDevice()
	{
		QueueFamilyIndices indices = findQueueFamilyIndices(m_VkPhysicalDevice);

		// Specifying the queues to be created
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		// Note: if the present and graphics queue is the same, this set only contains one value
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			// create a single queue for this queue family
			queueCreateInfo.queueCount = 1;
			// this single queue has the highest priority
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		// Specifying used device features (empty for now)
		VkPhysicalDeviceFeatures deviceFeatures{};
		createInfo.pEnabledFeatures = &deviceFeatures;
		// Enabling device extensions
		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
		// Enabling device layers (deprecated)
		if (m_enableValidationLayers)
		{
			// these are ignored by Vulkan b/c device specific validation layer is deprecated
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
			createInfo.ppEnabledLayerNames = m_validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		// Note: the queues are automatically created along with the logical device
		if (vkCreateDevice(m_VkPhysicalDevice, &createInfo, nullptr, &m_VkDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}

		// Retrieving queue handles
		// we only have a single queue from each queue family. Thus, queueIndex is 0
		vkGetDeviceQueue(m_VkDevice, indices.graphicsFamily.value(), 0, &m_VkGraphicsQueue);
		vkGetDeviceQueue(m_VkDevice, indices.presentFamily.value(), 0, &m_VkPresentQueue);
	}

	void ZDevice::createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilyIndices(m_VkPhysicalDevice);

		VkCommandPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			// TODO: Vulkan-tutorial only has RESET bit 
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			// Specify that the command buffers allocated from this command pool will be submitted on the graphics queue
			.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
		};

		if (vkCreateCommandPool(m_VkDevice, &poolInfo, nullptr, &m_VkCommandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void ZDevice::createSurface() { m_ZWindow.createWindowSurface(m_VkInstance, &m_VkSurfaceKHR); }

	void ZDevice::populateDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		// you can configure more settings using vk_layer_settings.txt
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		// Optional: you could pass a pointer to the HelloTriangleApplication class for example
		createInfo.pUserData = nullptr;
	}

	void ZDevice::setupDebugMessenger()
	{
		if (!m_enableValidationLayers) { return; }

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_VkDebugUtilsMessengerEXT) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	bool ZDevice::checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
		std::vector<std::string> availLayerName;
		std::cout << "Available layers:\n";
		for (const auto& layerProperties : availableLayers)
		{
			std::cout << '\t' << layerProperties.layerName << '\n';
		}
		// check if our desired layers are supported 
		std::set<std::string> requiredLayers(m_validationLayers.begin(), m_validationLayers.end());
		for (const auto& layer : availableLayers)
		{
			requiredLayers.erase(layer.layerName);
		}
		return requiredLayers.empty();
	}

	bool ZDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		std::cout << "Available device extensions:\n";
		for (const auto& extension : availableExtensions)
		{
			std::cout << '\t' << extension.extensionName << '\n';
		}
		// check if our desired device extensions are supported 
		std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());
		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();
	}

	bool ZDevice::checkInstanceExtensionsSupport()
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
		std::cout << "Available instance extensions:\n";
		for (const auto& extension : availableExtensions)
		{
			std::cout << '\t' << extension.extensionName << '\n';
		}

		// fill desired extensions
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		// these extensions are requested by GLFW 
		for (uint32_t i = 0; i < glfwExtensionCount; i++)
		{
			m_instanceExtensions.push_back(glfwExtensions[i]);
		}
		if (m_enableValidationLayers)
		{
			// this extension provides debug message callback
			m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		// check if our desired instance extensions are supported
		std::set<std::string> requiredExtensions(m_instanceExtensions.begin(), m_instanceExtensions.end());
		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();
	}

	QueueFamilyIndices ZDevice::findQueueFamilyIndices(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;
		// retrieve supported queue families for this physical device
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilies.size(); i++)
		{
			// find at least one queue family that supports VK_QUEUE_GRAPHICS_BIT
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			// find at least one queue family that supports presentation
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_VkSurfaceKHR, &presentSupport);
			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete())
			{
				// we've found the required family queues, break!
				break;
			}
		}
		return indices;
	}

	SwapChainSupportDetails ZDevice::querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		// Note: all of the swap chain support querying functions have VkPhysicalDevice and VkSurfaceKHR as first two parameters
		// because they are the core components of the swap chain

		// query basic surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_VkSurfaceKHR, &details.capabilities);

		// query the supported surface formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VkSurfaceKHR, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_VkSurfaceKHR, &formatCount, details.formats.data());
		}

		// query the supported presentation modes
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_VkSurfaceKHR, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device,
			                                          m_VkSurfaceKHR,
			                                          &presentModeCount,
			                                          details.presentModes.data());
		}

		return details;
	}

	VkFormat ZDevice::findSupportedFormat(
		const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_VkPhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (
				tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}
		throw std::runtime_error("failed to find supported format!");
	}

	uint32_t ZDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_VkPhysicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void ZDevice::createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(m_VkDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_VkDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(m_VkDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(m_VkDevice, buffer, bufferMemory, 0);
	}

	VkCommandBuffer ZDevice::beginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_VkCommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_VkDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// The command buffer will be rerecorded right after executing it once
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void ZDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_VkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_VkGraphicsQueue);

		vkFreeCommandBuffers(m_VkDevice, m_VkCommandPool, 1, &commandBuffer);
	}

	void ZDevice::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	void ZDevice::copyBufferToImage(
		VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;

		region.imageOffset = {0, 0, 0};
		region.imageExtent = {width, height, 1};

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
		endSingleTimeCommands(commandBuffer);
	}

	void ZDevice::createImageWithInfo(
		const VkImageCreateInfo& imageInfo,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory)
	{
		if (vkCreateImage(m_VkDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(m_VkDevice, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(m_VkDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate image memory!");
		}

		if (vkBindImageMemory(m_VkDevice, image, imageMemory, 0) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to bind image memory!");
		}
	}
}
