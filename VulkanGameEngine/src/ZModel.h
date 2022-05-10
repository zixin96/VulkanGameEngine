#pragma once
#include "ZDevice.h"
#include "ZBuffer.h"

namespace ZZX
{
	class ZModel
	{
	public:
		struct Vertex
		{
			glm::vec3 pos{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

			bool operator==(const Vertex& other) const
			{
				return pos == other.pos &&
					color == other.color &&
					normal == other.normal &&
					uv == other.uv;
			}
		};

		struct Builder
		{
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			void loadModel(const std::string& filepath);
		};

		ZModel(ZDevice& zDevice, const ZModel::Builder& builder);
		~ZModel();

		// delete copy ctor and assignment to avoid dangling pointer
		ZModel(const ZModel&);
		ZModel& operator=(const ZModel&);

		static std::unique_ptr<ZModel> createModelFromFile(ZDevice& device, const std::string& filepath);

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);
	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		ZDevice& m_zDevice;

		std::unique_ptr<ZBuffer> m_vertexBuffer;
		uint32_t m_vertexCount;

		bool m_hasIndexBuffer = false;
		std::unique_ptr<ZBuffer> m_indexBuffer;
		uint32_t m_indexCount;
	};
};
