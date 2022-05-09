#include "ZModel.h"
#include "ZUtils.h"

#include <iostream>
#include <unordered_map>
#include <tinyobjloader/tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std
{
	template <>
	struct hash<ZZX::ZModel::Vertex>
	{
		size_t operator()(ZZX::ZModel::Vertex const& vertex) const
		{
			size_t seed = 0;
			ZZX::hashCombine(seed, vertex.pos, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

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
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		attributeDescriptions.push_back({
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT, // Vertex::pos
			.offset = offsetof(Vertex, pos),
		});
		attributeDescriptions.push_back({
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT, // Vertex::color
			.offset = offsetof(Vertex, color),
		});
		attributeDescriptions.push_back({
			.location = 2,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT, // Vertex::normal
			.offset = offsetof(Vertex, normal),
		});
		attributeDescriptions.push_back({
			.location = 3,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT, // Vertex::uv
			.offset = offsetof(Vertex, uv),
		});
		return attributeDescriptions;
	}

	void ZModel::Builder::loadModel(const std::string& filepath)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
		{
			throw std::runtime_error(warn + err);
		}
		vertices.clear();
		indices.clear();

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};
				if (index.vertex_index >= 0)
				{
					vertex.pos = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};

					vertex.color = {
						attrib.colors[3 * index.vertex_index + 0],
						attrib.colors[3 * index.vertex_index + 1],
						attrib.colors[3 * index.vertex_index + 2],
					};
				}

				if (index.normal_index >= 0)
				{
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};
				}

				if (index.texcoord_index >= 0)
				{
					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1],
					};
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}
				indices.push_back(uniqueVertices[vertex]);
			}
		}
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

	std::unique_ptr<ZModel> ZModel::createModelFromFile(ZDevice& device, const std::string& filepath)
	{
		Builder builder{};
		builder.loadModel(filepath);
		std::cout << "Vertex count: " << builder.vertices.size() << '\n';
		return std::make_unique<ZModel>(device, builder);
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
