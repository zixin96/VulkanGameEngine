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

	ZModel::ZModel(ZDevice& zDevice, const ZModel::Builder& builder)
		: m_zDevice(zDevice)
	{
		createVertexBuffers(builder.vertices);
		createIndexBuffers(builder.indices);
	}

	ZModel::~ZModel()
	{
		// TODO: this has limitations. Consider using library like VMA 
		vkDestroyBuffer(m_zDevice.device(), m_vertexBuffer, nullptr);
		vkFreeMemory(m_zDevice.device(), m_vertexBufferMemory, nullptr);

		if (m_hasIndexBuffer)
		{
			vkDestroyBuffer(m_zDevice.device(), m_indexBuffer, nullptr);
			vkFreeMemory(m_zDevice.device(), m_indexBufferMemory, nullptr);
		}
	}

	void ZModel::bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = {m_vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
		if (m_hasIndexBuffer)
		{
			vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void ZModel::draw(VkCommandBuffer commandBuffer)
	{
		if (m_hasIndexBuffer)
		{
			vkCmdDrawIndexed(commandBuffer, m_indexCount, 1, 0, 0, 0);
		}
		else
		{
			vkCmdDraw(commandBuffer, m_vertexCount, 1, 0, 0);
		}
	}

	void ZModel::createVertexBuffers(const std::vector<Vertex>& vertices)
	{
		m_vertexCount = static_cast<uint32_t>(vertices.size());
		assert(m_vertexCount >= 3 && "vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		m_zDevice.createBuffer(bufferSize,
		                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                       stagingBuffer,
		                       stagingBufferMemory);
		void* data;
		vkMapMemory(m_zDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		// how mpmcpy works here: https://youtu.be/mnKp501RXDc?t=1041
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(m_zDevice.device(), stagingBufferMemory);

		m_zDevice.createBuffer(bufferSize,
		                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		                       m_vertexBuffer,
		                       m_vertexBufferMemory);
		m_zDevice.copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

		vkDestroyBuffer(m_zDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(m_zDevice.device(), stagingBufferMemory, nullptr);
	}

	void ZModel::createIndexBuffers(const std::vector<uint32_t>& indices)
	{
		m_indexCount = static_cast<uint32_t>(indices.size());
		m_hasIndexBuffer = m_indexCount > 0;
		if (!m_hasIndexBuffer)
		{
			return;
		}
		VkDeviceSize bufferSize = sizeof(indices[0]) * m_indexCount;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		m_zDevice.createBuffer(bufferSize,
		                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                       stagingBuffer,
		                       stagingBufferMemory);
		void* data;
		vkMapMemory(m_zDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		// how mpmcpy works here: https://youtu.be/mnKp501RXDc?t=1041
		memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(m_zDevice.device(), stagingBufferMemory);

		m_zDevice.createBuffer(bufferSize,
		                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		                       m_indexBuffer,
		                       m_indexBufferMemory);
		m_zDevice.copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

		vkDestroyBuffer(m_zDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(m_zDevice.device(), stagingBufferMemory, nullptr);
	}
}
