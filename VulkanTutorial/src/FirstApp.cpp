#include "FirstApp.h"

#include <array>
#include <chrono>
#include <cassert>
#include <stdexcept>

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include "SimpleRenderSystem.h"
#include "ZCamera.h"
#include "KeyboardMovementController.h"


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

		ZCamera camera{};
		camera.setViewTarget(glm::vec3{-1.f, -2.f, 2.f}, glm::vec3{0.0f, 0.f, 2.5f});

		// this game object is used to store camera's current state
		auto viewerObject = ZGameObject::createGameObject();

		KeyboardMovementController cameraController{};

		auto currentTime = std::chrono::high_resolution_clock::now();

		while (!m_zWindow.shouldClose())
		{
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			auto frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			cameraController.moveInPlaneXZ(m_zWindow.getGLFWWindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.m_transform.translation, viewerObject.m_transform.rotation);
			float aspect = m_zRenderer.getAspectRatio();
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

			if (auto commandBuffer = m_zRenderer.beginFrame())
			{
				m_zRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, m_gameObjects, camera);
				m_zRenderer.endSwapChainRenderPass(commandBuffer);
				m_zRenderer.endFrame();
			}
		}
		// wait for the logical device to finish operations
		vkDeviceWaitIdle(m_zDevice.device());
	}

	void FirstApp::loadGameObjects()
	{
		std::shared_ptr<ZModel> zModel = ZModel::createModelFromFile(m_zDevice, "assets/models/smooth_vase.obj");
		auto gameObject = ZGameObject::createGameObject();
		gameObject.m_model = zModel;
		gameObject.m_transform.translation = {0.f, 0.f, 2.5f};
		gameObject.m_transform.scale = glm::vec3{ 3.f };
		m_gameObjects.push_back(std::move(gameObject));
	}
}
