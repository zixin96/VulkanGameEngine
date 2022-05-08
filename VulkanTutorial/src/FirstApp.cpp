﻿#include "FirstApp.h"

#include <stdexcept>
#include <array>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace ZZX
{
	// TODO: we temporarily put push constant data in the app class
	struct SimplePushConstantData
	{
		glm::mat2 transform{1.f};
		glm::vec2 offset;
		alignas(16) glm::vec3 color;
	};

	FirstApp::FirstApp()
	{
		loadGameObjects();
		createPipelineLayout();
		recreateSwapChain();
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

	void FirstApp::loadGameObjects()
	{
		std::vector<ZModel::Vertex> vertices{
			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};
		// shared_ptr allows us to have one model instance to be used by multiple game objects
		// , which will stay in the memory as long as there is at least one game object using it
		auto lveModel = std::make_shared<ZModel>(m_zDevice, vertices);

		// https://www.color-hex.com/color-palette/5361
		std::vector<glm::vec3> colors{
			{1.f, .7f, .73f},
			{1.f, .87f, .73f},
			{1.f, 1.f, .73f},
			{.73f, 1.f, .8f},
			{.73, .88f, 1.f} //
		};
		for (auto& color : colors)
		{
			color = glm::pow(color, glm::vec3{2.2f});
		}
		for (int i = 0; i < 40; i++)
		{
			auto triangle = ZGameObject::createGameObject();
			triangle.m_model = lveModel;
			triangle.m_transform2d.scale = glm::vec2(.5f) + i * 0.025f;
			triangle.m_transform2d.rotation = i * glm::pi<float>() * .025f;
			triangle.m_color = colors[i % colors.size()];
			m_gameObjects.push_back(std::move(triangle));
		}
	}

	void FirstApp::sierpinski(std::vector<ZModel::Vertex>& vertices,
	                          int depth,
	                          glm::vec2 left,
	                          glm::vec2 right,
	                          glm::vec2 top)
	{
		if (depth <= 0)
		{
			vertices.push_back({top});
			vertices.push_back({right});
			vertices.push_back({left});
		}
		else
		{
			auto leftTop = 0.5f * (left + top);
			auto rightTop = 0.5f * (right + top);
			auto leftRight = 0.5f * (left + right);
			sierpinski(vertices, depth - 1, left, leftRight, leftTop);
			sierpinski(vertices, depth - 1, leftRight, right, rightTop);
			sierpinski(vertices, depth - 1, leftTop, rightTop, top);
		}
	}

	void FirstApp::createPipelineLayout()
	{
		VkPushConstantRange pushConstantRange{
			// both vert/frag shaders can access push constants
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0,
			.size = sizeof(SimplePushConstantData)
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 0,
			.pSetLayouts = nullptr,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange,
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
		assert(m_zSwapChain != nullptr && "Cannot create pipeline before swap chain");
		assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		ZPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = m_zSwapChain->getRenderPass();
		pipelineConfig.pipelineLayout = m_pipelineLayout;
		m_zPipeline = std::make_unique<ZPipeline>(m_zDevice,
		                                          pipelineConfig,
		                                          "assets/shaders/simple_shader.vert.spv",
		                                          "assets/shaders/simple_shader.frag.spv"
		);
	}

	void FirstApp::createCommandBuffers()
	{
		// one-to-one relationship between command buffers and swap chain images
		// TODO: this is different from vulkan-tutorial. They set it to MAX_FRAMES_IN_FLIGHT
		m_commandBuffers.resize(m_zSwapChain->imageCount());

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

	void FirstApp::freeCommandBuffers()
	{
		vkFreeCommandBuffers(m_zDevice.device(),
		                     m_zDevice.getCommandPool(),
		                     static_cast<uint32_t>(m_commandBuffers.size()),
		                     m_commandBuffers.data());
		m_commandBuffers.clear();
	}

	void FirstApp::drawFrame()
	{
		// acquire the next available image that your application should render to
		uint32_t imageIndex;
		auto result = m_zSwapChain->acquireNextImage(&imageIndex);

		// Every frame before drawing, check if window has been resized and swap chain is still valid
		// recreate the swap chain as needed:
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// immediately recreate the swap chain and try again in the next drawFrame call
			recreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		recordCommandBuffer(imageIndex);
		result = m_zSwapChain->submitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);

		// since some drivers/platforms will not trigger VK_ERROR_OUT_OF_DATE_KHR automatically after a window resize,
		// extra checks are needed here:
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_zWindow.wasWindowResized())
		{
			m_zWindow.resetWindowResizedFlag();
			recreateSwapChain();
			return;
		}

		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}
	}

	void FirstApp::recreateSwapChain()
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
			m_zSwapChain = std::make_unique<ZSwapChain>(m_zDevice, extent, std::move(m_zSwapChain));
			if (m_zSwapChain->imageCount() != m_commandBuffers.size())
			{
				freeCommandBuffers();
				createCommandBuffers();
			}
		}

		// since the pipeline also depends on the swap chain's render pass, we need to recreate the pipeline as well
		// TODO: however, if the new swap chain needs a pipeline similar to the current one (compatible one), we do not need to create a new one 
		createPipeline();
	}

	void FirstApp::recordCommandBuffer(int imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};

		if (vkBeginCommandBuffer(m_commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to begin recording command buffers!");
		}

		// Starting a render pass
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
		clearValues[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = m_zSwapChain->getRenderPass(),
			.framebuffer = m_zSwapChain->getFrameBuffer(imageIndex),
			// define the size of the render area
			.renderArea = {.offset = {0, 0}, .extent = m_zSwapChain->getSwapChainExtent()},
			.clearValueCount = static_cast<uint32_t>(clearValues.size()),
			.pClearValues = clearValues.data(),
		};

		// The render pass can now begin
		vkCmdBeginRenderPass(m_commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

		vkCmdSetViewport(m_commandBuffers[imageIndex], 0, 1, &viewport);
		vkCmdSetScissor(m_commandBuffers[imageIndex], 0, 1, &scissor);

		renderGameObjects(m_commandBuffers[imageIndex]);

		vkCmdEndRenderPass(m_commandBuffers[imageIndex]);

		if (vkEndCommandBuffer(m_commandBuffers[imageIndex]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to end recording command buffers!");
		}
	}

	void FirstApp::renderGameObjects(VkCommandBuffer commandBuffer)
	{
		// update
		int i = 0;
		for (auto& obj : m_gameObjects)
		{
			i += 1;
			obj.m_transform2d.rotation =
				glm::mod<float>(obj.m_transform2d.rotation + 0.0001f * i, 2.f * glm::pi<float>());
		}

		m_zPipeline->bind(commandBuffer);
		for (auto& obj : m_gameObjects)
		{
			SimplePushConstantData push{
				.transform = obj.m_transform2d.mat2(),
				.offset = obj.m_transform2d.translation,
				.color = obj.m_color,
			};
			vkCmdPushConstants(commandBuffer,
			                   m_pipelineLayout,
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			                   0,
			                   sizeof(SimplePushConstantData),
			                   &push);
			obj.m_model->bind(commandBuffer);
			obj.m_model->draw(commandBuffer);
		}
	}
}
