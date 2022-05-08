#include "SimpleRenderSystem.h"

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
		glm::mat2 transform{ 1.f };
		glm::vec2 offset;
		alignas(16) glm::vec3 color;
	};

	SimpleRenderSystem::SimpleRenderSystem(ZDevice& device, VkRenderPass renderPass)
		: m_zDevice(device)
	{
		createPipelineLayout();
		createPipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem()
	{
		vkDestroyPipelineLayout(m_zDevice.device(), m_pipelineLayout, nullptr);
	}

	void SimpleRenderSystem::createPipelineLayout()
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

	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass)
	{
		assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		ZPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = m_pipelineLayout;
		m_zPipeline = std::make_unique<ZPipeline>(m_zDevice,
			pipelineConfig,
			"assets/shaders/simple_shader.vert.spv",
			"assets/shaders/simple_shader.frag.spv"
			);
	}


	void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<ZGameObject>& gameObjects)
	{
		// update
		int i = 0;
		for (auto& obj : gameObjects)
		{
			i += 1;
			obj.m_transform2d.rotation =
				glm::mod<float>(obj.m_transform2d.rotation + 0.0001f * i, 2.f * glm::pi<float>());
		}

		m_zPipeline->bind(commandBuffer);
		for (auto& obj : gameObjects)
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
