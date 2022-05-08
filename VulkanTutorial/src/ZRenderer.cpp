#include "ZRenderer.h"

#include <stdexcept>
#include <array>

namespace ZZX
{
	ZRenderer::ZRenderer(ZWindow& window, ZDevice& device)
		: m_zWindow{window}, m_zDevice{device}
	{
		recreateSwapChain();
		createCommandBuffers();
	}

	ZRenderer::~ZRenderer()
	{
		freeCommandBuffers();
	}

	VkCommandBuffer ZRenderer::beginFrame()
	{
		assert(!m_isFrameStarted && "cannot call beginFrame while already in progress");
		// acquire the next available image that your application should render to
		auto result = m_zSwapChain->acquireNextImage(&m_currentImageIndex);

		// Every frame before drawing, check if window has been resized and swap chain is still valid
		// recreate the swap chain as needed:
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// immediately recreate the swap chain and try again in the next drawFrame call
			recreateSwapChain();
			// the frame has not successfully started
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		m_isFrameStarted = true;
		auto commandBuffer = getCurrentCommandBuffer();
		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffers!");
		}
		return commandBuffer;
	}

	void ZRenderer::endFrame()
	{
		assert(m_isFrameStarted && "cannot call endFrame while frame is not in progress");
		auto commandBuffer = getCurrentCommandBuffer();
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to end recording command buffers!");
		}

		auto result = m_zSwapChain->submitCommandBuffers(&commandBuffer, &m_currentImageIndex);

		// since some drivers/platforms will not trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize,
		// extra checks are needed here:
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_zWindow.wasWindowResized())
		{
			m_zWindow.resetWindowResizedFlag();
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}
		m_isFrameStarted = false;
		m_currentFrameIndex = (m_currentFrameIndex + 1) % ZSwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void ZRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(m_isFrameStarted && "cannot call beginSwapChainRenderPass if frame is not in progress");
		assert(
			commandBuffer == getCurrentCommandBuffer() &&
			"cannot begin render pass on command buffer from a different frame");

		// Starting a render pass
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
		clearValues[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_zSwapChain->getRenderPass(),
			.framebuffer = m_zSwapChain->getFrameBuffer(m_currentImageIndex),
			// define the size of the render area
			.renderArea = {.offset = {0, 0}, .extent = m_zSwapChain->getSwapChainExtent()},
			.clearValueCount = static_cast<uint32_t>(clearValues.size()),
			.pClearValues = clearValues.data(),
		};

		// The render pass can now begin
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// dynamic viewports/scissor: specifying viewports/scissor in the command buffer, rather than during pipeline creation,
		// so that pipeline is no longer dependent on swap chain dimensions

		// describes the viewport transformation from NDC to pixel space
		// "squishing/squashing" the triangles
		VkViewport viewport{
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(m_zSwapChain->getSwapChainExtent().width),
			.height = static_cast<float>(m_zSwapChain->getSwapChainExtent().height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		// define in which regions pixels will actually be stored
		// Any pixels outside the scissor rectangles will be discarded by the rasterizer
		// "cut" the triangle
		VkRect2D scissor{{0, 0}, m_zSwapChain->getSwapChainExtent()};

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void ZRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(m_isFrameStarted && "cannot call endSwapChainRenderPass if frame is not in progress");
		assert(
			commandBuffer == getCurrentCommandBuffer() &&
			"cannot end render pass on command buffer from a different frame");
		vkCmdEndRenderPass(commandBuffer);
	}

	void ZRenderer::createCommandBuffers()
	{
		m_commandBuffers.resize(ZSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = m_zDevice.getCommandPool(),
			// PRIMARY vs SECONDARY command buffer: https://youtu.be/_VOR6q3edig?t=168
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size())
		};

		if (vkAllocateCommandBuffers(m_zDevice.device(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void ZRenderer::freeCommandBuffers()
	{
		vkFreeCommandBuffers(m_zDevice.device(),
		                     m_zDevice.getCommandPool(),
		                     static_cast<uint32_t>(m_commandBuffers.size()),
		                     m_commandBuffers.data());
		m_commandBuffers.clear();
	}

	void ZRenderer::recreateSwapChain()
	{
		// retrieve the current window size
		auto extent = m_zWindow.getExtent();

		// the program will pause and wait if one of dimensions is sizeless (e.g. when window is minimized)
		while (extent.width == 0 || extent.height == 0)
		{
			extent = m_zWindow.getExtent();
			glfwWaitEvents();
		}

		// wait for the logical device to finish operations
		// before we create a new swap chain, we need to wait until the current swap chain is no longer being used 
		vkDeviceWaitIdle(m_zDevice.device());

		if (m_zSwapChain == nullptr)
		{
			m_zSwapChain = std::make_unique<ZSwapChain>(m_zDevice, extent);
		}
		// we have an old swap chain that has reusable resources:
		else
		{
			std::shared_ptr<ZSwapChain> oldSwapChain = std::move(m_zSwapChain);
			m_zSwapChain = std::make_unique<ZSwapChain>(m_zDevice, extent, oldSwapChain);
			if (!oldSwapChain->compareSwapFormats(*m_zSwapChain))
			{
				throw std::runtime_error("Swap chain image(or depth) format has changed!");
			}
		}
	}
}
