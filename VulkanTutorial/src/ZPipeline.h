#pragma once
#include "ZDevice.h"
#include "ZModel.h"

#include <string>
#include <vector>

namespace ZZX
{
	// this struct contains data specifying how we want to configure the pipeline
	struct PipelineConfigInfo
	{
		PipelineConfigInfo() = default;
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
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
		void operator=(const ZPipeline&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo, uint32_t width, uint32_t height);
	private:
		// read all of the bytes from the specified file and return them in a byte array
		static std::vector<char> readFile(const std::string& filepath);
		void createGraphicsPipeline(const std::string& vertFilepath,
		                            const std::string& fragFilepath,
		                            const PipelineConfigInfo& config_info);
		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		ZDevice& m_zDevice;
		VkPipeline m_graphicsPipeline;
		VkShaderModule m_vertShaderModule;
		VkShaderModule m_fragShaderModule;
	};
}
