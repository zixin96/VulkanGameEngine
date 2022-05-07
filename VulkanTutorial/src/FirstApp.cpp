#include "FirstApp.h"

#include <stdexcept>
#include <array>

namespace ZZX
{
	FirstApp::FirstApp()
	{
		createPipelineLayout();
		createPipeline();
		createCommandBuffers();
	}

	FirstApp::~FirstApp()
	{
		vkDestroyPipelineLayout(m_zDevice.device(), m_pipelineLayout, nullptr);
	}

	void FirstApp::run()
	{
		while (!m_zWindow.shouldClose())
		{
			glfwPollEvents();
			drawFrame();
		}
		// wait for the logical device to finish operations
		vkDeviceWaitIdle(m_zDevice.device());
	}

	void FirstApp::createPipelineLayout()
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 0,
			.pSetLayouts = nullptr,
			.pushConstantRangeCount = 0, // Optional
			.pPushConstantRanges = nullptr, // Optional
		};

		if (vkCreatePipelineLayout(m_zDevice.device(),
		                           &pipelineLayoutInfo,
		                           nullptr,
		                           &m_pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void FirstApp::createPipeline()
	{
		PipelineConfigInfo pipelineConfig{};
		ZPipeline::defaultPipelineConfigInfo(pipelineConfig,
		                                     // Note: here we use swap chain's width and height, not the window's
		                                     m_zSwapChain.width(),
		                                     m_zSwapChain.height());
		pipelineConfig.renderPass = m_zSwapChain.getRenderPass();
		pipelineConfig.pipelineLayout = m_pipelineLayout;

		m_zPipeline = std::make_unique<ZPipeline>(m_zDevice,
		                                          pipelineConfig,
		                                          "assets/shaders/simple_shader.vert.spv",
		                                          "assets/shaders/simple_shader.frag.spv"
		);
	}

	void FirstApp::createCommandBuffers()
	{
		// each command buffer will draw to a different framebuffer
		// TODO: this is different from vulkan-tutorial. They set it to MAX_FRAMES_IN_FLIGHT
		m_commandBuffers.resize(m_zSwapChain.imageCount());

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

		for (int i = 0; i < m_commandBuffers.size(); i++)
		{
			VkCommandBufferBeginInfo beginInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				//.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			};

			if (vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to begin recording command buffers!");
			}

			// Starting a render pass
			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0f};
			clearValues[1].depthStencil = {1.0f, 0};

			VkRenderPassBeginInfo renderPassInfo{
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.renderPass = m_zSwapChain.getRenderPass(),
				.framebuffer = m_zSwapChain.getFrameBuffer(i),
				// define the size of the render area
				.renderArea = {.offset = {0, 0}, .extent = m_zSwapChain.getSwapChainExtent()},
				.clearValueCount = static_cast<uint32_t>(clearValues.size()),
				.pClearValues = clearValues.data(),
			};

			// The render pass can now begin
			vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			m_zPipeline->bind(m_commandBuffers[i]);
			vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);
			vkCmdEndRenderPass(m_commandBuffers[i]);

			if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to end recording command buffers!");
			}
		}
	}

	void FirstApp::drawFrame()
	{
		// acquire the next available image that your application should render to
		uint32_t imageIndex;
		auto result = m_zSwapChain.acquireNextImage(&imageIndex);

		/*if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// immediately recreate the swap chain and try again in the next drawFrame call
			recreateSwapChain();
			return;
		}*/
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		result = m_zSwapChain.submitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}
	}
}
