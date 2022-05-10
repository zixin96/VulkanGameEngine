#include "pch.h"
#include "ZSwapChain.h"

namespace ZZX
{
	ZSwapChain::ZSwapChain(ZDevice& deviceRef)
		: m_ZDevice{deviceRef}
	{
		init();
	}

	ZSwapChain::ZSwapChain(ZDevice& deviceRef, std::shared_ptr<ZSwapChain> previous)
		: m_ZDevice{deviceRef}, m_oldSwapChain{std::move(previous)}
	{
		init();

		// we will only use the old swap chain during init(), thus after init() it's no longer needed
		// clean up old swap chain since it's no longer needed
		m_oldSwapChain = nullptr;
	}

	void ZSwapChain::init()
	{
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDepthResources();
		createFramebuffers();
		createSyncObjects();
	}

	ZSwapChain::~ZSwapChain()
	{
		for (auto imageView : m_swapChainImageViews)
		{
			vkDestroyImageView(m_ZDevice.device(), imageView, nullptr);
		}
		m_swapChainImageViews.clear();

		if (m_VkSwapchainKHR != nullptr)
		{
			vkDestroySwapchainKHR(m_ZDevice.device(), m_VkSwapchainKHR, nullptr);
			m_VkSwapchainKHR = nullptr;
		}

		for (int i = 0; i < m_depthImages.size(); i++)
		{
			vkDestroyImageView(m_ZDevice.device(), m_depthImageViews[i], nullptr);
			vkDestroyImage(m_ZDevice.device(), m_depthImages[i], nullptr);
			vkFreeMemory(m_ZDevice.device(), m_depthImageMemorys[i], nullptr);
		}

		for (auto framebuffer : m_swapChainFramebuffers)
		{
			vkDestroyFramebuffer(m_ZDevice.device(), framebuffer, nullptr);
		}

		vkDestroyRenderPass(m_ZDevice.device(), m_renderPass, nullptr);

		// cleanup synchronization objects
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_ZDevice.device(), m_renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_ZDevice.device(), m_imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_ZDevice.device(), m_inFlightFences[i], nullptr);
		}
	}

	VkResult ZSwapChain::acquireNextImage(uint32_t* imageIndex)
	{
		// At the start of the frame, we want to wait until the previous frame has finished,
		// so that the command buffer and semaphores are available to use
		vkWaitForFences(
			m_ZDevice.device(),
			1,
			&m_inFlightFences[m_currentFrame],
			VK_TRUE,
			// disable the timeout
			std::numeric_limits<uint64_t>::max());

		VkResult result = vkAcquireNextImageKHR(
			m_ZDevice.device(),
			m_VkSwapchainKHR,
			// disable the timeout
			std::numeric_limits<uint64_t>::max(),
			// specify synchronization objects that are to be signaled when it is safe to render to the image
			m_imageAvailableSemaphores[m_currentFrame], // must be a not signaled semaphore
			// no fence is needed here
			VK_NULL_HANDLE,
			// The last parameter specifies a variable to output the index of the swap chain image that has become available
			imageIndex);

		return result;
	}

	VkResult ZSwapChain::submitCommandBuffers(
		const VkCommandBuffer* buffers, uint32_t* imageIndex)
	{
		if (m_imagesInFlight[*imageIndex] != VK_NULL_HANDLE)
		{
			vkWaitForFences(m_ZDevice.device(), 1, &m_imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
		}
		m_imagesInFlight[*imageIndex] = m_inFlightFences[m_currentFrame];

		VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};

		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			// specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = waitSemaphores,
			.pWaitDstStageMask = waitStages,
			// which command buffers to actually submit for execution
			.commandBufferCount = 1,
			.pCommandBuffers = buffers,
			// specify which semaphores to signal once the command buffer(s) have finished execution
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphores,
		};

		// Only reset the fence if we are submitting work
		vkResetFences(m_ZDevice.device(), 1, &m_inFlightFences[m_currentFrame]);

		// submit the command buffer to the graphics queue
		if (vkQueueSubmit(m_ZDevice.graphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		// Present the swap chain image
		VkSwapchainKHR swapChains[] = {m_VkSwapchainKHR};
		VkPresentInfoKHR presentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			// specify which semaphores to wait on before presentation can happen
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = signalSemaphores,
			// specify the swap chains to present images to and the index of the image for each swap chain
			.swapchainCount = 1,
			.pSwapchains = swapChains,
			.pImageIndices = imageIndex,
		};

		auto result = vkQueuePresentKHR(m_ZDevice.presentQueue(), &presentInfo);
		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		return result;
	}

	void ZSwapChain::createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = m_ZDevice.getSwapChainSupport();

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		// number of images (buffers) to use in the swap chain (3 => triple buffering)
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_ZDevice.surface();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		// imageArrayLayers will always 1 unless you are developing a stereoscopic 3D application
		createInfo.imageArrayLayers = 1;
		// what kind of operations we'll use the images in the swap chain for
		// in this case, we are rendering directly to the images in the swap chain
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// Next, we need to specify how to handle swap chain images that will be used across multiple queue families
		// That will be the case in our application if the graphics queue family is different from the presentation queue

		QueueFamilyIndices indices = m_ZDevice.findPhysicalQueueFamilies();
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		if (indices.graphicsFamily != indices.presentFamily)
		{
			// enable concurrent sharing mode
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			// these two fields specify which queue families ownership will be shared
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			// no sharing b/c graphics and present family are the same queue family
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		// are there any transform should be applied to images in the swap chain (like a horizontal flip)? No
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

		// should the alpha channel be used for blending with other windows in the window system? No
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;

		// we don't care about the color of pixels that are obscured (for example b/c another window is in front of them)
		createInfo.clipped = VK_TRUE;

		// With vulkan, your swap chain may become invalid or unoptimized while your application is running, for example because the window was resized
		// In that case the swap chain actually needs to be recreated from scratch and a reference to the old one must be specified in this field
		createInfo.oldSwapchain = m_oldSwapChain == nullptr ? VK_NULL_HANDLE : m_oldSwapChain->m_VkSwapchainKHR;

		if (vkCreateSwapchainKHR(m_ZDevice.device(), &createInfo, nullptr, &m_VkSwapchainKHR) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		// Retrieving the swap chain images

		// we only specified a minimum number of images in the swap chain, so the implementation is
		// allowed to create a swap chain with more. That's why we'll first query the final number of
		// images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
		// retrieve the handles.
		vkGetSwapchainImagesKHR(m_ZDevice.device(), m_VkSwapchainKHR, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_ZDevice.device(), m_VkSwapchainKHR, &imageCount, m_swapChainImages.data());

		// the following two lines store the format and extent for future use
		m_swapChainImageFormat = surfaceFormat.format;
		m_swapChainExtent = extent;
	}

	void ZSwapChain::createImageViews()
	{
		m_swapChainImageViews.resize(m_swapChainImages.size());
		for (size_t i = 0; i < m_swapChainImages.size(); i++)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_swapChainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_swapChainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_ZDevice.device(), &viewInfo, nullptr, &m_swapChainImageViews[i]) !=
				VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture image view!");
			}
		}
	}

	void ZSwapChain::createRenderPass()
	{
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = getSwapChainImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstSubpass = 0;
		dependency.dstStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(m_ZDevice.device(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void ZSwapChain::createFramebuffers()
	{
		m_swapChainFramebuffers.resize(imageCount());
		for (size_t i = 0; i < imageCount(); i++)
		{
			std::array<VkImageView, 2> attachments = {m_swapChainImageViews[i], m_depthImageViews[i]};

			VkExtent2D swapChainExtent = getSwapChainExtent();
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(
				m_ZDevice.device(),
				&framebufferInfo,
				nullptr,
				&m_swapChainFramebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	void ZSwapChain::createDepthResources()
	{
		VkFormat depthFormat = findDepthFormat();
		m_swapChainDepthFormat = depthFormat;
		VkExtent2D swapChainExtent = getSwapChainExtent();

		m_depthImages.resize(imageCount());
		m_depthImageMemorys.resize(imageCount());
		m_depthImageViews.resize(imageCount());

		for (int i = 0; i < m_depthImages.size(); i++)
		{
			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChainExtent.width;
			imageInfo.extent.height = swapChainExtent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = depthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			m_ZDevice.createImageWithInfo(
				imageInfo,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				m_depthImages[i],
				m_depthImageMemorys[i]);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_depthImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = depthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_ZDevice.device(), &viewInfo, nullptr, &m_depthImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create texture image view!");
			}
		}
	}

	void ZSwapChain::createSyncObjects()
	{
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		m_imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};

		VkFenceCreateInfo fenceInfo = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			// create the fence in the signaled state for the first frame
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(m_ZDevice.device(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateSemaphore(m_ZDevice.device(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) !=
				VK_SUCCESS ||
				vkCreateFence(m_ZDevice.device(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}

	VkSurfaceFormatKHR ZSwapChain::chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && // choose SRGB color format
				availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) // choose SRGB color space if possible
			{
				return availableFormat;
			}
		}
		// if we can't find the requested surface format, settle with the first one
		return availableFormats[0];
	}

	VkPresentModeKHR ZSwapChain::chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				std::cout << "Present mode: Mailbox" << std::endl;
				return availablePresentMode;
			}
		}

		std::cout << "Present mode: V-Sync" << std::endl;
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ZSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		// Vulkan will match currentExtent.width/height with the window resolution (and deal with high DPI display issue)
		// However, some window managers allow us to differ here by setting currentExtent.width/height to a special value: max value of uint32_t

		// Case 1: currentExtent.width/height matches window resolution:
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			// use currentExtent (which stores the window size) if possible
			return capabilities.currentExtent;
		}
		// Case 2: currentExtent.width/height doesn't match window resolution:
		// pick the resolution that best matches the window within the minImageExtent and maxImageExtent bounds
		int width, height;
		glfwGetFramebufferSize(m_ZDevice.getZWindow().getGLFWWindow(), &width, &height);
		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
		                                capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
		                                 capabilities.maxImageExtent.height);

		return actualExtent;
	}

	VkFormat ZSwapChain::findDepthFormat()
	{
		return m_ZDevice.findSupportedFormat(
			{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
}
