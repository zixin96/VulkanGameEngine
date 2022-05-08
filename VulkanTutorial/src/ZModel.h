#pragma once
#include "ZDevice.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace ZZX
{
	class ZModel
	{
	public:
		struct Vertex
		{
			glm::vec2 pos;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		ZModel(ZDevice& zDevice, const std::vector<Vertex>& vertices);
		~ZModel();

		// delete copy ctor and assignment to avoid dangling pointer
		ZModel(const ZModel&);
		ZModel& operator=(const ZModel&);
		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);
	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		ZDevice& m_zDevice;
		VkBuffer m_vertexBuffer;
		VkDeviceMemory m_vertexBufferMemory;
		uint32_t m_vertexCount;
	};
};
