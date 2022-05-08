#pragma once

#include "ZDevice.h"
#include "ZWindow.h"
#include "ZSwapChain.h"
// std
#include <cassert>
#include <memory>
#include <vector>

namespace ZZX
{
	class ZRenderer
	{
	public:
		ZRenderer(ZWindow& window, ZDevice& device);
		~ZRenderer();

		// delete copy ctor and assignment to avoid dangling pointer
		ZRenderer(const ZRenderer&) = delete;
		ZRenderer& operator=(const ZRenderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return m_zSwapChain->getRenderPass(); }
		float getAspectRatio() const { return m_zSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return m_isFrameStarted; }

		VkCommandBuffer getCurrentCommandBuffer() const
		{
			assert(m_isFrameStarted && "cannot get command buffer when frame not in progress");
			return m_commandBuffers[m_currentFrameIndex];
		}

		int getFrameIndex() const
		{
			assert(m_isFrameStarted && "cannot get frame index when frame not in progress");
			return m_currentFrameIndex;
		}

		// start the frame, preparing for command buffer recording
		VkCommandBuffer beginFrame();
		// end the frame, executing the command buffer
		void endFrame();

		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
	private:
		// this function is only responsible for command buffers allocation
		void createCommandBuffers();

		void freeCommandBuffers();
		void recreateSwapChain();


		ZWindow& m_zWindow;
		ZDevice& m_zDevice;

		// by using a unique pointer to the swap chain,
		// we can create a new swap chain with updated info (such as width and height)
		// by simply creating a new swap chain object
		std::unique_ptr<ZSwapChain> m_zSwapChain;

		std::vector<VkCommandBuffer> m_commandBuffers;

		uint32_t m_currentImageIndex;
		int m_currentFrameIndex = 0;
		bool m_isFrameStarted = false;
	};
}
