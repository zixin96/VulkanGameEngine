#pragma once
#include "ZCamera.h"

#include <vulkan/vulkan.h>

namespace ZZX
{
	struct FrameInfo
	{
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		ZCamera& camera;
		VkDescriptorSet globalDescriptorSet;
	};
}
