#include "ZPipeline.h"

#include <cassert>
#include <fstream>
#include <iostream>

namespace ZZX
{
	ZPipeline::ZPipeline(ZDevice& zDevice,
	                     const PipelineConfigInfo& config_info,
	                     const std::string& vertFilepath,
	                     const std::string& fragFilepath)
		: m_zDevice(zDevice)
	{
		createGraphicsPipeline(vertFilepath, fragFilepath, config_info);
	}

	ZPipeline::~ZPipeline()
	{
		vkDestroyShaderModule(m_zDevice.device(), m_vertShaderModule, nullptr);
		vkDestroyShaderModule(m_zDevice.device(), m_fragShaderModule, nullptr);
		vkDestroyPipeline(m_zDevice.device(), m_graphicsPipeline, nullptr);
	}

	void ZPipeline::bind(VkCommandBuffer commandBuffer)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
	}

	void ZPipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo)
	{
		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		// "triangle from every 3 vertices without reuse"
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		configInfo.viewportInfo.viewportCount = 1;
		// nullptr here b/c we're using dynamic viewports
		configInfo.viewportInfo.pViewports = nullptr; 
		configInfo.viewportInfo.scissorCount = 1;
		// nullptr here b/c we're using dynamic scissors
		configInfo.viewportInfo.pScissors = nullptr; 

		configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
		configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		// configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
		configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
		configInfo.rasterizationInfo.depthBiasClamp = 0.0f; // Optional
		configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f; // Optional
		configInfo.rasterizationInfo.lineWidth = 1.0f;

		// Multi-sampling stage (disable for now)
		configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
		configInfo.multisampleInfo.minSampleShading = 1.0f; // Optional
		configInfo.multisampleInfo.pSampleMask = nullptr; // Optional
		configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
		configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE; // Optional

		// the per-framebuffer color blending settings
		configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
		configInfo.colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;

		// the global color blending settings
		configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
		configInfo.colorBlendInfo.attachmentCount = 1;
		configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
		configInfo.colorBlendInfo.blendConstants[0] = 0.0f; // Optional
		configInfo.colorBlendInfo.blendConstants[1] = 0.0f; // Optional
		configInfo.colorBlendInfo.blendConstants[2] = 0.0f; // Optional
		configInfo.colorBlendInfo.blendConstants[3] = 0.0f; // Optional

		// depth stencil settings
		configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.front = {}; // optional
		configInfo.depthStencilInfo.back = {}; // optional
		configInfo.depthStencilInfo.minDepthBounds = 0.0f; // optional
		configInfo.depthStencilInfo.maxDepthBounds = 1.0f; // optional

		// dynamic state: configure the pipeline to expect dynamic viewport/scissor to be provided later
		configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
		configInfo.dynamicStateInfo.dynamicStateCount =
			static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
		configInfo.dynamicStateInfo.flags = 0;
	}

	std::vector<char> ZPipeline::readFile(const std::string& filepath)
	{
		// start reading at the end of the file + read file as binary
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file: " + filepath);
		}

		// since we read at the end of the file, we can directly compute the size of the file
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		// seek back to the beginning of the file
		file.seekg(0);
		// read all of the bytes at once
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	void ZPipeline::createGraphicsPipeline(const std::string& vertFilepath,
	                                       const std::string& fragFilepath,
	                                       const PipelineConfigInfo& config_info)
	{
		assert(
			config_info.pipelineLayout != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline:: no pipelineLayout provided in configInfo");
		assert(
			config_info.renderPass != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline:: no renderPass provided in configInfo");

		// load the bytecode of the two shaders
		auto vertShaderCode = readFile(vertFilepath);
		auto fragShaderCode = readFile(fragFilepath);

		createShaderModule(vertShaderCode, &m_vertShaderModule);
		createShaderModule(fragShaderCode, &m_fragShaderModule);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = m_vertShaderModule,
			// specify entry point
			.pName = "main",
		};

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = m_fragShaderModule,
			.pName = "main",
		};

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		// Vertex input stage

		auto bindingDescriptions = ZModel::Vertex::getBindingDescriptions();
		auto attributeDescriptions = ZModel::Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size()),
			.pVertexBindingDescriptions = bindingDescriptions.data(),
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
			.pVertexAttributeDescriptions = attributeDescriptions.data(),
		};

		// create graphics pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = shaderStages,
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &config_info.inputAssemblyInfo,
			.pViewportState = &config_info.viewportInfo,
			.pRasterizationState = &config_info.rasterizationInfo,
			.pMultisampleState = &config_info.multisampleInfo,
			.pDepthStencilState = &config_info.depthStencilInfo,
			.pColorBlendState = &config_info.colorBlendInfo,
			.pDynamicState = &config_info.dynamicStateInfo, // dynamic viewport/scissor
			.layout = config_info.pipelineLayout,
			.renderPass = config_info.renderPass,
			.subpass = config_info.subpass,
			.basePipelineHandle = VK_NULL_HANDLE, // Optional
			.basePipelineIndex = -1, // Optional
		};

		if (vkCreateGraphicsPipelines(m_zDevice.device(),
		                              VK_NULL_HANDLE,
		                              1,
		                              &pipelineInfo,
		                              nullptr,
		                              &m_graphicsPipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphics pipeline!");
		}
	}

	void ZPipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule)
	{
		VkShaderModuleCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			// specify the length of the bytecode in bytes
			.codeSize = code.size(),
			// notice that pCode expects const uint32_t* rather than const char*
			// Note: this works because we use std::vector<char>, which ensures the data satisfy the worst-case alignment requirement
			.pCode = reinterpret_cast<const uint32_t*>(code.data())
		};

		if (vkCreateShaderModule(m_zDevice.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create shader module!");
		}
	}
}
