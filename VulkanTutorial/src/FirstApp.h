#pragma once

#include "ZWindow.h"
#include "ZPipeline.h"
#include "ZSwapChain.h"
#include "ZModel.h"

// std
#include <memory>

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
		void loadModels();
		void sierpinski(std::vector<ZModel::Vertex>& vertices,
		                int depth,
		                glm::vec2 left,
		                glm::vec2 right,
		                glm::vec2 top);
		void createPipelineLayout();
		void createPipeline();

		// this function is only responsible for command buffers allocation
		void createCommandBuffers();

		void freeCommandBuffers();
		void drawFrame();
		void recreateSwapChain();

		// this function records the command buffer every frame for the swap chain image indicated by imageIndex
		void recordCommandBuffer(int imageIndex);

		ZWindow m_zWindow{WIDTH, HEIGHT, "Vulkan Renderer"};
		ZDevice m_zDevice{m_zWindow};

		// by using a unique pointer to the swap chain,
		// we can create a new swap chain with updated info (such as width and height)
		// by simply creating a new swap chain object
		std::unique_ptr<ZSwapChain> m_zSwapChain;

		std::unique_ptr<ZPipeline> m_zPipeline;
		VkPipelineLayout m_pipelineLayout;
		std::vector<VkCommandBuffer> m_commandBuffers;
		std::unique_ptr<ZModel> m_zModel;
	};
}
