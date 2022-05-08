#include "ZModel.h"

namespace ZZX
{
	std::vector<VkVertexInputBindingDescription> ZModel::Vertex::getBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0] = {
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> ZModel::Vertex::getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
		attributeDescriptions[0] = {
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT, // Vertex::pos
			.offset = offsetof(Vertex, pos),
		};
		attributeDescriptions[1] = {
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT, // Vertex::color
			.offset = offsetof(Vertex, color),
		};
		return attributeDescriptions;
	}

	ZModel::ZModel(ZDevice& zDevice, const std::vector<Vertex>& vertices)
		: m_zDevice(zDevice)
	{
		createVertexBuffers(vertices);
	}

	ZModel::~ZModel()
	{
		// TODO: this has limitations. Consider using library like VMA 
		vkDestroyBuffer(m_zDevice.device(), m_vertexBuffer, nullptr);
		vkFreeMemory(m_zDevice.device(), m_vertexBufferMemory, nullptr);
	}

	void ZModel::bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = {m_vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	}

	void ZModel::draw(VkCommandBuffer commandBuffer)
	{
		vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
	}

	void ZModel::createVertexBuffers(const std::vector<Vertex>& vertices)
	{
		m_vertexCount = static_cast<uint32_t>(vertices.size());
		assert(m_vertexCount >= 3 && "vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		m_zDevice.createBuffer(bufferSize,
		                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                       m_vertexBuffer,
		                       m_vertexBufferMemory);
		void* data;
		vkMapMemory(m_zDevice.device(), m_vertexBufferMemory, 0, bufferSize, 0, &data);
		// how mpmcpy works here: https://youtu.be/mnKp501RXDc?t=1041
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(m_zDevice.device(), m_vertexBufferMemory);
	}
}
