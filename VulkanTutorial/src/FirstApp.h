#pragma once

#include <memory>

#include "ZWindow.h"
#include "ZPipeline.h"
#include "ZSwapChain.h"

namespace ZZX
{
	class FirstApp
	{
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp();
		~FirstApp();

		// delete copy ctor and assignment to avoid dangling pointer
		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();

	private:
		// for passing uniforms 
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffers();
		void drawFrame();

		ZWindow m_zWindow{WIDTH, HEIGHT, "Vulkan Renderer"};
		ZDevice m_zDevice{m_zWindow};
		ZSwapChain m_zSwapChain{m_zDevice, m_zWindow.getExtent()};
		std::unique_ptr<ZPipeline> m_zPipeline;
		VkPipelineLayout m_pipelineLayout;
		std::vector<VkCommandBuffer> m_commandBuffers;
	};
}
