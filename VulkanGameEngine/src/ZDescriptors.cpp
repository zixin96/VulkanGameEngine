#include "pch.h"
#include "ZDescriptors.h"

namespace ZZX
{
	// *************** Descriptor Set Layout Builder *********************

	/**
	 * \brief Appends to the map a {binding point-VkDescriptorSetLayoutBinding} pair
	 * \param binding What binding point to give?
	 * \param descriptorType What kinds of descriptor set to expect (uniform/storage/image buffer?)
	 * \param stageFlags Which shader stages will have access to the binding (vertex/fragment or both)
	 * \param count How many descriptors does this binding have?
	 * \return A reference to itself (make it easy to chain multiple addBinding calls together)
	 */
	ZDescriptorSetLayout::Builder& ZDescriptorSetLayout::Builder::addBinding(uint32_t binding,
	                                                                         VkDescriptorType descriptorType,
	                                                                         VkShaderStageFlags stageFlags,
	                                                                         uint32_t count)
	{
		assert(m_bindings.count(binding) == 0 && "Binding already in use");
		VkDescriptorSetLayoutBinding layoutBinding{
			.binding = binding,
			.descriptorType = descriptorType,
			.descriptorCount = count,
			.stageFlags = stageFlags,
		};
		m_bindings[binding] = layoutBinding;
		return *this;
	}

	/**
	 * \brief As the final step, create an instance of ZDescriptorSetLayout
	 * \return an instance of ZDescriptorSetLayout
	 */
	std::unique_ptr<ZDescriptorSetLayout> ZDescriptorSetLayout::Builder::build() const
	{
		return std::make_unique<ZDescriptorSetLayout>(m_zDevice, m_bindings);
	}

	// *************** Descriptor Set Layout *********************

	ZDescriptorSetLayout::ZDescriptorSetLayout(ZDevice& zDevice,
	                                           std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
		: m_zDevice{zDevice}, m_bindings{bindings}
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
		for (auto& kv : bindings)
		{
			setLayoutBindings.push_back(kv.second);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(zDevice.device(),
		                                &descriptorSetLayoutInfo,
		                                nullptr,
		                                &m_descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	ZDescriptorSetLayout::~ZDescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout(m_zDevice.device(), m_descriptorSetLayout, nullptr);
	}

	// *************** Descriptor Pool Builder *********************

	ZDescriptorPool::Builder& ZDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType,
	                                                                uint32_t count)
	{
		m_poolSizes.push_back({descriptorType, count});
		return *this;
	}

	ZDescriptorPool::Builder& ZDescriptorPool::Builder::setPoolFlags(
		VkDescriptorPoolCreateFlags flags)
	{
		m_poolFlags = flags;
		return *this;
	}

	ZDescriptorPool::Builder& ZDescriptorPool::Builder::setMaxSets(uint32_t count)
	{
		m_maxSets = count;
		return *this;
	}

	std::unique_ptr<ZDescriptorPool> ZDescriptorPool::Builder::build() const
	{
		return std::make_unique<ZDescriptorPool>(m_zDevice, m_maxSets, m_poolFlags, m_poolSizes);
	}

	// *************** Descriptor Pool *********************

	ZDescriptorPool::ZDescriptorPool(ZDevice& zDevice,
	                                 uint32_t maxSets,
	                                 VkDescriptorPoolCreateFlags poolFlags,
	                                 const std::vector<VkDescriptorPoolSize>& poolSizes)
		: m_zDevice{zDevice}
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSets;
		descriptorPoolInfo.flags = poolFlags;

		if (vkCreateDescriptorPool(zDevice.device(), &descriptorPoolInfo, nullptr, &m_descriptorPool) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	ZDescriptorPool::~ZDescriptorPool()
	{
		vkDestroyDescriptorPool(m_zDevice.device(), m_descriptorPool, nullptr);
	}

	bool ZDescriptorPool::allocateDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout,
	                                            VkDescriptorSet& descriptor) const
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.pSetLayouts = &descriptorSetLayout;
		allocInfo.descriptorSetCount = 1;

		// Might want to create a "DescriptorPoolManager" class that handles this case, and builds
		// a new pool whenever an old pool fills up. But this is beyond our current scope
		if (vkAllocateDescriptorSets(m_zDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	void ZDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const
	{
		vkFreeDescriptorSets(m_zDevice.device(),
		                     m_descriptorPool,
		                     static_cast<uint32_t>(descriptors.size()),
		                     descriptors.data());
	}

	void ZDescriptorPool::resetPool()
	{
		vkResetDescriptorPool(m_zDevice.device(), m_descriptorPool, 0);
	}

	// *************** Descriptor Writer *********************

	ZDescriptorWriter::ZDescriptorWriter(ZDescriptorSetLayout& setLayout, ZDescriptorPool& pool)
		: m_setLayout{setLayout}, m_pool{pool}
	{
	}

	ZDescriptorWriter& ZDescriptorWriter::writeBuffer(
		uint32_t binding, VkDescriptorBufferInfo* bufferInfo)
	{
		assert(m_setLayout.m_bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto& bindingDescription = m_setLayout.m_bindings[binding];

		assert(
			bindingDescription.descriptorCount == 1 &&
			"Binding single descriptor info, but binding expects multiple");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pBufferInfo = bufferInfo;
		write.descriptorCount = 1;

		m_writes.push_back(write);
		return *this;
	}

	ZDescriptorWriter& ZDescriptorWriter::writeImage(
		uint32_t binding, VkDescriptorImageInfo* imageInfo)
	{
		assert(m_setLayout.m_bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto& bindingDescription = m_setLayout.m_bindings[binding];

		assert(
			bindingDescription.descriptorCount == 1 &&
			"Binding single descriptor info, but binding expects multiple");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pImageInfo = imageInfo;
		write.descriptorCount = 1;

		m_writes.push_back(write);
		return *this;
	}

	bool ZDescriptorWriter::build(VkDescriptorSet& set)
	{
		bool success = m_pool.allocateDescriptorSet(m_setLayout.getDescriptorSetLayout(), set);
		if (!success)
		{
			return false;
		}
		overwrite(set);
		return true;
	}

	void ZDescriptorWriter::overwrite(VkDescriptorSet& set)
	{
		for (auto& write : m_writes)
		{
			write.dstSet = set;
		}
		vkUpdateDescriptorSets(m_pool.m_zDevice.device(),
		                       static_cast<uint32_t>(m_writes.size()),
		                       m_writes.data(),
		                       0,
		                       nullptr);
	}
} // namespace ZZX
