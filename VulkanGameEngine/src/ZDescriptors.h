#pragma once

#include "ZDevice.h"

namespace ZZX
{
	class ZDescriptorSetLayout
	{
	public:
		/**
		 * This class is for building descriptor set layouts that the pipeline requires at creation.
		 */
		class Builder
		{
		public:
			Builder(ZDevice& zDevice) : m_zDevice{zDevice}
			{
			}

			Builder& addBinding(uint32_t binding,
			                    VkDescriptorType descriptorType,
			                    VkShaderStageFlags stageFlags,
			                    uint32_t count = 1);
			std::unique_ptr<ZDescriptorSetLayout> build() const;

		private:
			ZDevice& m_zDevice;
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings{};
		};

		ZDescriptorSetLayout(ZDevice& zDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
		~ZDescriptorSetLayout();
		ZDescriptorSetLayout(const ZDescriptorSetLayout&) = delete;
		ZDescriptorSetLayout& operator=(const ZDescriptorSetLayout&) = delete;

		VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout; }

	private:
		ZDevice& m_zDevice;
		VkDescriptorSetLayout m_descriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindings;

		friend class ZDescriptorWriter;
	};

	class ZDescriptorPool
	{
	public:
		class Builder
		{
		public:
			Builder(ZDevice& zDevice) : m_zDevice{zDevice}
			{
			}

			Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
			Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
			Builder& setMaxSets(uint32_t count);
			std::unique_ptr<ZDescriptorPool> build() const;

		private:
			ZDevice& m_zDevice;
			std::vector<VkDescriptorPoolSize> m_poolSizes{};
			uint32_t m_maxSets = 1000;
			VkDescriptorPoolCreateFlags m_poolFlags = 0;
		};

		ZDescriptorPool(
			ZDevice& zDevice,
			uint32_t maxSets,
			VkDescriptorPoolCreateFlags poolFlags,
			const std::vector<VkDescriptorPoolSize>& poolSizes);
		~ZDescriptorPool();
		ZDescriptorPool(const ZDescriptorPool&) = delete;
		ZDescriptorPool& operator=(const ZDescriptorPool&) = delete;

		bool allocateDescriptorSet(
			const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

		void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

		void resetPool();

	private:
		ZDevice& m_zDevice;
		VkDescriptorPool m_descriptorPool;

		friend class ZDescriptorWriter;
	};

	class ZDescriptorWriter
	{
	public:
		ZDescriptorWriter(ZDescriptorSetLayout& setLayout, ZDescriptorPool& pool);

		ZDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
		ZDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

		bool build(VkDescriptorSet& set);
		void overwrite(VkDescriptorSet& set);

	private:
		ZDescriptorSetLayout& m_setLayout;
		ZDescriptorPool& m_pool;
		std::vector<VkWriteDescriptorSet> m_writes;
	};
}
