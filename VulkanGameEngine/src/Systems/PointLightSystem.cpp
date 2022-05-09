#include "PointLightSystem.h"

#include <stdexcept>
#include <array>
#include <map>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace ZZX
{
	struct PointLightPushConstants
	{
		glm::vec4 position{};
		glm::vec4 color{};
		float radius;
	};

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
		VkPushConstantRange pushConstantRange{
			// both vert/frag shaders can access push constants
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.offset = 0,
			.size = sizeof(PointLightPushConstants)
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

	void PointLightSystem::createPipeline(VkRenderPass renderPass)
	{
		assert(m_pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		ZPipeline::defaultPipelineConfigInfo(pipelineConfig);
		ZPipeline::enableAlphaBlending(pipelineConfig);

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


	void PointLightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo)
	{
		auto rotateLight = glm::rotate(glm::mat4(1.f), frameInfo.frameTime, {0.f, -1.f, 0.f});
		int lightIndex = 0;
		for (auto& kv : frameInfo.gameObjects)
		{
			auto& obj = kv.second;
			if (obj.m_pointLight == nullptr) continue;

			assert(lightIndex < MAX_LIGHTS && "Point lights exceed max specified");

			// update light position
			obj.m_transform.translation = glm::vec3(rotateLight * glm::vec4(obj.m_transform.translation, 1.0f));

			// copy light to ubo
			ubo.pointLights[lightIndex].position = glm::vec4(obj.m_transform.translation, 1.0f);
			ubo.pointLights[lightIndex].color = glm::vec4(obj.m_color, obj.m_pointLight->lightIntensity);
			lightIndex += 1;
		}
		ubo.numLights = lightIndex;
	}

	void PointLightSystem::render(FrameInfo& frameInfo)
	{
		// sort lights
		std::map<float, ZGameObject::id_t> sorted;
		for (auto& kv : frameInfo.gameObjects)
		{
			auto& obj = kv.second;
			if (obj.m_pointLight == nullptr) continue;
			// calculate distance
			auto offset = frameInfo.camera.getPosition() - obj.m_transform.translation;
			float disSquared = glm::dot(offset, offset);
			sorted[disSquared] = obj.getId();
		}

		m_zPipeline->bind(frameInfo.commandBuffer);
		vkCmdBindDescriptorSets(frameInfo.commandBuffer,
		                        VK_PIPELINE_BIND_POINT_GRAPHICS,
		                        m_pipelineLayout,
		                        0,
		                        1,
		                        &frameInfo.globalDescriptorSet,
		                        0,
		                        nullptr);
		 
		// iterate through sorted lights in reverse order
		for (auto it = sorted.rbegin(); it != sorted.rend(); ++it)
		{
			auto& obj = frameInfo.gameObjects.at(it->second);
			PointLightPushConstants push{};
			push.position = glm::vec4(obj.m_transform.translation, 1.0f);
			push.color = glm::vec4(obj.m_color, obj.m_pointLight->lightIntensity);
			push.radius = obj.m_transform.scale.x;
			vkCmdPushConstants(frameInfo.commandBuffer,
			                   m_pipelineLayout,
			                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			                   0,
			                   sizeof(PointLightPushConstants),
			                   &push);
			vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
		}
	}
}
