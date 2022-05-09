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

#include "Systems/SimpleRenderSystem.h"
#include "Systems/PointLightSystem.h"
#include "ZCamera.h"
#include "KeyboardMovementController.h"
#include "ZBuffer.h"

namespace ZZX
{
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
		                       .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
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

		PointLightSystem pointLightSystem{
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
					globalDescriptorSets[frameIndex],
					m_gameObjects
				};
				// update
				GlobalUbo ubo{};
				ubo.projection = camera.getProjection();
				ubo.view = camera.getView();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				m_zRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo);
				pointLightSystem.render(frameInfo);
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
		auto flat_vase = ZGameObject::createGameObject();
		flat_vase.m_model = zModel;
		flat_vase.m_transform.translation = {-0.5f, 0.5f, 0.f};
		flat_vase.m_transform.scale = glm::vec3{3.f, 1.5f, 3.f};
		m_gameObjects.emplace(flat_vase.getId(), std::move(flat_vase));

		zModel = ZModel::createModelFromFile(m_zDevice, "assets/models/smooth_vase.obj");
		auto smoothVase = ZGameObject::createGameObject();
		smoothVase.m_model = zModel;
		smoothVase.m_transform.translation = {0.5f, 0.5f, 0.f};
		smoothVase.m_transform.scale = glm::vec3{3.f, 1.5f, 3.f};
		m_gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

		zModel = ZModel::createModelFromFile(m_zDevice, "assets/models/quad.obj");
		auto floor = ZGameObject::createGameObject();
		floor.m_model = zModel;
		floor.m_transform.translation = {0.0f, 0.5f, 0.f};
		floor.m_transform.scale = glm::vec3{3.f, 1.f, 3.f};
		m_gameObjects.emplace(floor.getId(), std::move(floor));


		std::vector<glm::vec3> lightColors{
			{1.f, .1f, .1f},
			{.1f, .1f, 1.f},
			{.1f, 1.f, .1f},
			{1.f, 1.f, .1f},
			{.1f, 1.f, 1.f},
			{1.f, 1.f, 1.f}
		};

		for (int i = 0; i < lightColors.size(); i++)
		{
			auto pointLight = ZGameObject::makePointLight(0.2f);
			pointLight.m_color = lightColors[i];
			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{0.f, -1.f, 0.f});
			pointLight.m_transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
			m_gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}
	}
}
