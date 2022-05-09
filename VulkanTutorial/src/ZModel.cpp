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
		VkBuffer buffers[] = {m_vertexBuffer->getBuffer()};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
		if (m_hasIndexBuffer)
		{
			vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
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

		uint32_t vertexSize = sizeof(vertices[0]);
		ZBuffer stagingBuffer{
			m_zDevice,
			vertexSize,
			m_vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)vertices.data());

		m_vertexBuffer = std::make_unique<ZBuffer>(m_zDevice,
		                                           vertexSize,
		                                           m_vertexCount,
		                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		m_zDevice.copyBuffer(stagingBuffer.getBuffer(), m_vertexBuffer->getBuffer(), bufferSize);
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
		uint32_t indexSize = sizeof(indices[0]);
		ZBuffer stagingBuffer{
			m_zDevice,
			indexSize,
			m_indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};
		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)indices.data());

		m_indexBuffer = std::make_unique<ZBuffer>(m_zDevice,
		                                          indexSize,
		                                          m_indexCount,
		                                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		m_zDevice.copyBuffer(stagingBuffer.getBuffer(), m_indexBuffer->getBuffer(), bufferSize);
	}
}
