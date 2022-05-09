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
#include "ZBuffer.h"

namespace ZZX
{
	struct GlobalUbo
	{
		glm::mat4 projectionView{1.f};
		glm::vec4 ambientLightColor{1.f, 1.f, 1.f, 0.02f}; // w is intensity
		glm::vec3 lightPosition{-1.f};
		alignas(16) glm::vec4 lightColor{1.f}; // w is light intensity
	};

	FirstApp::FirstApp()
	{
		m_globalPool = ZDescriptorPool::Builder(m_zDevice)
		               .setMaxSets(ZSwapChain::MAX_FRAMES_IN_FLIGHT)
		               .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ZSwapChain::MAX_FRAMES_IN_FLIGHT)
		               .build();

		loadGameObjects();
	}

	FirstApp::~FirstApp()
	{
	}

	void FirstApp::run()
	{
		std::vector<std::unique_ptr<ZBuffer>> uboBuffers(ZSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++)
		{
			uboBuffers[i] = std::make_unique<ZBuffer>(m_zDevice,
			                                          sizeof(GlobalUbo),
			                                          1,
			                                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->map();
		}

		auto globalSetLayout = ZDescriptorSetLayout::Builder(m_zDevice)
		                       .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		                       .build();
		std::vector<VkDescriptorSet> globalDescriptorSets(ZSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			ZDescriptorWriter(*globalSetLayout, *m_globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{
			m_zDevice, m_zRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()
		};

		ZCamera camera{};
		camera.setViewTarget(glm::vec3{-1.f, -2.f, 2.f}, glm::vec3{0.0f, 0.f, 2.5f});

		// this game object is used to store camera's current state
		auto viewerObject = ZGameObject::createGameObject();
		viewerObject.m_transform.translation.z = -2.5f;

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
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 100.f);

			if (auto commandBuffer = m_zRenderer.beginFrame())
			{
				int frameIndex = m_zRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex]
				};
				// update
				GlobalUbo ubo{};
				ubo.projectionView = camera.getProjection() * camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				m_zRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo, m_gameObjects);
				m_zRenderer.endSwapChainRenderPass(commandBuffer);
				m_zRenderer.endFrame();
			}
		}
		// wait for the logical device to finish operations
		vkDeviceWaitIdle(m_zDevice.device());
	}

	void FirstApp::loadGameObjects()
	{
		std::shared_ptr<ZModel> zModel = ZModel::createModelFromFile(m_zDevice, "assets/models/flat_vase.obj");
		auto gameObject = ZGameObject::createGameObject();
		gameObject.m_model = zModel;
		gameObject.m_transform.translation = {-0.5f, 0.5f, 0.f};
		gameObject.m_transform.scale = glm::vec3{3.f, 1.5f, 3.f};
		m_gameObjects.push_back(std::move(gameObject));

		zModel = ZModel::createModelFromFile(m_zDevice, "assets/models/smooth_vase.obj");
		auto smoothVase = ZGameObject::createGameObject();
		smoothVase.m_model = zModel;
		smoothVase.m_transform.translation = {0.5f, 0.5f, 0.f};
		smoothVase.m_transform.scale = glm::vec3{3.f, 1.5f, 3.f};
		m_gameObjects.push_back(std::move(smoothVase));

		zModel = ZModel::createModelFromFile(m_zDevice, "assets/models/quad.obj");
		auto floor = ZGameObject::createGameObject();
		floor.m_model = zModel;
		floor.m_transform.translation = {0.0f, 0.5f, 0.f};
		floor.m_transform.scale = glm::vec3{3.f, 1.f, 3.f};
		m_gameObjects.push_back(std::move(floor));
	}
}
