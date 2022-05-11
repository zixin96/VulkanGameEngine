#pragma once
#include "ZDevice.h"
#include "ZModel.h"

namespace ZZX
{
	// this struct contains data specifying how we want to configure the pipeline
	struct PipelineConfigInfo
	{
		PipelineConfigInfo() = default;
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout m_VkPipelineLayout = nullptr;
		VkRenderPass m_VkRenderPass = nullptr;
		uint32_t subpass = 0;
	};

	class ZPipeline
	{
	public:
		ZPipeline(ZDevice& device,
		          const PipelineConfigInfo& config_info,
		          const std::string& vertFilepath,
		          const std::string& fragFilepath);
		~ZPipeline();

		ZPipeline(const ZPipeline&) = delete;
		ZPipeline& operator=(const ZPipeline&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
		static void enableAlphaBlending(PipelineConfigInfo& configInfo);
	private:
		// read all of the bytes from the specified file and return them in a byte array
		static std::vector<char> readFile(const std::string& filepath);
		void createGraphicsPipeline(const std::string& vertFilepath,
		                            const std::string& fragFilepath,
		                            const PipelineConfigInfo& config_info);
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		ZDevice& m_ZDevice;
		VkPipeline m_VkPipeline;
		VkShaderModule m_VkVertexShaderModule;
		VkShaderModule m_VkFragmentShaderModule;
	};
}
