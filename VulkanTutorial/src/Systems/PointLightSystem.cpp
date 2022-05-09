#include "PointLightSystem.h"

#include <stdexcept>
#include <array>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace ZZX
{
	PointLightSystem::PointLightSystem(ZDevice& device, VkRenderPass renderPass,
	                                       VkDescriptorSetLayout globalSetLayout)
		: m_zDevice(device)
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	PointLightSystem::~PointLightSystem()
	{
		vkDestroyPipelineLayout(m_zDevice.device(), m_pipelineLayout, nullptr);
	}

	void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		/*VkPushConstantRange pushConstantRange{
			// both vert/frag shaders can access push constants
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0,
			.size = sizeof(SimplePushConstantData)
		};*/

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
			.pSetLayouts = descriptorSetLayouts.data(),
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr,
		};

		if (vkCreatePipelineLayout(m_zDevice.device(),
		                           &pipelineLayoutInfo,
		                           nullptr,
		                           &m_pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void PointLightSystem::createPipeline(VkRenderPass renderPass)
	{
		assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		ZPipeline::defaultPipelineConfigInfo(pipelineConfig);

		// we don't want point light system to take the default attribute and binding descriptions
		pipelineConfig.attributeDescriptions.clear();
		pipelineConfig.bindingDescriptions.clear();

		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = m_pipelineLayout;
		m_zPipeline = std::make_unique<ZPipeline>(m_zDevice,
		                                          pipelineConfig,
		                                          "assets/shaders/point_light.vert.spv",
		                                          "assets/shaders/point_light.frag.spv"
		);
	}


	void PointLightSystem::render(FrameInfo& frameInfo)
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
		vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
	}
}
