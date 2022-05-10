#pragma once

#include "ZDevice.h"
#include "ZGameObject.h"
#include "ZPipeline.h"
#include "ZCamera.h"
#include "ZFrameInfo.h"

namespace ZZX
{
	class SimpleRenderSystem
	{
	public:
		SimpleRenderSystem(ZDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~SimpleRenderSystem();

		// delete copy ctor and assignment to avoid dangling pointer
		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;
		void renderGameObjects(FrameInfo& frameInfo);
	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);

		ZDevice& m_zDevice;
		std::unique_ptr<ZPipeline> m_zPipeline;
		VkPipelineLayout m_pipelineLayout;
	};
}
