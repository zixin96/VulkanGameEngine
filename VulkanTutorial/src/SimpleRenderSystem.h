#pragma once

#include "ZDevice.h"
#include "ZGameObject.h"
#include "ZPipeline.h"
#include "ZCamera.h"

// std
#include <memory>
#include <vector>

namespace ZZX
{
	class SimpleRenderSystem
	{
	public:
		SimpleRenderSystem(ZDevice& device, VkRenderPass renderPass);
		~SimpleRenderSystem();

		// delete copy ctor and assignment to avoid dangling pointer
		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;
		void renderGameObjects(VkCommandBuffer commandBuffer,
		                       std::vector<ZGameObject>& gameObjects,
		                       const ZCamera& camera);
	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass);

		ZDevice& m_zDevice;
		std::unique_ptr<ZPipeline> m_zPipeline;
		VkPipelineLayout m_pipelineLayout;
	};
}
