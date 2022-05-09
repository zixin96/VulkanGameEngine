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
	struct SimplePushConstantData
	{
		glm::mat4 modelMatrix{1.f};
		glm::mat4 normalMatrix{1.f};
	};

	SimpleRenderSystem::SimpleRenderSystem(ZDevice& device, VkRenderPass renderPass,
	                                       VkDescriptorSetLayout globalSetLayout)
		: m_zDevice(device)
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem()
	{
		vkDestroyPipelineLayout(m_zDevice.device(), m_pipelineLayout, nullptr);
	}

	void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		VkPushConstantRange pushConstantRange{
			// both vert/frag shaders can access push constants
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0,
			.size = sizeof(SimplePushConstantData)
		};

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
			.pSetLayouts = descriptorSetLayouts.data(),
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


	void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo,
	                                           std::vector<ZGameObject>& gameObjects)
	{
		m_zPipeline->bind(frameInfo.commandBuffer);
		vkCmdBindDescriptorSets(frameInfo.commandBuffer,
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        m_pipelineLayout,
		                        0,
		                        1,
		                        &frameInfo.globalDescriptorSet,
		                        0,
		                        nullptr);
		for (auto& obj : gameObjects)
		{
			SimplePushConstantData push{
				.modelMatrix = obj.m_transform.mat4(),
				.normalMatrix = obj.m_transform.normalMatrix(),
			};
			vkCmdPushConstants(frameInfo.commandBuffer,
			                   m_pipelineLayout,
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			                   0,
			                   sizeof(SimplePushConstantData),
			                   &push);
			obj.m_model->bind(frameInfo.commandBuffer);
			obj.m_model->draw(frameInfo.commandBuffer);
		}
	}
}
