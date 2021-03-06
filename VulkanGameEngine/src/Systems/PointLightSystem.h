#pragma once

#include "ZDevice.h"
#include "ZGameObject.h"
#include "ZPipeline.h"
#include "ZCamera.h"
#include "ZFrameInfo.h"

namespace ZZX
{
	class PointLightSystem
	{
	public:
		PointLightSystem(ZDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PointLightSystem();

		// delete copy ctor and assignment to avoid dangling pointer
		PointLightSystem(const PointLightSystem&) = delete;
		PointLightSystem& operator=(const PointLightSystem&) = delete;

		void update(FrameInfo& frameInfo, GlobalUbo& ubo);
		void render(FrameInfo& frameInfo);
	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		ZDevice& m_zDevice;
		std::unique_ptr<ZPipeline> m_zPipeline;
		VkPipelineLayout m_pipelineLayout;
	};
}
