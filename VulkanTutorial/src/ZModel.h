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
			glm::vec3 pos;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		struct Builder
		{
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};
		};

		ZModel(ZDevice& zDevice, const ZModel::Builder& builder);
		~ZModel();

		// delete copy ctor and assignment to avoid dangling pointer
		ZModel(const ZModel&);
		ZModel& operator=(const ZModel&);

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);
	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		ZDevice& m_zDevice;

		VkBuffer m_vertexBuffer;
		VkDeviceMemory m_vertexBufferMemory;
		uint32_t m_vertexCount;

		bool m_hasIndexBuffer = false;
		VkBuffer m_indexBuffer;
		VkDeviceMemory m_indexBufferMemory;
		uint32_t m_indexCount;
	};
};
