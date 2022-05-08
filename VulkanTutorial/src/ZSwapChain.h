#pragma once
#include "ZDevice.h"

#include <memory>
namespace ZZX
{
	class ZSwapChain
	{
	public:
		// how many frames should be processed concurrently
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		ZSwapChain(ZDevice& deviceRef, VkExtent2D windowExtent);
		ZSwapChain(ZDevice& deviceRef, VkExtent2D windowExtent, std::shared_ptr<ZSwapChain> previous);
		~ZSwapChain();

		ZSwapChain(const ZSwapChain&) = delete;
		ZSwapChain& operator=(const ZSwapChain&) = delete;

		VkFramebuffer getFrameBuffer(int index) { return m_swapChainFramebuffers[index]; }
		VkRenderPass getRenderPass() { return m_renderPass; }
		VkImageView getImageView(int index) { return m_swapChainImageViews[index]; }

		// this count will likely be 2 (for double buffering) or 3 (for triple buffering)
		size_t imageCount() { return m_swapChainImages.size(); }

		VkFormat getSwapChainImageFormat() { return m_swapChainImageFormat; }
		VkExtent2D getSwapChainExtent() { return m_swapChainExtent; }
		uint32_t width() { return m_swapChainExtent.width; }
		uint32_t height() { return m_swapChainExtent.height; }

		float extentAspectRatio()
		{
			return static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height);
		}

		VkFormat findDepthFormat();

		// this function fetches the index of the next available image that your application should render to
		// it also handles CPU-GPU sync 
		VkResult acquireNextImage(uint32_t* imageIndex);
		VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

	private:
		void init();
		void createSwapChain();
		void createImageViews();
		void createDepthResources();
		void createRenderPass();
		void createFramebuffers();
		void createSyncObjects();

		// Helper functions
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		VkFormat m_swapChainImageFormat;
		VkExtent2D m_swapChainExtent;

		std::vector<VkFramebuffer> m_swapChainFramebuffers;
		VkRenderPass m_renderPass;

		std::vector<VkImage> m_depthImages;
		std::vector<VkDeviceMemory> m_depthImageMemorys;
		std::vector<VkImageView> m_depthImageViews;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;

		ZDevice& m_zDevice;
		VkExtent2D m_windowExtent;

		VkSwapchainKHR m_swapChain;

		// keep track of the old swap chain for better resizing behavior
		// (since resources can be reused when you provide the old swap chain when creating a new one)
		std::shared_ptr<ZSwapChain> m_oldSwapChain;

		// signal that an image has been acquired from the swapchain and is ready for rendering
		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		// signal that rendering has finished and presentation can happen
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		// make sure only one frame is rendering at a time 
		std::vector<VkFence> m_inFlightFences;

		std::vector<VkFence> m_imagesInFlight;
		size_t m_currentFrame = 0;
	};
};
