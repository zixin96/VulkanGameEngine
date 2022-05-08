#include "FirstApp.h"

#include <stdexcept>
#include <array>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "SimpleRenderSystem.h"

namespace ZZX
{
	FirstApp::FirstApp()
	{
		loadGameObjects();
	}

	FirstApp::~FirstApp()
	{
	}

	void FirstApp::run()
	{
		SimpleRenderSystem simpleRenderSystem{m_zDevice, m_zRenderer.getSwapChainRenderPass()};
		while (!m_zWindow.shouldClose())
		{
			glfwPollEvents();
			if (auto commandBuffer = m_zRenderer.beginFrame())
			{
				m_zRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, m_gameObjects);
				m_zRenderer.endSwapChainRenderPass(commandBuffer);
				m_zRenderer.endFrame();
			}
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
}
